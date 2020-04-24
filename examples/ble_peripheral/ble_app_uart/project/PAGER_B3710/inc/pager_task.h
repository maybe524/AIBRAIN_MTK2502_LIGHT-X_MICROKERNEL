#pragma once

#define TASK_CHECK_BATTERY					(1 << 0)
#define TASK_CHECK_CHARING					(1 << 1)
#define TASK_CHECK_MSG_B2H					(1 << 2)		/* if low battery, tellto apk, stop current task */
#define TASK_CHECK_G_STANDBY				(1 << 3)

extern struct msg_msg *mt2502_task_msg_check(struct task_struct *task);
