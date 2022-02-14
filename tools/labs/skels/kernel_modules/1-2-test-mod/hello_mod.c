#include <linux/fs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

MODULE_DESCRIPTION("Simple module");
MODULE_AUTHOR("Kernel Hacker");
MODULE_LICENSE("GPL");

static ssize_t hello_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset);

static const struct file_operations hello_ops = 
{
    .owner          = THIS_MODULE,
    .read           = hello_read
};

static int my_hello_init(void)
{
	pr_debug("Hello!\n");
	return 0;
}

static void hello_exit(void)
{
	pr_debug("Goodbye!\n");
}

static ssize_t hello_read(struct file *file, char __user *user_buffer, size_t size, loff_t *offset)
{
	printk(KERN_DEBUG "Reading\n");

	return size;
}

module_init(my_hello_init);
module_exit(hello_exit);
