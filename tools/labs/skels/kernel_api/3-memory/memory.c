/*
 * SO2 lab3 - task 3
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>

MODULE_DESCRIPTION("Memory processing");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

struct task_info {
	pid_t pid;
	unsigned long timestamp;
};

static struct task_info *ti1, *ti2, *ti3, *ti4, *ti5;

static struct task_info *task_info_alloc(int pid)
{
	struct task_info *ti;

	/* TODO 1: allocated and initialize a task_info struct */
	
	// kmalloc() task_info
	ti = kmalloc(sizeof *ti, GFP_KERNEL);
	if (ti == NULL)
		return NULL;

	// initialize it : give it pid and timestamp
	ti->pid = pid;
	ti->timestamp = jiffies / HZ; // there are HZ jiffies in a second

	return ti;
}

static int memory_init(void)
{
	struct task_struct *next_process;

	/* TODO 2: call task_info_alloc for current pid */
	ti1 = task_info_alloc(current->pid);

	/* TODO 2: call task_info_alloc for parent PID */
	ti2 = task_info_alloc(current->parent->pid);

	/* TODO 2: call task_info alloc for next process PID */
	next_process = next_task(current);
	ti3 = task_info_alloc(next_process->pid);

	/* TODO 2: call task_info_alloc for next process of the next process */
	ti4 = task_info_alloc(next_task(next_process)->pid);

	ti5 = task_info_alloc(current->real_parent->pid);

	return 0;
}

static void memory_exit(void)
{

	/* TODO 3: print ti* field values */
	pr_info("ti1 : %d, %ld\n", ti1->pid, ti1->timestamp);
	pr_info("ti2 : %d (%d), %ld\n", ti2->pid, ti5->pid, ti2->timestamp);
	pr_info("ti3 : %d, %ld\n", ti3->pid, ti3->timestamp);
	pr_info("ti4 : %d, %ld\n", ti4->pid, ti4->timestamp);

	/* TODO 4: free ti* structures */
	kfree(ti1);
	kfree(ti2);
	kfree(ti3);
	kfree(ti4);
	kfree(ti5);
}

module_init(memory_init);
module_exit(memory_exit);
