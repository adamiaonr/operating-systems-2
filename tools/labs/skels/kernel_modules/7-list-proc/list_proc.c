#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
/* TODO: add missing headers */
#include <linux/sched.h> 		// task_struct
// #include <linux/sched/task.h>	// tasklist_lock
#include <linux/rwlock.h>		// read_lock()
#include <linux/sched/signal.h> // for_each_process()

MODULE_DESCRIPTION("List current processes");
MODULE_AUTHOR("Kernel Hacker");
MODULE_LICENSE("GPL");

static int my_proc_init(void)
{
	struct task_struct *p;

	/* TODO: print current process pid and its name */
	printk(KERN_DEBUG "[%d,%s]\n", current->pid, current->comm);

	/* TODO: print the pid and name of all processes */
	// read_lock(&tasklist_lock);
	rcu_read_lock();
	for_each_process(p)
	{
		printk(KERN_DEBUG "  [%d,%s]\n", p->pid, p->comm);
	}
	// read_unlock(&tasklist_lock);
	rcu_read_unlock();

	return 0;
}

static void my_proc_exit(void)
{
	/* TODO: print current process pid and name */
	printk(KERN_DEBUG "[%d,%s]\n", current->pid, current->comm);
}

module_init(my_proc_init);
module_exit(my_proc_exit);
