#include <include/task.h>
#include <include/stdio.h>
#include <include/errno.h>
#include <include/sched.h>
#include <include/lock.h>

#include "./inc/pager_time.h"

#define list_alarm(n)		for ((n) = 0; (n) < ARRAY_ELEM_NUM(alarm_trghdls); (n)++)

struct alarm_trigger {
	__u8  step;
	__u32 time_already_done;
	int (*trigger_handler)(void *data);
	void *private;
};

static int alarm_vibrator(void *data)
{
	struct alarm_trigger *alarm_data;
	struct localtime *local_time;

	alarm_data = (struct alarm_trigger *)data;
	local_time = (struct localtime *)alarm_data->private;

	if (local_time->Hour == 7 && local_time->Minute == 0 && local_time->Second == 0)
		shell("factory -vibr");

	return 0;
}


struct alarm_trigger alarm_trghdls[] = 
{
	{.trigger_handler = alarm_vibrator},
};


static unsigned char __sched
	alarm_thread(struct task_struct *task, void *data)
{
	int idx, ret;
	struct localtime local_time;

	if (check_lock(&task->lock))
		return -EBUSY;
	lock(&task->lock);

	ret = get_localtime(&local_time);
	if (ret < 0)
		goto end;
	list_alarm(idx) {
		struct alarm_trigger *alarm_iter = &alarm_trghdls[idx];
		alarm_iter->private = &local_time;

		ret = alarm_iter->trigger_handler(alarm_iter);
	}

end:
	unlock(&task->lock);
	return TSLEEP;
}
TASK_INIT(CMD_ALARM, alarm_thread, kd_false, NULL, 
		EXEC_TRUE,
		SCHED_PRIORITY_HIGH);

int pager_alarm_init(void)
{
	sched_task_create(TASK_INFO(alarm_thread), SCHED_COMMON_TYPE, EXEC_TRUE);
	return 0;
}
