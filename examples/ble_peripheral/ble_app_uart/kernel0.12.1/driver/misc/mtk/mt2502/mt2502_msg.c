#include <include/stdio.h>
#include <include/errno.h>
#include <include/errno.h>
#include <include/time.h>
#include <include/sched.h>
#include <include/task.h>
#include <include/msg.h>
#include <include/wait.h>

#include "mt2502.h"

struct msg_msg *task_check_msg(struct task_struct *task);

#define list_mt2502_task_msg(v)	\
			for ((v) = 0; (v) < MT2502_TASK_COUNT; (v)++)

static struct task_struct *mt2502_task_msg_list[MT2502_TASK_COUNT] = {NULL};

struct msg_msg *mt2502_task_msg_check(struct task_struct *task)
{
	return task_check_msg(task);
}

int mt2502_task_msg_register(struct task_struct *task)
{
	int idx, idle = -1;

	list_mt2502_task_msg(idx) {
		if (mt2502_task_msg_list[idx] == task)
			return -EBUSY;
		if (idle < 0 && !mt2502_task_msg_list[idx])
			idle = idx;
	}
	if (idle < 0)
		return -ENOMEM;
	mt2502_task_msg_list[idle] = task;

	return 0;
}

int mt2502_task_msg_clear(struct task_struct *task)
{
	return task_clear_msg(task);
}

static unsigned char mt2502_task_msg_thread(struct task_struct *task, void *argc)
{
	int idx;
	struct list_head *iter;

next_msg:
	list_for_each(iter, &task->msg_list) {
		struct msg_msg *msg = container_of(iter, struct msg_msg, msg_list);

		list_mt2502_task_msg(idx) {
			struct mt2502_task_msg_fmt *mt2502_msg = (struct mt2502_task_msg_fmt *)(&msg->msg_comment[2]);
			struct task_struct *task_tmp = mt2502_task_msg_list[idx];

			if (task_tmp && task_tmp->t_id == mt2502_msg->task_id) {
				list_del(iter);
				list_add(iter, &task_tmp->msg_list);
				wake_up(task_tmp);
				goto next_msg;
			}

			if (idx == MT2502_TASK_COUNT - 1)
				list_del(iter);
		}
	}
	//sleep_on(task);

	return TSLEEP;
}
TASK_INIT(CMD_MT2502_MSG, mt2502_task_msg_thread, kd_true, NULL, 
		EXEC_TRUE,
		SCHED_PRIORITY_MIDE);
static struct msg_user mt2502_msg_user = {.task_id = CMD_MT2502_MSG, .task = TASK_INFO(mt2502_task_msg_thread)};

int mt2502_msg_init(void)
{
	int ret;

	ret = sched_task_create(TASK_INFO(mt2502_task_msg_thread),
					SCHED_COMMON_TYPE,
					EXEC_TRUE);
	ret = wait_queue_put(WAIT_QUEUE_UART, TASK_INFO(mt2502_task_msg_thread));
	ret = msg_user_register(&mt2502_msg_user);

	return ret;
}
