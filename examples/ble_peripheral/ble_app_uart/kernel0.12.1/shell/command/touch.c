#include "include/stdio.h"
#include "include/errno.h"
#include "include/string.h"
#include "include/unistd.h"
#include "include/fcntl.h"
#include "include/dirent.h"
#include "include/block.h"
#include "include/types.h"
#include "include/shell.h"
#include "include/fs.h"
#include "include/sched.h"


#define ARGV_NUM 3
#define PATH_LEN 15

unsigned char touch(struct task_struct *task, void *argc)
{
	int n, opt_cnt;
	int ret;
	char path[PATH_LEN], mode[8];
	struct command_opts argv[ARGV_NUM];

TASK_GEN_STEP_ENTRY(0) {
		kmemset(path, 0, PATH_LEN);
		kmemset(argv, 0, ARGV_NUM * sizeof(struct command_opts));

		opt_cnt = getopt(argc, argv, ARGV_NUM);
		if (!opt_cnt)
			goto few_argc;
		for_each_opt(n, opt_cnt) {
			__u8 *ostr = (argv + n)->opt.str, olen = (argv + n)->opt.len;
			__u8 *vstr = (argv + n)->val.str, vlen = (argv + n)->val.len;
			switch (fixopt(ostr, olen)) {
				case 'path':
					kmemcpy(path, vstr, vlen);
					break;
				case 'mode':
					kmemcpy(mode, vstr, vlen);
					break;
				default:
					break;
			}
		}

		TASK_NEXT_STEP();
	}

TASK_GEN_STEP_ENTRY(1) {
		if (!path[0])
			kmemcpy(path, "./", 2);
		ret = sys_create(path, 0);
		if (ret < 0)
			kprintf("create file fail(%d)\r\n", ret);
	}

	return 0;

L2:
	DPRINT("invalid option -- %c\r\n", (argv + n)->opt.str[0]);
	return -EINVAL;

few_argc:
	DPRINT("too few arguments\r\n");
	return -EINVAL;
}

EXPORT_CMD(touch,
		""
	);
