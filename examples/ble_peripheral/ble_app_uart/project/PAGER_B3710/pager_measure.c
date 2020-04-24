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

#include "inc/pager_task.h"
#include "inc/pager_time.h"
#include "inc/pager_measure.h"
#include "inc/pager_log.h"

#define	ON		kd_true
#define OFF		kd_false
#define msure_pwrset		mt2502_power_set
#define msure_pwrget		mt2502_power_get
#define msure_utils			task_utils
#define msure_msg_clear		mt2502_task_msg_clear
#define msure_msg_register 	mt2502_task_msg_register
#define msure_msg_check		mt2502_task_msg_check
#define msure_wait_timeout	task_wait_timeout
#define msure_getversion	mt2502_get_version

static struct mesure_info {
	__u8	curr_taskid;
	kd_bool msg2h_fail;
};

#define msure_atcmd(fmt, argc...)	do {	\
				__u8 at_buf[50] = {0};	\
				ksprintf(at_buf, "\r\n"fmt"\r\n", ##argc);	\
				kprintf(at_buf);\
			} while (0)
#define mesure_log(fmt, argc...)	printk("Mesure: " fmt, ##argc)

static __u8 mesure_d2h[20] = {0};
static struct mesure_info mesure_infos = {0};
static struct bhrr_msg_s bhrr_mesure_result = {0};

int mesure_get_result(int mesure_idx, void *data)
{
	if (mesure_idx == 'bhrr')
		kmemcpy(data, &bhrr_mesure_result, sizeof(struct bhrr_msg_s));

	return 0;
}

static int mesure_store_result(__u8 *buf, __u32 len)
{
	int ret;
	KFILE *kfile;

	kfile = kfopen("/data/history.hex", "ab+");
	if (!kfile) {
		ret = -EBUSY;
		goto store_end;
	}

	kfwrite(buf, len, 0, kfile);
	kfclose(kfile);

	return 0;

store_end:
	printk("store result fail(%d).\r\n", ret);
	return ret;
}

static unsigned char mesure_bhrr_thread(struct task_struct *task, void *argc)
{
	int ret = 0;
	int ccl, cst, cto, cwf = 0;
	struct msg_msg *msg;
	struct mesure_info *mesure = &mesure_infos;
	struct mesure_packet *mpacket = (struct mesure_packet *)mesure_d2h;;
	struct localtime local_time;
	__u8 temp_step;

TASK_GEN_STEP_ENTRY(0) {
		cst = msure_check_status(task,
			(TASK_CHECK_G_STANDBY | TASK_CHECK_BATTERY | TASK_CHECK_CHARING | TASK_CHECK_MSG_B2H));
		if (cst)
			return TSTOP;
		msure_pwrset(ON);
		msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP, 0);
	}

TASK_GEN_STEP_ENTRY(1) {
		cst = msure_check_status(task, TASK_CHECK_BATTERY | TASK_CHECK_MSG_B2H);
		if (cst) {
			msure_pwrset(OFF);
			return TSTOP;
		}
		else if (mesure->curr_taskid)
			return TSLEEP;
		else if (msure_pwrget() == TRUN)
			msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP, 0);
		else {
			msure_check_cancel(task);
			msure_check_timeout(task, Minute(1));
			return TSLEEP;
		}
	}

TASK_GEN_STEP_ENTRY(2) {
		mesure->curr_taskid = task->t_id;

		msure_msg_clear(task);
		msure_msg_register(task);

		msure_atcmd("AT+CMR_MODE=%d", 0);
		mdelay(5);
		msure_atcmd("AT+CMRR=%02X,%02X", 0x11, 0x11);

		msure_utils(task, TCFG_SET_NEXT_STEP, 0);
	}

TASK_GEN_STEP_ENTRY(3) {
		cto = msure_wait_timeout(task, Second(2));
		msg = msure_msg_check(task);
		cst = msure_check_status(task, TASK_CHECK_BATTERY | TASK_CHECK_MSG_B2H);
		if (msg) {
			if (!kstrncmp(&msg->msg_comment[3], "CMR:OK", 6)) {
				msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP | TCFG_CLEAR_MSG, 0);
				return TAGAIN;
			}
		}
		else if (cto) {

		}
		else if (cst) {
			msure_pwrset(OFF);
			msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_STEP, 0);
			return TSTOP;
		}
		return TSLEEP;
	}

TASK_GEN_STEP_ENTRY(4) {
		cto = msure_wait_timeout(task, Minute(2));
		msg = msure_msg_check(task);
		cst = msure_check_status(task, TASK_CHECK_BATTERY | TASK_CHECK_MSG_B2H);
		if (msg) {
			mpacket = (struct mesure_packet *)mesure_d2h;

			mesure_log("%s\r\n", msg->msg_comment);
			mpacket->idx = 0x03;
			mpacket->key = 0x04;
			mpacket->len = 0x0D;
			hex_str2val_bylen(&msg->msg_comment[32], &mpacket->comm.bhrr_msg.zone, 2);		// Zone
			hex_str2val_bylen(&msg->msg_comment[35], &mpacket->comm.bhrr_msg.timestamp, 8);		// Time
			hex_str2val_bylen(&msg->msg_comment[ 7], &mpacket->comm.bhrr_msg.h_bp, 4);		// High BP
			hex_str2val_bylen(&msg->msg_comment[12], &mpacket->comm.bhrr_msg.l_bp, 4);		// Low BP
			mpacket->comm.bhrr_msg.mesure_mode = task->t_param;
			mpacket->comm.bhrr_msg.person_status = 0;
			kmemcpy(&bhrr_mesure_result, &mpacket->comm.bhrr_msg, sizeof(struct bhrr_msg_s));
			ret = ui_set_result('XYCL', (void *)&bhrr_mesure_result);

			ret = ble_send_packet(mpacket, 20, Second(10));
			if (ret)
				mesure->msg2h_fail = kd_true;
			msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP | TCFG_CLEAR_MSG, 0);
		}
		else if (cst) {
			msure_pwrset(OFF);
			msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_CLEAR_MSG, 0);
			return TSTOP;
		}
		else
			return TSLEEP;	 
	}

TASK_GEN_STEP_ENTRY(5) {
		if (mesure->msg2h_fail)
			cwf = mesure_store_result(mesure_d2h, sizeof(mesure_d2h));
		if (cwf) {
			cto = msure_wait_timeout(task, Minute(1));
			return TSLEEP;
		}
		msure_utils(task, TCFG_SET_NEXT_STEP, 0);
	}

TASK_GEN_STEP_ENTRY(6) {
		msure_pwrset(OFF);
		kmemset(mesure, 0, sizeof(struct mesure_info));
	}

	return TSTOP;
}
TASK_INIT(CMD_BP_HR_RR_REQ, mesure_bhrr_thread, kd_true, NULL, 
		EXEC_FALSE,
		SCHED_PRIORITY_MIDE);

static unsigned char mesure_tird_thread(struct task_struct *task, void *argc)
{
	int ret, ccl, chk;
	struct msg_msg *msg;
	struct mesure_info *mesure = &mesure_infos;

TASK_GEN_STEP_ENTRY(0) {
		chk = msure_check_status(task,
			(TASK_CHECK_G_STANDBY | TASK_CHECK_BATTERY | TASK_CHECK_CHARING | TASK_CHECK_MSG_B2H));
		if (chk)
			return TSTOP;
		msure_pwrset(ON);
		msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP, 0);
	}

TASK_GEN_STEP_ENTRY(1) {
		chk = msure_check_status(task, TASK_CHECK_BATTERY | TASK_CHECK_MSG_B2H);
		if (chk) {
			msure_pwrset(OFF);
			return TSTOP;
		}
		else if (mesure->curr_taskid)
			return TSLEEP;
		else if (msure_pwrget() == TRUN)
			msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP, 0);
		else {
			msure_check_cancel(task);
			msure_check_timeout(task, Minute(1));
			return TSLEEP;
		}
	}

TASK_GEN_STEP_ENTRY(2) {
		ret = pps960_collect_grayraw(task, 1);
		if (ret)
			return TSLEEP;

		mesure->curr_taskid = task->t_id;
		msure_msg_clear(task);
		msure_msg_register(task);

		msure_atcmd("AT+MESURE_TIRED=%02X,%02X", 0x11, 0x11);
#if 0
		msure_utils(task, TCFG_SET_NEXT_STEP, 0);
#else
		msure_utils(task, TCFG_SET_STEP, 4);
#endif
	}

TASK_GEN_STEP_ENTRY(3) {
		ret = msure_wait_timeout(task, Second(2));
		msg = msure_msg_check(task);
		chk = msure_check_status(task, TASK_CHECK_BATTERY | TASK_CHECK_MSG_B2H);
		if (msg) {
			if (!kstrncmp(&msg->msg_comment[3], "ACK:OK", 6)) {
				msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP | TCFG_CLEAR_MSG, 0);
				return TAGAIN;
			}
		}
		else if (ret) {

		}
		else if (chk) {
			msure_pwrset(OFF);
			msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_STEP, 0);
			return TSTOP;
		}
		return TSLEEP;
	}

TASK_GEN_STEP_ENTRY(4) {
		ret = msure_wait_timeout(task, Minute(2));
		msg = msure_msg_check(task);
		chk = msure_check_status(task, TASK_CHECK_BATTERY | TASK_CHECK_MSG_B2H);
		if (msg) {
			switch (ENDING_SWAP32(*(__u32 *)&msg->msg_comment[3])) {
				// Raw data
				case 'RDT:':
					msure_atcmd("AT+SRDAT_TIRED=%02X,%s", 0, &msg->msg_comment[7]);
					msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_CLEAR_MSG, 0);
					return TSLEEP;
				// Raw data error
				case 'RDE:':
					break;
				// Return measure data
				case 'RET:':
					kmemset(mesure_d2h, 0, sizeof(mesure_d2h));
					kmemcpy(mesure_d2h, &msg->msg_comment[3], sizeof(mesure_d2h));					
					ret = ble_send_packet(mesure_d2h, sizeof(mesure_d2h), Second(10));
					break;
				default:
					break;
			}

			msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP | TCFG_CLEAR_MSG, 0);
		}
		else if (chk) {
			msure_pwrset(OFF);
			msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_CLEAR_MSG, 0);
			return TSTOP;
		}
		else
			return TSLEEP;	 
	}

TASK_GEN_STEP_ENTRY(5) {
		msure_pwrset(OFF);
	}

	return TSTOP;
}
TASK_INIT(CMD_GREEN_RAY_TIRED_REQ, mesure_tird_thread, kd_true, NULL, 
		EXEC_FALSE,
		SCHED_PRIORITY_MIDE);

static unsigned char mesure_sent_thread(struct task_struct *task, void *argc)
{
	int ret, ccl, chk;
	struct msg_msg *msg;

TASK_GEN_STEP_ENTRY(0) {
		chk = msure_check_status(task,
			(TASK_CHECK_G_STANDBY | TASK_CHECK_BATTERY | TASK_CHECK_CHARING | TASK_CHECK_MSG_B2H));
		if (chk)
			return TSTOP;
		msure_pwrset(ON);
		msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP, 0);
	}

TASK_GEN_STEP_ENTRY(1) {
		chk = msure_check_status(task, TASK_CHECK_BATTERY | TASK_CHECK_MSG_B2H);
		if (chk) {
			msure_pwrset(OFF);
			return TSTOP;
		}
		else if (msure_pwrget() == TRUN)
			msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP, 0);
		else {
			msure_check_cancel(task);
			msure_check_timeout(task, Minute(1));
			return TSLEEP;
		}
	}

TASK_GEN_STEP_ENTRY(2) {
		msure_msg_clear(task);
		msure_msg_register(task);

		msure_atcmd("AT+CMR_MODE=%d", 0);
		mdelay(5);
		msure_atcmd("AT+CMRR=%02X,%02X", 0x11, 0x11);

		msure_utils(task, TCFG_SET_NEXT_STEP, 0);
	}

TASK_GEN_STEP_ENTRY(3) {
		ret = msure_wait_timeout(task, Second(2));
		msg = msure_msg_check(task);
		chk = msure_check_status(task, TASK_CHECK_BATTERY | TASK_CHECK_MSG_B2H);
		if (msg) {
			if (!kstrncmp(&msg->msg_comment[3], "CMR:OK", 6)) {
				msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP | TCFG_CLEAR_MSG, 0);
				return TAGAIN;
			}
		}
		else if (ret) {

		}
		else if (chk) {
			msure_pwrset(OFF);
			msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_STEP, 0);
			return TSTOP;
		}
		return TSLEEP;
	}

TASK_GEN_STEP_ENTRY(4) {
		ret = msure_wait_timeout(task, Minute(2));
		msg = msure_msg_check(task);
		chk = msure_check_status(task, TASK_CHECK_BATTERY | TASK_CHECK_MSG_B2H);
		if (msg) {
			kmemset(mesure_d2h, 0, sizeof(mesure_d2h));
	
			if (kstrncmp(&msg->msg_comment[3], "CRT:", 4)) {
			}

			kmemcpy(mesure_d2h, &msg->msg_comment[3], 20);

			ret = ble_nus_string_send(get_nus_conn_handle(), mesure_d2h, sizeof(mesure_d2h));
			msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP | TCFG_CLEAR_MSG, 0);
		}
		else if (chk) {
			msure_pwrset(OFF);
			msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_CLEAR_MSG, 0);
			return TSTOP;
		}
		else
			return TSLEEP;	 
	}

TASK_GEN_STEP_ENTRY(5) {
		msure_pwrset(OFF);
	}

	return TSTOP;
}
TASK_INIT(CMD_GREEN_RAY_SENT_REQ, mesure_sent_thread, kd_true, NULL, 
		EXEC_FALSE,
		SCHED_PRIORITY_MIDE);

static unsigned char mesure_getversion_thread(struct task_struct *task, void *argc)
{
	int ret;
	__u8 version[20] = {0}, temp_step;

TASK_GEN_STEP_ENTRY(0) {
		ret = msure_getversion(&version[8]);
		if (!ret)
			temp_step = TASK_STEP(2);
		else {
			temp_step = task->t_step + 1;
			msure_pwrset(ON);
		}
		msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_STEP, temp_step);
	}

TASK_GEN_STEP_ENTRY(1) {
		if (msure_pwrget() == TRUN) {
			msure_pwrset(OFF);
			msure_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP, 0);
		}
		else {
			msure_check_timeout(task, Minute(1));
			return TSLEEP;
		}
	}

TASK_GEN_STEP_ENTRY(2) {
		app_getversion(&version[3]);
		msure_getversion(&version[14]);
		version[0] = 0x65;
		version[1] = 0x04;
		version[2] = 0x0E;
		ret = ble_send_packet(version, 20, Second(10));
		mesure_log("Version:%s, ret=%d\r\n", &version[3], ret);
		return TSTOP;
	}

	return TSTOP;
}
TASK_INIT(CMD_GET_DEVICE_INFO_REQ, mesure_getversion_thread, kd_true, NULL, 
		EXEC_FALSE,
		SCHED_PRIORITY_MIDE);

int mesure_start_up(const char *idex)
{
	int ret = 0;
	struct task_struct *task = NULL;

	switch (ENDING_SWAP32(*(__u32 *)idex)) {
		case 'bhrr':
			task = TASK_INFO(mesure_bhrr_thread);
			break;
		case 'tird':
			task = TASK_INFO(mesure_tird_thread);
			break;
		case 'sent':
			task = TASK_INFO(mesure_sent_thread);
			break;
		case 'imhr':
			break;
		case 'dyhr':
			break;
		case 'gver':
			task = TASK_INFO(mesure_getversion_thread);
			break;
		default:
			ret = -EINVAL;
			break;
	}

	if (task) {
		ret = sched_task_create(task, SCHED_COMMON_TYPE, EXEC_TRUE);
		logs(L3, "mesure startup@%s", "bhrr");
	}	
	mesure_log("start up \"%s\", ret = %d\r\n", idex, ret);

	return ret;
}
