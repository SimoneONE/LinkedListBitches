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

static int major;
static spinlock_t buffer_lock[DEVICE_MAX_NUMBER];
DECLARE_WAIT_QUEUE_HEAD(read_queue);
DECLARE_WAIT_QUEUE_HEAD(write_queue);
Packet* buffer[DEVICE_MAX_NUMBER];

static int ll_open(struct inode *inode, struct file *filp){
	try_module_get(THIS_MODULE);
	int minor = iminor(filp->f_path.dentry->d_inode);
	if( minor < DEVICE_MAX_NUMBER){
		return 0; /* success */
	} 
	else 
	{
		return -ENODEV;
	}
}

static int ll_release(struct inode *inode, struct file *filp){
	
	module_put(THIS_MODULE);
	return 0;
}


static long ll_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
	return 0;
}

static ssize_t ll_write(struct file *filp, const char *buff, size_t count, loff_t *f_pos){
	return 0;
}

static ssize_t ll_read(struct file *filp, char *buffer, size_t count, loff_t *f_pos){
	return 0;

}

static struct file_operations fops = {
	.read = ll_read,
	.write = ll_write,
	.open = ll_open,
	.release = ll_release,
	.unlocked_ioctl = ll_ioctl
};

int init_module(void){
	
	major = register_chrdev(0, DEVICE_NAME, &fops); // with major==0 the function dinamically allocates a major and return the
							     // corresponding number
	if (major < 0) {
	  printk("registering linkedlist device failed\n");
	  return major;
	}
	printk("linkedlist device registered, it is assigned major number %d\n", major);
	return 0;
}

void cleanup_module(void)
{

	unregister_chrdev(major, DEVICE_NAME);
	printk(KERN_INFO "linkedlist device unregistered, it was assigned major number %d\n", major);
	/* Freeing the device buffer da fare*/
  	printk("removing memory module\n");
}
