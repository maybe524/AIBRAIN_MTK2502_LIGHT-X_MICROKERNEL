#include <include/stdio.h>
#include <include/errno.h>
#include <include/delay.h>
#include <include/errno.h>
#include <include/lock.h>
#include <include/time.h>
#include <include/sched.h>
#include <include/task.h>
#include <include/msg.h>
#include <include/platform.h>

kd_bool dmp_need_suspend = kd_false;
kd_bool dmp_need_resume = kd_false;

static unsigned char dpm_prev_thread(struct task_struct *task, void *argc)
{
TASK_GEN_STEP_ENTRY(0) {
		if (dmp_need_suspend) {
			enter_suspend();
			dmp_need_suspend = kd_false;
		}
		else if (dmp_need_resume) {
			enter_resume();
			dmp_need_resume = kd_false;
		}
	}

	return TSTOP;
}
TASK_INIT(CMD_DMP, dpm_prev_thread, kd_true, NULL, 
		EXEC_FALSE,
		SCHED_PRIORITY_MIDE);
