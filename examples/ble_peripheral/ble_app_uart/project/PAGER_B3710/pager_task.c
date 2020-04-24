#include <include/stdio.h>
#include <include/errno.h>
#include <include/gpio.h>
#include <include/delay.h>
#include <include/errno.h>
#include <include/lock.h>
#include <include/time.h>
#include <include/init.h>
#include <include/sched.h>
#include <include/task.h>
#include <include/msg.h>
#include <include/wait.h>
#include <include/platform.h>

#include "inc/pager_task.h"


int msure_check_cancel(struct task_struct *task)
{
	return 0;
}

int msure_check_timeout(struct task_struct *task, __u32 timeout)
{
	return 0;
}

int msure_check_status(struct task_struct *task, __u32 item)
{
	return 0;
}

