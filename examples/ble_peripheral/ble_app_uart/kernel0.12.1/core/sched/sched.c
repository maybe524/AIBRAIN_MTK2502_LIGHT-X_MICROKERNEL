#include <include/errno.h>
#include <include/task.h>
#include <include/sched.h>
#include <include/lock.h>
#include <include/wait.h>
#include "include/arm/cpu.h"

LIST_HEAD(__kd_sched_list);
struct task_struct *current = NULL;

void schedule(void)
{
	struct list_head *iter;

	list_for_each(iter, &__kd_sched_list) {
		struct task_sched *scheder = container_of(iter, struct task_sched, list);
		if (scheder)
			scheder->sched(&current);
	}
}

int sched_task_create(struct task_struct *task, __u8 type, __u16 config)
{
	int ret = -EINVAL;
	struct list_head *iter;
	unsigned char __UNUSED__ psr;

	if (!task)
		return ret;

	lock_irq_psr(psr);

	list_for_each(iter, &__kd_sched_list) {
		struct task_sched *scheder = container_of(iter, struct task_sched, list);
		if (scheder->type == type) {
			ret = scheder->sched_put(task);
			if (!ret) {
				task->scheder = scheder;
				ret = sched_task_set(task, config);
			}
			break;
		}
	}

	unlock_irq_psr(psr);

	return ret;
}

int sched_task_set(struct task_struct *task, __u16 config)
{
	int ret;
	unsigned char __UNUSED__ psr;

	lock_irq_psr(psr);

	if (task && task->scheder)
		ret = task->scheder->sched_set(task, config);

	unlock_irq_psr(psr);

	return ret;
}

struct list_head *sched_task_del(struct task_struct *task)
{
	struct list_head *iter;
	unsigned char __UNUSED__ psr;

	if (!task)
		return NULL;

	lock_irq_psr(psr);

	iter = task->task_list.prev;
	list_del(&task->task_list);

	unlock_irq_psr(psr);

	return iter;
}

int sleep_on(struct task_struct *task)
{
	if (task) {
		if (task->t_state == TAGAIN)
			return -EINVAL;
		else {
			task->t_execfg |= EXEC_INTERRUPTIBLE;
			return 0;
		}
	}
	schedule();

	return 0;
}


int wake_up(struct task_struct *task)
{
	if (!task)
		return -EINVAL;
	return sched_task_set(task, task->t_execfg & ~EXEC_INTERRUPTIBLE);
}

int sched_register(struct task_sched *scheder)
{
	struct list_head *iter;

	if (!scheder || !scheder->sched || !scheder->sched_put || \
			!scheder->sched_set)
		return -EINVAL;

	list_for_each(iter, &__kd_sched_list) {
		struct task_sched *sched = container_of(iter, struct task_sched, list);
		if (sched->type == scheder->type || \
				iter == &scheder->list)
			return -EBUSY;
	}

	list_add(&scheder->list, &__kd_sched_list);

	return 0;
}

int __init sched_init(void)
{
	sched_common_init();
	return 0;
}
