/*
 * Character device drivers lab
 *
 * All tasks
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/wait.h>

#include "../include/so2_cdev.h"

MODULE_DESCRIPTION("SO2 character device");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

#define LOG_LEVEL	KERN_INFO

#define MY_MAJOR		42
// #define MY_MAJOR		229
#define MY_MINOR		0
#define NUM_MINORS		2
#define MODULE_NAME		"so2_cdev"
#define MESSAGE			"hello\n"
#define IOCTL_MESSAGE		"Hello ioctl"

#ifndef BUFSIZ
#define BUFSIZ		4096
#endif

#define EXTRA

DEFINE_RWLOCK(lock);

struct so2_device_data {
	/* TODO 2: add cdev member */
	struct cdev dev;
	/* TODO 4: add buffer with BUFSIZ elements */
	uint8_t buffer[BUFSIZ];
	size_t buffer_size;
	/* TODO 7: extra members for home */
	// - flag for testing wait queue
	uint8_t is_free;
	// - wait queue : this will be used to keep a thread waiting for an event
	// Q : why do we need to have a 'waiting queue' per device driver ?
	// A : dunno
	struct wait_queue_head wq;
	/* TODO 3: add atomic_t access variable to keep track if file is opened */
	atomic_t is_busy;
};

struct so2_device_data devs[NUM_MINORS];

// iterate and print list of tasks waiting in the queue wq
void print_wait_queue(struct wait_queue_head* wq, const char* message)
{
	struct list_head *i;
	pr_info("%s : waiting queue : [", message);
	list_for_each(i, &(wq->head)) 
	{
		struct wait_queue_entry* wq_item = list_entry(i, struct wait_queue_entry, entry);
		struct task_struct* task = (struct task_struct*) wq_item->private;

		pr_info(" %d,", task->pid);
	}
	pr_info("]\n");
}

static int so2_cdev_open(struct inode *inode, struct file *file)
{
	struct so2_device_data *data;
	int is_busy;

	/* TODO 2: print message when the device file is open. */
	pr_info("device {%d,%d} open\n", MAJOR(inode->i_cdev->dev), MINOR(inode->i_cdev->dev));

	/* TODO 3: inode->i_cdev contains our cdev struct, use container_of to obtain a pointer to so2_device_data */
	data = container_of(inode->i_cdev, struct so2_device_data, dev);

	file->private_data = data;

	/* TODO 3: return immediately if access is != 0, use atomic_cmpxchg */
	// this is how atomic_cmpxchg() works:
	//	- 1 : signature : int atomic_cmpxchg(atomic_t *v, int cmp, int val)
	//	- 2 : one-liner : it compares cmp to v, setting it to val if cmp == v, else leaving v as it is, all in an atomic operation
	//	- 2 : what do the args mean? 
	//		- 2.1 : *v 	: pointer to atomic variable
	//		- 2.2 : cmp : the value we want to compare v against
	//		- 2.3 : val : the value we want to set v to in case v == cmp
	//	- 3 : atomic operation :
	//		- read the 32 bit value of v (typically referred to as 'old'), compare it against cmp
	//		- if cmp == old, set v to val. else, set v to old.
	//		- return old
	// is_busy = atomic_cmpxchg(&(data->is_busy), 0, 1);
	// if (is_busy)
	// {
	// 	pr_err("device {%d,%d} is busy\n", MAJOR(inode->i_cdev->dev), MINOR(inode->i_cdev->dev));
	// 	return -EBUSY;
	// }

	// this allows the task to be interrupted, preempted, etc.
	// set_current_state(TASK_INTERRUPTIBLE);
	// schedule_timeout(10 * HZ);

	return 0;
}

static int
so2_cdev_release(struct inode *inode, struct file *file)
{
	struct so2_device_data *data;
	
	/* TODO 2: print message when the device file is closed. */
	pr_info("device {%d,%d} close\n", MAJOR(inode->i_cdev->dev), MINOR(inode->i_cdev->dev));

#ifndef EXTRA
	data = (struct so2_device_data *) file->private_data;

	/* TODO 3: reset access variable to 0, use atomic_set */
	atomic_set(&(data->is_busy), 0);
#endif
	return 0;
}

static ssize_t
so2_cdev_read(struct file *file,
		char __user *user_buffer,
		size_t size, loff_t *offset)
{
	struct so2_device_data *data;		
	size_t to_read;
	char msg[128];

	data = (struct so2_device_data *) file->private_data;

#ifdef EXTRA
	/* TODO 7: extra tasks for home */

	// if we don't have data to read, check flags :
	//	- if O_NONBLOCK flag is set, return immediately
	//	- otherwise, wait in a waiting queue until data is available
	if (!data->buffer_size)
	{
		if (file->f_flags & O_NONBLOCK)
		{
			pr_info("(read) read operation on {%d,%d} would block. exiting.\n", MAJOR(data->dev.dev), MINOR(data->dev.dev));
			return -EWOULDBLOCK;
		}
		else
		{
			pr_info("(read) [pid:%d] adding {%d,%d} to queue\n", 
				current->pid, MAJOR(data->dev.dev), MINOR(data->dev.dev));
			
			snprintf(msg, 128, "(read) [pid:%d] before sleep", current->pid);
			print_wait_queue(&(data->wq), msg);

			wait_event_interruptible(data->wq, data->buffer_size > 0);

			pr_info("(read) [pid:%d] removing {%d,%d} from queue\n", 
				current->pid, MAJOR(data->dev.dev), MINOR(data->dev.dev));

			snprintf(msg, 128, "(read) [pid:%d] after wake up", current->pid);
			print_wait_queue(&(data->wq), msg);
		}
	}
#endif

	// update to_read based on offset :
	//	- at the start of a read cycle, you'll have strlen(data->buffer) to send, with (*offset) = 0
	//	- at any moment, you have strlen(data->buffer) - (*offset) to send
	//	- you can send a max of size byte to user-space
	to_read = min(strlen(data->buffer) - (size_t) *offset, size);

	/* TODO 4: Copy data->buffer to user_buffer, use copy_to_user */
	if (to_read <= 0)
	{
		return 0;
	}
	
	// order of args : to, from, size
	if (copy_to_user(user_buffer, data->buffer, to_read))
	{
		return -EFAULT;
	}

	// update offset
	*offset += to_read;

	// return the number of characters passed to the user-space!!!
	return to_read;
}

static ssize_t
so2_cdev_write(struct file *file,
		const char __user *user_buffer,
		size_t size, loff_t *offset)
{
	struct so2_device_data *data;
	size_t to_write;
	char msg[128];

	data = (struct so2_device_data *) file->private_data;

	/* TODO 5: copy user_buffer to data->buffer, use copy_from_user */
	to_write = min(strlen(user_buffer) - (size_t) *offset, size);

	if (to_write <= 0)
	{
		return 0;
	}

	// to, from, size
	if (copy_from_user(data->buffer, user_buffer, to_write))
	{
		return -EFAULT;
	}

	// update offset
	*offset += to_write;
	// add null character '\0' at end of content written to buffer
	if (*offset < BUFSIZ)
	{
		data->buffer[*offset] = 0;
	}

	/* TODO 7: extra tasks for home */
	// - update buffer_size variable
	data->buffer_size = strlen(data->buffer);
	// - there's something in buffer now : wake up waiting threads
	snprintf(msg, 128, "(write) [pid:%d] before wake up", current->pid);
	print_wait_queue(&(data->wq), msg);
	
	wake_up_interruptible(&(data->wq));

	return to_write;
}

static long
so2_cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct so2_device_data *data;
	int ret;
	int remains;
	size_t len;
	char msg[128];

	data = (struct so2_device_data *) file->private_data;
	ret = 0;

	switch (cmd) {
	/* TODO 6: if cmd = MY_IOCTL_PRINT, display IOCTL_MESSAGE */
	case MY_IOCTL_PRINT:
		pr_info("%s\n", IOCTL_MESSAGE);
	break;
	/* TODO 7: extra tasks, for home */	
	case MY_IOCTL_SET_BUFFER:
		len = min(strlen((char *) arg), (size_t) BUFFER_SIZE);

		if (copy_from_user(data->buffer, (char *) arg, len))
		{
			return -EFAULT;
		}

		// add '\0' character at the end of the message
		if (len < BUFSIZ)
		{
			data->buffer[len] = 0;
		}

	break;
	case MY_IOCTL_GET_BUFFER:
		len = min(strlen(data->buffer), (size_t) BUFFER_SIZE);
		
		if (copy_to_user((char *) arg, data->buffer, len))
		{
			return -EFAULT;
		}

	break;
	case MY_IOCTL_DOWN:
		// add process to a queue
		pr_info("(ioctl down) [pid:%d] adding {%d,%d} to queue\n", 
			current->pid, MAJOR(data->dev.dev), MINOR(data->dev.dev));
		
		snprintf(msg, 128, "(ioctl down) [pid:%d] before sleep", current->pid);
		print_wait_queue(&(data->wq), msg);

		data->is_free = 0;
		remains = wait_event_interruptible_timeout(data->wq, data->is_free != 0, 10 * HZ);

		if (remains > 1)
		{
			pr_info("(ioctl down) [pid:%d] {%d,%d} awaken before timeout (%d sec left)\n", 
				current->pid, MAJOR(data->dev.dev), MINOR(data->dev.dev), remains / HZ);
		} 
		else
		{
			pr_info("(ioctl down) [pid:%d] {%d,%d} awaken after timeout, condition %s\n", 
				current->pid, MAJOR(data->dev.dev), MINOR(data->dev.dev), (remains > 0 ? "true" : "false"));
		}

		snprintf(msg, 128, "(ioctl down) [pid:%d] after wake up", current->pid);
		print_wait_queue(&(data->wq), msg);

	break;
	case MY_IOCTL_UP:
		// remove process from a queue (?)
		//	- set flag to value != 0
		data->is_free = 1;
		// 	- wake up 'waiting' thread
		snprintf(msg, 128, "(ioctl up) [pid:%d] before wake up", current->pid);
		print_wait_queue(&(data->wq), msg);

		wake_up_interruptible(&(data->wq));

		snprintf(msg, 128, "(ioctl up) [pid:%d] after wake up", current->pid);
		print_wait_queue(&(data->wq), msg);		

	break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static const struct file_operations so2_fops = {
	.owner = THIS_MODULE,
/* TODO 2: add open and release functions */
	.open = so2_cdev_open,
	.release = so2_cdev_release,
/* TODO 4: add read function */
	.read = so2_cdev_read,
/* TODO 5: add write function */
	.write = so2_cdev_write,
/* TODO 6: add ioctl function */
	.unlocked_ioctl = so2_cdev_ioctl
};

static int so2_cdev_init(void)
{
	int err;
	int i;

	/* TODO 1: register char device region for MY_MAJOR and NUM_MINORS starting at MY_MINOR */
	err = register_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), NUM_MINORS, "so2 device");

	if(err)
	{			
		pr_err("error registering device: %d\n", err);
		return err;
	}

	pr_info("registered so2 device : %s\n", MESSAGE);

	for (i = 0; i < NUM_MINORS; i++) {
#ifdef EXTRA
		/* TODO 7: extra tasks, for home */
		memset(devs[i].buffer, 0, BUFSIZ);
		devs[i].buffer_size = strlen(devs[i].buffer);
#else
		/*TODO 4: initialize buffer with MESSAGE string */
		// to, from, count
		memcpy(devs[i].buffer, MESSAGE, strlen(MESSAGE));
		devs[i].buffer_size = strlen(MESSAGE);
#endif

		pr_info("dev buffer : %s\n", devs[i].buffer);

		/* TODO 7: extra tasks for home */
		// - initialize flag
		devs[i].is_free = 0;
		// - initialize queue (?)
		init_waitqueue_head(&(devs[i].wq));

		/* TODO 3: set access variable to 0, use atomic_set */
		atomic_set(&(devs[i].is_busy), 0);
		/* TODO 2: init and add cdev to kernel core */
		cdev_init(&devs[i].dev, &so2_fops);
		cdev_add(&devs[i].dev, MKDEV(MY_MAJOR, MY_MINOR + i), 1);
	}

	return 0;
}

static void so2_cdev_exit(void)
{
	int i;

	for (i = 0; i < NUM_MINORS; i++) {
		/* TODO 2: delete cdev from kernel core */
		cdev_del(&devs[i].dev);
	}

	/* TODO 1: unregister char device region, for MY_MAJOR and NUM_MINORS starting at MY_MINOR */
	unregister_chrdev_region(MKDEV(MY_MAJOR, MY_MINOR), NUM_MINORS);
	pr_info("unregisted so2 device\n");
}

module_init(so2_cdev_init);
module_exit(so2_cdev_exit);
