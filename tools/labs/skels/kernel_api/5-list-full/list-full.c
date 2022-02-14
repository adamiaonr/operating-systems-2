/*
 * Kernel API lab
 * 
 * list-full.c: Working with lists (advanced)
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/sched/signal.h>

MODULE_DESCRIPTION("Full list processing");
MODULE_AUTHOR("SO2");
MODULE_LICENSE("GPL");

struct task_info {
	pid_t pid;
	unsigned long timestamp;
	atomic_t count;
	struct list_head list;
};

static struct list_head head;

static struct task_info *task_info_alloc(int pid)
{
	struct task_info *ti;

	ti = kmalloc(sizeof(*ti), GFP_KERNEL);
	if (ti == NULL)
		return NULL;
	ti->pid = pid;
	ti->timestamp = jiffies;
	atomic_set(&ti->count, 0);

	return ti;
}

static struct task_info *task_info_find_pid(int pid)
{
	struct list_head *p;
	struct task_info *ti;

	/* TODO 1: Look for pid and return task_info or NULL if not found */
	list_for_each(p, &head)
	{
		ti = list_entry(p, struct task_info, list);
		if (ti->pid == pid)
		{
			return ti;
		}
	}

	return NULL;
}

static void task_info_add_to_list(int pid)
{
	struct task_info *ti;

	ti = task_info_find_pid(pid);
	if (ti != NULL) {
		ti->timestamp = jiffies;
		atomic_inc(&ti->count);
		return;
	}

	ti = task_info_alloc(pid);
	list_add(&ti->list, &head);
}

static void task_info_add_for_current(void)
{
	task_info_add_to_list(current->pid);
	task_info_add_to_list(current->parent->pid);
	task_info_add_to_list(next_task(current)->pid);
	task_info_add_to_list(next_task(next_task(current))->pid);
}

static void task_info_print_list(const char *msg, bool show_time_diff)
{
	struct list_head *p;
	struct task_info *ti;
	unsigned long timestamp;

	pr_info("%s: [ ", msg);
	list_for_each(p, &head) {
		ti = list_entry(p, struct task_info, list);
		timestamp = (show_time_diff ? jiffies - ti->timestamp : ti->timestamp);
		pr_info("(%d, %lu, %d) ", ti->pid, timestamp, atomic_read(&(ti->count)));
	}
	pr_info("]\n");
}

static void task_info_remove_expired(void)
{
	struct list_head *p, *q;
	struct task_info *ti;

	list_for_each_safe(p, q, &head) {
		ti = list_entry(p, struct task_info, list);
		if (jiffies - ti->timestamp > 3 * HZ && atomic_read(&ti->count) < 5) {
			list_del(p);
			kfree(ti);
		}
	}
}

static void task_info_purge_list(void)
{
	struct list_head *p, *q;
	struct task_info *ti;

	list_for_each_safe(p, q, &head) {
		ti = list_entry(p, struct task_info, list);
		list_del(p);
		kfree(ti);
	}
}

static int list_full_init(void)
{
	INIT_LIST_HEAD(&head);

	task_info_add_for_current();
	task_info_print_list("after first add", false);

	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(5 * HZ);

	return 0;
}

static void list_full_exit(void)
{
	struct task_info *ti;

	/* TODO 2: Ensure that at least one task is not deleted */

	// tasks are removed if two conditions are satisfied :
	//	- timestamp older than 3 secs
	//	- count less than 5

	// update the 1st element timestamp, and the 2nd's count to 10
	ti = list_entry(head.next, struct task_info, list);
	ti->timestamp = jiffies;
	ti = list_entry(ti->list.next, struct task_info, list);
	atomic_set(&(ti->count),10);

	task_info_remove_expired();
	task_info_print_list("after removing expired", true);
	task_info_purge_list();
}

module_init(list_full_init);
module_exit(list_full_exit);