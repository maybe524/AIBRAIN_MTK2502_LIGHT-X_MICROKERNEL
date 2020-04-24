#include "include/stdio.h"
#include "include/errno.h"
#include "include/string.h"
#include "include/shell.h"
#include "include/getopt.h"
#include "include/sched.h"
#include "include/task.h"
#include "include/lock.h"
#include "include/time.h"

#define ARGV_NUM		5

struct sync_info {
	kd_bool is_need_sync;
	kd_bool lock;
};

struct sync_info sync_infos = {0};

int sync_request(void);

unsigned char sync_fsys_thread(struct task_struct *task, void *argc)
{
	int ret;
	struct sync_info *infos = &sync_infos;

TASK_GEN_STEP_ENTRY(0) {
		if (!infos->is_need_sync)
			return TSLEEP;
		lock(&infos->lock);
		sync_request();
		task_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP, 0);
	}

TASK_GEN_STEP_ENTRY(1) {
		ret = task_wait_timeout(task, Second(30));
		if (ret)
			goto sync_timeout;
		ret = sync_status();
		if (ret)
			return TSLEEP;

		unlock(&infos->lock);
		goto sync_end;
	}

sync_timeout:
	kprintf("sync timeout! 0x%x-0x%x\r\n", get_timestamp(), task->t_stime_temp);
	return TSTOP;

sync_end:
	kprintf("sync done!\r\n");
	return TSTOP;
}
TASK_INIT(CMD_SYNC_FSYS, sync_fsys_thread, kd_true, NULL, 
		EXEC_TRUE,
		SCHED_PRIORITY_MIDE);

unsigned char sync(struct task_struct *task, void *argc)
{
	int n, opt_cnt;
	struct sync_info *infos = &sync_infos;
	struct command_opts argv[ARGV_NUM] = {0};

	opt_cnt = getopt((char *)argc, argv, ARGV_NUM);
	if (!opt_cnt)
		goto normal_end;
	for_each_opt(n, opt_cnt) {
		__u8 *ostr = (argv + n)->opt.str, olen = (argv + n)->opt.len;
		__u8 *vstr = (argv + n)->val.str, vlen = (argv + n)->val.len;
		switch (fixopt(ostr, olen)) {
			case 'fsys':
				if (check_lock(&infos->lock)) {
					kprintf("sync is busy!!!\r\n");
					return EBUSY;
				}
				infos->is_need_sync = kd_true;
				sched_task_create(TASK_INFO(sync_fsys_thread), SCHED_COMMON_TYPE, EXEC_TRUE);
				break;
			default:
				break;
		}
	}

normal_end:
	return 0;
}

EXPORT_CMD(sync,
		"sync file system\r\n"
	);
