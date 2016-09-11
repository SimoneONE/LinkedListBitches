#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include "linked_list.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("BOOM BITCHES");

int major;

#define IS_EMPTY(minor) (minorStreams[minor] == NULL)
#define O_PACKET 0x80000000

DECLARE_WAIT_QUEUE_HEAD(read_queue);
DECLARE_WAIT_QUEUE_HEAD(write_queue);

spinlock_t buffer_lock[DEVICE_MAX_NUMBER];
Packet* minorStreams[DEVICE_MAX_NUMBER];
Packet* lastPacket[DEVICE_MAX_NUMBER];
// Stream/Packet dynamic sizes
atomic_t maxStreamSizes[DEVICE_MAX_NUMBER];
atomic_t segmentSizes[DEVICE_MAX_NUMBER];
atomic_t countBytes[DEVICE_MAX_NUMBER];
atomic_t activeStreams[DEVICE_MAX_NUMBER];

static int ll_open(struct inode *inode, struct file *filp) {
	int minor;

	try_module_get(THIS_MODULE);
	minor = iminor(filp->f_path.dentry->d_inode);
	printk(KERN_INFO "open operation on device with minor %d is called\n", minor);
	
	if( minor < DEVICE_MAX_NUMBER) {
		if(atomic_read(&activeStreams[minor]) == 0){
			atomic_set(&activeStreams[minor], 1);
			atomic_set(&(maxStreamSizes[minor]), MAX_STREAM_SIZE);
			atomic_set(&(segmentSizes[minor]), MAX_PACKET_SIZE);
			atomic_set(&(countBytes[minor]),0);
		}
		return 0; /* success */
	} 
	else {
		printk(KERN_INFO "minor not allowed\n");
		return -ENODEV;
	}
}

static int ll_release(struct inode *inode, struct file *filp) {
	int minor = iminor(filp->f_path.dentry->d_inode);
	module_put(THIS_MODULE);/* decrements the reference counter*/

	/*I do not free the buffer because may be another device with
	  the same pair <major,minor> that is working on that buffer.
	  The buffer is freed when the module is unmounted.*/

	printk(KERN_INFO "release operation on device with minor %d is called\n",minor);
	return 0;
}


static long ll_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
	int minor;
	int res;
	int size;	
	int old_size;
	printk(KERN_INFO "ioctl called\n");
	minor = iminor(filp->f_path.dentry->d_inode);
	printk(KERN_INFO "minor is %d\n", minor);
	switch (cmd) {
        case LL_SET_PACKET_MODE :
            printk("Packet mode now is active on ll-device %d\n", minor);
            filp->private_data = (void *) ((unsigned long)filp->private_data | O_PACKET);
            break;
        case LL_SET_STREAM_MODE :
            printk("Stream mode now is active on ll-device %d\n", minor);
            filp->private_data = (void *) ((unsigned long)filp->private_data & ~O_PACKET);
            break;
        case LL_SET_BLOCKING :
            printk("Blocking mode now is active on ll-device %d\n", minor);
            filp->f_flags &= ~O_NONBLOCK;
            break;
        case LL_SET_NONBLOCKING :
            printk("Non blocking mode now is active on ll-device %d\n", minor);
            filp->f_flags |= O_NONBLOCK;
            break;
        case LL_GET_MAX_SIZE:
        	printk("Returning current buffer size for ll-device %d...\n", minor);
			size = atomic_read(&(maxStreamSizes[minor]));
					res = copy_to_user((int *) arg, &size , sizeof(int));
					if(res != 0)
					return -EINVAL; // if copy_from_user didn't return 0, there was a problem in the parameters.
			printk("Buffer size for ll-device %d read.\n", minor);
        	break;
        case LL_SET_MAX_SIZE:
        	res = copy_from_user(&size, (int *) arg, sizeof(int));
        	old_size = atomic_read(&(maxStreamSizes[minor]));
        	if( size < MIN_LIMIT_STREAM || size > MAX_LIMIT_STREAM ){
        		return -EINVAL;
        	}
        	atomic_set(&(maxStreamSizes[minor]), size);
        	
        	if(size > old_size)
				wake_up_interruptible(&write_queue);
			printk(KERN_INFO "Maximum buffer size set to: %d", size);
        	break;
		case LL_GET_PACK_SIZE:
        	printk("Returning current packet size for ll-device %d...\n", minor);
			size = atomic_read(&(segmentSizes[minor]));
			res = copy_to_user((int *) arg, &size , sizeof(int));
			if(res != 0)
				return -EINVAL; // if copy_from_user didn't return 0, there was a problem in the parameters.
			printk("Packet size for ll-device %d read.\n", minor);
        	break;
        case LL_SET_PACK_SIZE:
        	res = copy_from_user(&size, (int *) arg, sizeof(int));
        	if( size < MIN_LIMIT_PACKET || size > MAX_LIMIT_PACKET ){
        		return -EINVAL;
        	}
        	atomic_set(&(segmentSizes[minor]), size);
        	
            printk(KERN_INFO "Maximum packet size set to: %d", size);
        	break;
        default:
            return -EINVAL;
    }
	return 0;
}

static ssize_t ll_write(struct file *filp, const char *buff, size_t count, loff_t *f_pos) {
	Packet* p;
	int res;
	int buff_size;
	int bytes_busy;
	int minor = iminor(filp->f_path.dentry->d_inode);
	printk("write operation on device with minor %d is called\n",minor);
	if(count < MIN_LIMIT_PACKET){
		printk("error : bytes lower than the minimum packet size\n");
		return -EINVAL;	
	}
	else if (count > MAX_LIMIT_PACKET){
		printk("error : bytes greater than the maximum packet size\n");
		return -EINVAL;
	}
	/*acquire the lock*/
	spin_lock(&(buffer_lock[minor]));
	buff_size = atomic_read(&maxStreamSizes[minor]);
	bytes_busy = atomic_read(&countBytes[minor]);
	printk("before buffer check, buff_size=%d bytes_busy=%d\n",buff_size,bytes_busy);
	while (bytes_busy >= buff_size) {
		printk("the buffer is full\n");
		/*release spinlock*/
		spin_unlock(&(buffer_lock[minor]));
		if (filp->f_flags & O_NONBLOCK) {
				printk("mode is not-blocking therefore return\n");
				return -EAGAIN; //the buffer is full therefore no data can be written
		}
		printk("mode is blocking therefore sleep on the write queue\n");
		if (wait_event_interruptible(write_queue, ( atomic_read(&countBytes[minor]) < atomic_read(&maxStreamSizes[minor])) ) ){
			printk("a signal is received.Exit\n");
			return -ERESTARTSYS; // Woke up by a signal -> -ERESTARTSYS
		}
		
		spin_lock(&(buffer_lock[minor]));
		buff_size = atomic_read(&maxStreamSizes[minor]);
		bytes_busy = atomic_read(&countBytes[minor]);
   	}
   	printk("after buffer check buff_size=%d bytes_busy=%d\n",buff_size,bytes_busy);
	// From now exclusive access + buffer not full
	if((buff_size-bytes_busy)<count) /* BEST EFFORT WRITE */
		count = buff_size - bytes_busy;
	
	/*allocating struct Packet*/
	p = kmalloc(sizeof(Packet), GFP_KERNEL);
  	p->buffer = kmalloc(count, GFP_KERNEL);
  	p->bufferSize = count;
	p->readPos = 0;
	p->next = NULL;
	/*add the packet on the head of the linkedlist*/
	if(IS_EMPTY(minor)) {
		printk("head packet case\n");
		minorStreams[minor] = p;
		lastPacket[minor] = p;
	}
	/*add the packet on queue of the linkedlist*/
	else {
		printk("queue packet case\n");
		lastPacket[minor]->next = p;
		lastPacket[minor] = lastPacket[minor]->next;
	}
	res = copy_from_user(p->buffer, buff, count);
	if(res != 0) {
		printk("error : failed to copy from user\n");
		spin_unlock(&(buffer_lock[minor]));
		return -EINVAL; // Error in the copy_to_user (res !=  0)
	}
	printk("written %d bytes\n",(int)count);
  	//atomic_set(&(countBytes[minor]),bytes_busy+count);
	atomic_add(count,&(countBytes[minor]));
	buff_size = atomic_read(&maxStreamSizes[minor]);
	bytes_busy = atomic_read(&countBytes[minor]);
	printk("buff_size = %d bytes_busy = %d\n",buff_size,bytes_busy);
	wake_up_interruptible(&(read_queue));
	spin_unlock(&(buffer_lock[minor]));
	return count;
}

static ssize_t ll_read_packet(struct file *filp, char *out_buffer, size_t size, loff_t *offset) {
	int minor=iminor(filp->f_path.dentry->d_inode);
	int res;
	int readable_bytes;
	int to_copy;
	Packet* p;
        char * temp_buff;
    printk("Read-Packet was called on ll-device %d\n", minor);

    // acquire spinlock
    spin_lock(&(buffer_lock[minor]));

    printk("Before buffer check\n");

    while (IS_EMPTY(minor)) {
        printk("Buffer is empty\n");
        // release spinlock
        spin_unlock(&(buffer_lock[minor]));

        printk("Should we block?\n");
        if (filp->f_flags & O_NONBLOCK) {
            printk("No blocking\n");
            return -EAGAIN;  // EGAIN:resource is temporarily unavailable
        }

        printk("Sleeping on the read queue\n");
        if(wait_event_interruptible(read_queue, !IS_EMPTY(minor)))
            return -ERESTARTSYS;
            
        // otherwise loop, but first re-acquire spinlock
        spin_lock(&(buffer_lock[minor]));
    }
    // if we get here, then data is in the buffer AND we have exclusive access to it: we're ready to go.
    printk("After buffer check\n");
    
    // First packet in the stream
    p = minorStreams[minor];
    
    // Checking for previous read-streams
    readable_bytes = p->bufferSize - p->readPos;
    
    // Checking for bytes to be read effectively
    if(size < readable_bytes)
		to_copy = size;
	else
		to_copy = readable_bytes;
    
    // Copying to user buffer
    /* TODO (Optimization tip): Dovremo usare anche qui un buffer 
     * temporaneo come per la read stream. */
    temp_buff = kmalloc(to_copy,GFP_KERNEL);
    memcpy((void*)(&(temp_buff[0])), (void*)(&(p->buffer[p->readPos])), to_copy);
    // Update list and counters
    minorStreams[minor] = minorStreams[minor]->next;
    // Case 1: readPos bytes already counted
    // atomic_sub(readable_bytes, &countBytes[minor]);
    // Case 2: readPos bytes not counted
    atomic_sub(p->bufferSize, &countBytes[minor]);
    kfree(p);
    printk("countBytes updated to %d", atomic_read(&countBytes[minor]));
    wake_up_interruptible(&write_queue);
    spin_unlock(&(buffer_lock[minor]));

    res = copy_to_user(out_buffer, (char *)temp_buff, to_copy);
    
    //if res>0, it means an unexpected error happened, so we abort the operation (=not update pointers)
    if(res != 0)
        //as if 0 bytes were read. Exit
        return -EINVAL;
    
    // Free the memory
    kfree(temp_buff);
    
    return to_copy;
}
/*
static ssize_t ll_read_stream(struct file *filp, char *out_buffer, size_t size, loff_t *offset) {
	int minor=iminor(filp->f_path.dentry->d_inode);
	int res;
	int bytes_read = 0;
	char* temp_buff;
	int tempPos = 0;
	int readPosTemp=0;
	int bufferSizeTemp=0;
	int to_read;
	int left;
	Packet* p;
	Packet* temp;
    
    printk("Read-Stream was called on ll-device %d\n", minor);

    // acquire spinlock
    spin_lock(&(buffer_lock[minor]));

    printk("Before buffer check\n");

    while (IS_EMPTY(minor)) {
        printk("Buffer is empty\n");
        // release spinlock
        spin_unlock(&(buffer_lock[minor]));

        printk("Should we block?\n");
        if (filp->f_flags & O_NONBLOCK) {
            printk("No blocking\n");
            return -EAGAIN;  // EGAIN:resource is temporarily unavailable
        }

        printk("Sleeping on the read queue\n");
        if(wait_event_interruptible(read_queue, !IS_EMPTY(minor)))
            return -ERESTARTSYS;
            
        // otherwise loop, but first re-acquire spinlock
        spin_lock(&(buffer_lock[minor]));
    }
    // if we get here, then data is in the buffer AND we have exclusive access to it: we're ready to go.
    printk("After buffer check\n");
    
    // Alloc temporary buff to keep output
    temp_buff = kmalloc(size, GFP_KERNEL);
    
    // First packet in the stream
    p = minorStreams[minor];
    printk("before while\n");
    while(p != NULL && bytes_read != size) {
		// left to read
		printk("size: %d, bytes_read: %d\n", (int)size, bytes_read);
		left = size - bytes_read;
		printk("left: %d\n", left);
		// How much to read this round
		printk("p->bufferSize: %d, p->readPos: %d\n", p->bufferSize, p->readPos);
		if(left < p->bufferSize - p->readPos) {
			to_read = left;
			printk("left < p->bufferSize - p->readPos => to_read: %d\n", to_read);
		}
		else {
			to_read = p->bufferSize - p->readPos;
			printk("left >= p->bufferSize - p->readPos => to_read: %d\n", to_read);
		}
		
		memcpy((void*)(&(temp_buff[tempPos])), (void*)(&(p->buffer[p->readPos])), to_read);
		
		// Update bytes_read
		bytes_read += to_read;
		tempPos += to_read;
		printk("Updating => bytes_read: %d, tempPos: %d\n", bytes_read, tempPos);
		
		// For sure, now bytes_read = size
		if(p->readPos + to_read < p->bufferSize) {
			// TODO (Optimization tip): We could in this case reduce the
			// size needed for the packet, in order to free some of the 
			// bytes that was previuosly used to keep the bytes that has
			// been already read by some user.
			p->readPos += to_read;
			printk("p->readPos + to_read < p->bufferSize => p->readPos: %d\n", p->readPos);
		}
		else {
			temp = p;
			readPosTemp = temp->readPos;
			bufferSizeTemp = temp->bufferSize;
			p = p->next;
			kfree(temp);
			printk("p->readPos + to_read >= p->bufferSize => readPosTemp: %d, bufferSizeTemp: %d\n", readPosTemp, bufferSizeTemp);
			if(p != NULL)
				printk("p = { bufferSize: %d, readPos: %d }\n", p->bufferSize, p->readPos);
			else
				printk("p = NULL\n");
		}		
	}
    printk("after while\n");
    minorStreams[minor]=p;
    // Copy the buffer to user
    res = copy_to_user(out_buffer, (char *)(temp_buff), bytes_read);
    
    // Free temp buff
    kfree(temp_buff);
    
    //if res>0, it means an unexpected error happened, so we abort the operation (=not update pointers)
    if(res != 0){
        //as if 0 bytes were read. Exit
        spin_unlock(&(buffer_lock[minor]));
        return -EINVAL;
    }
    
    // Case 1: readPos bytes already counted
    //atomic_sub(bytes_read, &countBytes[minor]);
    // Case 2: readPos bytes not counted
    
    if(p==NULL){
		printk("p = NULL!\n");
		if(readPosTemp!=bytes_read) {
			atomic_sub(bytes_read - readPosTemp, &countBytes[minor]);
			printk("updating countBytes => bytes_read - readPosTemp: %d\n", bytes_read - readPosTemp);	
		}
		else {
			atomic_sub(bytes_read,&countBytes[minor]);
			printk("updating countBytes => bytes_read: %d\n", bytes_read);
		}
    }
    else {
		printk("p != NULL!\n");
		if(p->readPos!=bytes_read) {
				atomic_sub(bytes_read - p->readPos, &countBytes[minor]);
				printk("updating countBytes => bytes_read - p->readPos: %d\n", bytes_read - p->readPos);
		}
        else {
			atomic_sub(bytes_read,&countBytes[minor]);
			printk("updating countBytes => bytes_read: %d\n", bytes_read);
		}
    }

    wake_up_interruptible(&write_queue);
    spin_unlock(&(buffer_lock[minor]));
    return bytes_read;
}
*/

static ssize_t ll_read_stream(struct file *filp, char *out_buffer, size_t size, loff_t *offset) {
	int minor=iminor(filp->f_path.dentry->d_inode);
	int res;
	int bytes_read = 0;
	int left;
	int to_read;
	int tempPos = 0;
	int freed = 0;
	char* temp_buff;
	Packet* p;
	Packet* temp;
	
	printk("Read-Stream was called on ll-device %d\n", minor);

    // acquire spinlock
    spin_lock(&(buffer_lock[minor]));

    printk("Before buffer check\n");

    while (IS_EMPTY(minor)) {
        printk("Buffer is empty\n");
        // release spinlock
        spin_unlock(&(buffer_lock[minor]));

        printk("Should we block?\n");
        if (filp->f_flags & O_NONBLOCK) {
            printk("No blocking\n");
            return -EAGAIN;  // EGAIN:resource is temporarily unavailable
        }

        printk("Sleeping on the read queue\n");
        if(wait_event_interruptible(read_queue, !IS_EMPTY(minor)))
            return -ERESTARTSYS;
            
        // otherwise loop, but first re-acquire spinlock
        spin_lock(&(buffer_lock[minor]));
    }
    // if we get here, then data is in the buffer AND we have exclusive access to it: we're ready to go.
    printk("After buffer check\n");
    
    // Alloc temporary buff to keep output
    temp_buff = kmalloc(size, GFP_KERNEL);
    
    // First packet in the stream
    p = minorStreams[minor];
    printk("before while\n");
    while(p != NULL && bytes_read != size) {
		left = size  - bytes_read;
		printk("left: %d\n", left);
		
		// Last packet to be read
		if(left <= p->bufferSize - p->readPos)
			to_read = left;
		// Not (Eventually) the last one
		else
			to_read = p->bufferSize - p->readPos;
			
		memcpy((void*)(&(temp_buff[tempPos])), (void*)(&(p->buffer[p->readPos])), to_read);
		
		// count to_read bytes
		bytes_read += to_read;
		tempPos += to_read;
		
		if(p->readPos + to_read < p->bufferSize)
			p->readPos += to_read;
		else {	
			temp = p;
			p = p->next;
			freed += temp->bufferSize;
			kfree(temp);
		}
	}
    
    printk("after while\n");
    minorStreams[minor]=p;
    // Case 1: readPos bytes already counted
    //atomic_sub(bytes_read, &countBytes[minor]);
    // Case 2: readPos bytes not counted
    atomic_sub(freed, &countBytes[minor]);
    printk("countBytes updated to %d", atomic_read(&countBytes[minor]));
    wake_up_interruptible(&write_queue);
    spin_unlock(&(buffer_lock[minor]));
    // Copy the buffer to user
    res = copy_to_user(out_buffer, (char *)(temp_buff), bytes_read);
    // Free temp buff
    kfree(temp_buff);
    //if res>0, it means an unexpected error happened, so we abort the operation (=not update pointers)
    if(res != 0)
        //as if 0 bytes were read. Exit
        return -EINVAL;
    return bytes_read;
}

static ssize_t ll_read(struct file *filp, char *buffer, size_t count, loff_t *f_pos) {
	/*int minor=iminor(filp->f_path.dentry->d_inode);
	Packet *p = minorStreams[minor];
	while(p!=NULL){
		char data[p->bufferSize+1];
		memcpy(data,p->buffer,p->bufferSize);
		data[p->bufferSize]='\0';
		printk("%s",data);
		p=p->next;
	}
	printk("\n");*/
	if ((unsigned long)filp->private_data & O_PACKET)
        return ll_read_packet(filp, buffer, count, f_pos);
        return ll_read_stream(filp, buffer, count, f_pos);
	//return 0;
}


static struct file_operations fops = {
	.read = ll_read,
	.write = ll_write,
	.open = ll_open,
	.release = ll_release,
	.unlocked_ioctl = ll_ioctl
};

int init_module(void){
	int i;
	
	/* with major==0 the function dinamically allocates a major and return corresponding number */
	major = register_chrdev(0, DEVICE_NAME, &fops); 
	if (major < 0) {
	  printk("registering linkedlist device failed\n");
	  return major;
	}

	for(i = 0; i < DEVICE_MAX_NUMBER; i++)
		atomic_set (&activeStreams[i], 0);
	
	printk("linkedlist device registered, it is assigned major number %d\n", major);
	return 0;
}

void cleanup_module(void) {
	int i;
	unregister_chrdev(major, DEVICE_NAME);
	printk(KERN_INFO "linkedlist device unregistered, it was assigned major number %d\n", major);
	// We have to free the streams
	for(i = 0; i < DEVICE_MAX_NUMBER; i++) {
		Packet* p = minorStreams[i];
		while(p != NULL) {
			Packet* temp = p;
			kfree(p->buffer);
			p = p->next;
			kfree(temp);
		}
	}
  	printk("removing memory module\n");
}
