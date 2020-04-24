#include <include/msg.h>
#include <include/sched.h>
#include <include/errno.h>
#include <include/task.h>

struct msg_msg *task_check_msg(struct task_struct *task)
{
	if (task_check_msgl(task))
		return container_of(task->msg_list.next, struct msg_msg, msg_list);
	return NULL;
}

int task_clear_msg(struct task_struct *task)
{
	int ret;
	struct list_head *iter;
	
next_msg:
	list_for_each(iter, &task->msg_list) {
		list_del(iter);
		ret = msg_destroy(container_of(iter, struct msg_msg, msg_list));
		goto next_msg;
	}

	return 0;
}

int task_del_msg(struct task_struct *task, struct msg_msg *msg)
{
	struct list_head *iter;

	if (!task)
		return -EINVAL;

	list_del(&msg->msg_list);
	msg_destroy(msg);

	return 0;
}

int task_wait_timeout(struct task_struct *task, time_t timeout)
{
	if (!task->t_stime_temp)
		task->t_stime_temp = get_systime();

	if (get_systime() - task->t_stime_temp >= timeout)
		return -ETIMEDOUT;

	return 0;
}

int task_utils(struct task_struct *task, umode_t cfg, void *data)
{
	if (cfg & TCFG_CLEAR_TIMEOUT)
		task->t_stime_temp = 0;
	if (cfg & TCFG_SET_STEP)
		task->t_step = (__u32)data;
	if (cfg & TCFG_CLEAR_TRYCNT)
		task->t_trycnt = 0;
	if (cfg & TCFG_SET_NEXT_STEP)
		task->t_step++;
	if (cfg & TCFG_SET_NEXT_TRY)
		task->t_trycnt++;
	if (cfg & TCFG_CLEAR_MSG)
		task_clear_msg(task);

	return 0;
}
