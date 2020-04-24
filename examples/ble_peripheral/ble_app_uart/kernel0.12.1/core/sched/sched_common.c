#include <include/errno.h>
#include <include/task.h>
#include <include/sched.h>
#include <include/lock.h>

#define SCHED_ENTER_CNT		10

LIST_HEAD(__sched_common_list);
static __u8 sched_enter_depth = 0;
extern kd_bool system_inited;

static int ___sched_common(struct task_struct **current)
{
	__u8 ret;
	static struct list_head *iter = &__sched_common_list;

	while (kd_true) {
		static struct task_struct *task;

		iter = iter->next;
		if (iter == &__sched_common_list)
			break;
		task = container_of(iter, struct task_struct, task_list);
		*current = task;

sched_again:
		if (task_check_exec(task)) {
			if (sleep_interruptible(task))
				ret = TSLEEP;
			else
				ret = task->task_handler(task, task->data ? task->data : NULL);
			switch (ret) {
				case TSTOP:
				case TDEAD:
					task->t_step   = 0;
					task->t_execfg = EXEC_FALSE;
					task->t_state  = TSTOP;
					task->t_param  = 0;
					task->t_trycnt = 0;
					task->t_stime_temp = 0;
					if (likely_dynamic(task))
						iter = sched_task_del(task);
					break;
				case TSLEEP:
					task->t_execfg |= EXEC_TRUE;
					task->t_state = TSLEEP;
					break;
				case TAGAIN:
					task->t_execfg |= EXEC_TRUE;
					task->t_state = TAGAIN;
				default:
					break;
			}
		}
	}

	return 0;
}

static __sched int sched_common(struct task_struct **current)
{
	int ret;

	if (!system_inited)
		return -EINVAL;

	if (sched_enter_depth > SCHED_ENTER_CNT)
		return -EBUSY;
	sched_enter_depth++;
	ret = ___sched_common(current);
	sched_enter_depth--;

	return ret;
}

static int sched_common_put(struct task_struct *task)
{
	struct list_head *iter, *lowp = NULL;

	list_for_each_reverse(iter, &__sched_common_list) {
		struct task_struct *temp = container_of(iter, struct task_struct, task_list);
		if (temp == task || \
				temp->t_id == task->t_id)
			return -EBUSY;
		if (!lowp && task->t_priority >= temp->t_priority)
			lowp = iter;
	}
	list_add(&task->task_list, lowp ? lowp : &__sched_common_list);

	return 0;
}

static int sched_common_set(struct task_struct *task, __u16 config)
{
	task->t_execfg = config;

	return 0;
}

static struct task_sched task_sched_mutex = {
	.type = SCHED_COMMON_TYPE,
	.sched = sched_common,
	.sched_put = sched_common_put,
	.sched_set = sched_common_set,
};

int __init sched_common_init(void)
{
	return sched_register(&task_sched_mutex);
}

