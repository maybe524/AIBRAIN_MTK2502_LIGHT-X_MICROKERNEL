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
#define CONTENT_LEN	100

unsigned char rm(struct task_struct *task, void *argc)
{
	int opt_cnt, n;
	int ret;
	char filename[PATH_LEN];
	struct command_opts argv[ARGV_NUM];

TASK_GEN_STEP_ENTRY(0) {
		kmemset(filename, 0, PATH_LEN);
		kmemset(argv, 0, ARGV_NUM * sizeof(struct command_opts));

		opt_cnt = getopt(argc, argv, ARGV_NUM);
		if (!opt_cnt)
			goto L1;
		else {
			for (n = 0; n < opt_cnt; n++) {
				switch ((argv + n)->opt.len) {
				case 5:
					if (!kstrncmp((argv + n)->opt.str, "fname", 5))
						kmemcpy(filename, (argv + n)->val.str, (argv + n)->val.len);
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
		ret = remove(filename, 0);
		if (ret)
			printk("remove file fail(%d).\r\n", ret);
	} /* TASK_GEN_STEP_ENTRY 1 END */

	return 0;

L2:
	DPRINT("invalid option -- %c\r\n", (argv + n)->opt.str[0]);
	return -EINVAL;

L1:
	DPRINT("too few arguments\r\n");
	return -EINVAL;
}

EXPORT_CMD(rm,
		""
	);
