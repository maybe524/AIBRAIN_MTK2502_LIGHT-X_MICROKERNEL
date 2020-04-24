#include "include/stdio.h"
#include "include/errno.h"
#include "include/syscalls.h"
#include "include/shell.h"
#include "include/types.h"
#include "include/string.h"
#include "include/sched.h"
#include "include/task.h"
#include "include/malloc.h"

#define OPTS_NUM		3
#define PATH_LEN		15

struct mkdir_dat {
	kd_bool make_parent;	/* make parent directory as needed */
	kd_bool need_waiting_res;
};

static struct command_opts *opts = NULL, *opt_path = NULL;
static char *buff = NULL;
static struct task_struct task_mkdir = {0};
static struct mkdir_dat this_dat;

unsigned char mkdir(struct task_struct *task, void *argc)
{
	int ret = 0;
	char *cmd_opt = (char *)argc;
	char path[PATH_LEN];
	unsigned char n, opt_cnt, opt_len;
	struct command_opts argv[OPTS_NUM];

	if (!argc || IS_STR_END(cmd_opt))
		goto L1;

TASK_GEN_STEP_ENTRY(0) {
		kmemset(argv, 0, sizeof(struct command_opts) * OPTS_NUM);
		kmemset(&this_dat, 0, sizeof(struct mkdir_dat));

		opt_cnt = getopt(argc, argv, OPTS_NUM);
		if (!opt_cnt)
			goto L1;
		else {
			for (n = 0; n < opt_cnt; n++) {
				switch ((argv + n)->opt.len) {
				case 4:
					if (!kstrncmp((argv + n)->opt.str, "path", (argv + n)->opt.len))
						ksnprintf(path, (argv + n)->val.len + 1, "%s", (argv + n)->val.str);
					else
						goto L2;
					break;
				case 6:
					if (!kstrncmp((argv + n)->opt.str, "parent", (argv + n)->opt.len))
						this_dat.make_parent = kd_true;
					else
						goto L2;
					break;
				default:
					goto L2;
				}
			}
		}

		TASK_NEXT_STEP();
	} /* TASK_GEN_STEP_ENTRY 0 END */

TASK_GEN_STEP_ENTRY(1) {
		if (this_dat.need_waiting_res) {
			//task_mkdir = sched_task_new(0);
			//if (!task_mkdir)
				//goto L3;

			opt_len = kstrlen(cmd_opt);
			buff = (char *)kzalloc(opt_len, GFP_NORMAL);
			if (!buff)
				goto L4;
			//kmemcpy(buff, buff_tmp, opt_len);
			
			opts = (struct command_opts *)kzalloc(sizeof(struct command_opts) * opt_cnt, GFP_NORMAL);
			if (!opts) {
				kfree(buff);
				goto L4;
			}
			//kmemcpy(opts, opts_tmp, sizeof(struct command_opts) * opt_cnt);

			task_mkdir.t_id = CMD_MKDIR;
			task_mkdir.t_step = TASK_STEP(2);
			task_mkdir.t_execfg |= EXEC_TRUE;

			sched_task_create(&task_mkdir, SCHED_COMMON_TYPE, EXEC_TRUE);
			return TAGAIN;
		}

		TASK_NEXT_STEP();
	} /* TASK_GEN_STEP_ENTRY 1 END */

TASK_GEN_STEP_ENTRY(2) {
		if (this_dat.need_waiting_res)
			return TWAIT;

		TASK_NEXT_STEP();
	} /* TASK_GEN_STEP_ENTRY 2 END */

TASK_GEN_STEP_ENTRY(3) {
		ret = sys_mkdir(path, 0755);
		if (ret < 0)
			DPRINT("cannot create directory `%s'(%d)!!!\r\n", path, ret);

		if (task) {
			//sched_task_del(task_mkdir);
			//task_mkdir = NULL;
		}
		//kfree(buff);
		//kfree(opts);

		TASK_SET_STEP(0);
	} /* TASK_GEN_STEP_ENTRY 3 END */

	return TSTOP;


L4:
	//sched_task_del(task_mkdir);
	return TSTOP;

L3:
	DPRINT("no memery\r\n");
	return TSTOP;

L2:
	DPRINT("invalid option -- %c\r\n", (argv + n)->opt.str[0]);
	return TSTOP;

L1:
	DPRINT("too few arguments\r\n");
	return TSTOP;
}

EXPORT_CMD(mkdir,
		""
	);
