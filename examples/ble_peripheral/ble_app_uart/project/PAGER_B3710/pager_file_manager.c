#include <include/stdio.h>
#include <include/errno.h>
#include <include/delay.h>
#include <include/errno.h>
#include <include/lock.h>
#include <include/time.h>
#include <include/init.h>
#include <include/sched.h>
#include <include/task.h>
#include <include/msg.h>
#include <include/wait.h>

#include "inc/pager_file_manager.h"

#define FLOG(fmt, argc...)		printk("FLOG ", fmt, ##argc)

static struct file_manager_list fmlist[] = {
		{FIDX_MESURE_HISTORY, "/data/history.hex"},
		{FIDX_BLE_LOCAL_LOG, "/data/local.log"},
};
static int file_current_handle = 0;
static kd_bool file_handle_abnormal = kd_false, file_handle_endpacket = kd_false;
static KFILE *kfile = NULL;
static __u32 file_aready_sendsize = 0;

static unsigned char file_gethdl_thread(struct task_struct *task, void *argc)
{
	int ret, cto, rsz;
	struct stat kstatus = {0};
	__u8 fpacket[20];

TASK_GEN_STEP_ENTRY(0) {
		if (!file_current_handle)
			return TSLEEP;
		else if (file_handle_abnormal) {
			cto = task_wait_timeout(task, Second(30));
			if (!cto)
				return TSLEEP;
			task_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_STEP, TASK_STEP(80));
		}
		else {
			kfile = kfopen(fmlist[file_current_handle].fname, "ab+");
			if (!kfile) {
				file_handle_abnormal = kd_true;
				return TSLEEP;
			}
			ret = kfstat(&kstatus, kfile);
			FLOG("File: %s, size: %d\r\n", fmlist[file_current_handle].fname, kstatus.st_size);

			task_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP, 0);
		}
	}

TASK_GEN_STEP_ENTRY(1) {
		rsz = kfread(fpacket, 20, 0, kfile);
		if (rsz < 20)
			task_utils(task, TCFG_SET_NEXT_STEP, 0);
		ret = ble_send_packet(fpacket, sizeof(fpacket), Second(10));
		if (ret)
			task_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_STEP, TASK_STEP(80));
		else {
			file_aready_sendsize += 20;
			kflseek(file_aready_sendsize, SEEK_SET, kfile);
		}
		return TSLEEP;
	}

TASK_GEN_STEP_ENTRY(2) {
		
	}

TASK_GEN_STEP_ENTRY(80) {
		if (file_current_handle)
			kfclose(kfile);
		file_handle_abnormal = kd_false;
		file_current_handle = 0;
	}

	return TSLEEP;
}
TASK_INIT(CMD_FGET_HDL, file_gethdl_thread, kd_true, NULL, 
		EXEC_FALSE,
		SCHED_PRIORITY_MIDE);
static struct msg_user fmanager_msg_user = {.task_id = CMD_FGET_HDL, .task = TASK_INFO(file_gethdl_thread)};

int file_get_byfid(int fid)
{
	if (file_current_handle)
		return -EBUSY;
	if (file_current_handle >= FIDX_MAX)
		return -ERANGE;

	file_current_handle = fid;
	return 0;
}

int pager_file_manager_init(void)
{
	int ret;

	sched_task_create(TASK_INFO(file_gethdl_thread), SCHED_COMMON_TYPE, EXEC_TRUE);
	ret = msg_user_register(&fmanager_msg_user);

	return 0;
}
