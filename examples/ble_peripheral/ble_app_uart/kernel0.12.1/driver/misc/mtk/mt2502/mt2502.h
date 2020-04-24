#pragma once

#include <include/types.h>

struct mt2502_status {
	__u8		curr_status;
    __u8		curr_use_count;     // if curr_use_count=0, mtk2502 will be shut down
    __u8		loadsw_use_cnt;
	kd_bool		lock;
	kd_bool		is_abnormal;
	kd_bool		is_usefirst;
};


enum MT2502_POWER {
	POWER_OFF,
	POWER_ON,
};

#define MT2502_TASK_COUNT	10


struct mt2502_task_msg_fmt {
	__u8 task_id;
	__u8 task_msg_comment[80];
};

struct msg_msg *mt2502_task_msg_check(struct task_struct *task);