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

#define IS_EMPTY (minorStreams[minor] == NULL)
#define O_PACKET 0x80000000

DECLARE_WAIT_QUEUE_HEAD(read_queue);
DECLARE_WAIT_QUEUE_HEAD(write_queue);

spinlock_t buffer_lock[DEVICE_MAX_NUMBER];
Packet* minorStreams[DEVICE_MAX_NUMBER];

// Stream/Packet dynamic sizes
atomic_t maxStreamSizes[DEVICE_MAX_NUMBER];
atomic_t segmentSizes[DEVICE_MAX_NUMBER];
atomic_t countBytes[DEVICE_MAX_NUMBER];

static int ll_open(struct inode *inode, struct file *filp) {
	int minor;

	try_module_get(THIS_MODULE);
	minor = iminor(filp->f_path.dentry->d_inode);
	printk(KERN_INFO "open operation on device with minor %d is called\n", minor);
	
	if( minor < DEVICE_MAX_NUMBER) {
		atomic_set(&(maxStreamSizes[minor]), MAX_STREAM_SIZE);
		atomic_set(&(segmentSizes[minor]), MAX_PACKET_SIZE);
		atomic_set(&(countBytes[minor]),0);
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
        	if( size < MIN_LIMIT_STREAM || size > MAX_LIMIT_STREAM ){
        		return -EINVAL;
        	}
        	atomic_set(&(maxStreamSizes[minor]), size);
        	
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
	int minor = iminor(filp->f_path.dentry->d_inode);
	//int result = 0;
	//int buff_size = 0;
	printk("write operation on device with minor %d is called\n",minor);
	if(count < MIN_LIMIT_PACKET){
		printk("error : bytes lower than the minimum packet size\n");
		return -EINVAL;	
	}
	else if (count > MAX_LIMIT_PACKET){
		printk("error : bytes greater than the maximum packet size\n");
		return -EINVAL;
	}
	return 0;
}

static ssize_t dharma_read_packet(struct file *filp, char *out_buffer, size_t size, loff_t *offset) {
	int minor=iminor(filp->f_path.dentry->d_inode);
    int res = 0;
    int residual;
    int to_end;
    int byte_read = 0;
    int buffer_size;
    printk("Read-Packet was called on dharma-device %d\n", minor);


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
    
    wake_up_interruptible(&write_queue);

    spin_unlock(&(buffer_lock[minor]));
    return byte_read;
}

static ssize_t dharma_read_stream(struct file *filp, char *out_buffer, size_t size, loff_t *offset) {
	
}

static ssize_t ll_read(struct file *filp, char *buffer, size_t count, loff_t *f_pos) {
	if ((unsigned long)filp->private_data & O_PACKET)
        return dharma_read_packet(filp, out_buffer, size, offset);
    return dharma_read_stream(filp, out_buffer, size, offset);
}


static struct file_operations fops = {
	.read = ll_read,
	.write = ll_write,
	.open = ll_open,
	.release = ll_release,
	.unlocked_ioctl = ll_ioctl
};

int init_module(void){
	
	/* with major==0 the function dinamically allocates a major and return corresponding number */
	major = register_chrdev(0, DEVICE_NAME, &fops); 
	if (major < 0) {
	  printk("registering linkedlist device failed\n");
	  return major;
	}
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
