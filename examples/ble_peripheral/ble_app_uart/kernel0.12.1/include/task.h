#pragma once

#include "include/types.h"
#include "include/init.h"
#include "include/task.h"

#define __GSECT_EXE__       __attribute__((section(".gsect_exe")))
#define __GSECT_HELP__      __attribute__((section(".gsect_help")))

#define MAX_ARG_LEN         512
#define MAX_ARGC            64

// command
struct command {
	const char *name;
	int (*main)(int argc, char *argv[]);
};

#define REGISTER_EXECUTIVE(id, entry) \
	static const __USED__ __GSECT_EXE__ struct command __g_exe_##id##__ = { \
		.name = #id, \
		.main = entry, \
	}

// help
struct option {
	const char *opt;
	const char *desc;
};

struct help_info {
	const char *name;
	const char *desc;
	int level;
	int count;
	union {
		const struct help_info *cmdv;
		const struct option *optv;
		const void *list;
	} u;
};

#define REGISTER_HELP_INFO(n, d, l, v) \
	static const __USED__ __GSECT_HELP__ struct help_info __g_help_##n##__ = { \
		.name  = #n, \
		.desc  = d, \
		.level = l, \
		.count = ARRAY_ELEM_NUM(v), \
		.u.list = v, \
	}

#define REGISTER_HELP_L1(n, d, v) REGISTER_HELP_INFO(n, d, 1, v)
#define REGISTER_HELP_L2(n, d, v) REGISTER_HELP_INFO(n, d, 2, v)

// run-time task
struct task {
	int argc;
	char **argv;
	const struct command *exe;
	const struct help_info *help;
};

struct task *get_current_task(void);
void set_current_task(struct task *);
int task_clear_msg(struct task_struct *task);

enum task_id {
	CMD_NONE = 0x00,
	CMD_POWERON_REQ		=  0x01,
	CMD_BP_HR_RR_REQ		= 0x02,
	CMD_QUERYSTATE_REQ	= 0x03,
	CMD_PULSESENSOR_REQ		= 0x04,
	CMD_GREEN_RAY_HRT_REQ		= 0x05,
	CMD_GREEN_RAY_SENT_REQ	= 0x06,
	CMD_GREEN_RAY_TIRED_REQ	= 0x07,
	CMD_GET_DEVICE_INFO_REQ	= 0x08,
	CMD_IHR	= 0x09,
	CMD_GREEN_SENT_TIRED	= 0x0B,
	CMD_DHR = 0x0C,
	CMD_MT2502_LSW = 0x0D,

	CMD_M2B_BIGDATA_TRANSFER			= 0x1E,
	CMD_M2B_BIGDATA_TRANSFER_BEGIN	= 0x0E,
	CMD_M2B_BIGDATA_TRANSFER_END		= 0x0F,
	CMD_FACTORY_CMD	= 0xF0,

	CMD_B2M_BIGDATA_TRANSFER	= 0x1F,
	CMD_BLE_CONN_DEAL_WITH		= 0x11,

	CMD_MT2502_POWERON	= 0x21,
	CMD_MT2502_POWEROFF	= 0x22,
	CMD_TRIGGER_DEDECT		= 0x23,
	CMD_MT2502_MSG,
	CMD_ERROR_MESSAGE		= 0xEE,	/* B2H/M2B general massage */

	CMD_GREEN_RAY_LED_REQ		= 0x24,
	CMD_MESSAGE_MT2502_2_BLE	= 0x25,
	CMD_GET_SN_NUMBER_REQ	= 0x26,

	CMD_IO_CONTRL	= 0x30,
	CMD_USER_INFO	= 0x31,
	CMD_AGING_TEST	= 0x32,
	CMD_SHELL	= 0x40,
	CMD_MKDIR,
	CMD_FLASH,
	CMD_BUFFER_HEAD,
	CMD_PPS960_ISR,

	CMD_UART,
	CMD_BUTTON_PUSH,
	CMD_BUTTON_RELEASE,
	CMD_BUTTON,
	CMD_SYNC_FSYS,
	CMD_LXT_THREAD,
	CMD_FB_SYNC,

	CMD_DMP,
	CMD_CHARGER,
	CMD_ALARM,
	CMD_UI,
	CMD_FGET_HDL,
	CMD_SURFACEFLINGER,
};


#define TCFG_CLEAR_TIMEOUT		(1 << 0)
#define TCFG_SET_STEP			(1 << 1)
#define TCFG_SET_NEXT_STEP		(1 << 2)
#define TCFG_CLEAR_TRYCNT		(1 << 3)
#define TCFG_SET_NEXT_TRY		(1 << 4)
#define TCFG_CLEAR_MSG			(1 << 5)

struct msg_msg *task_check_msg2(struct task_struct *task);
