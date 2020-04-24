#include "include/stdio.h"
#include "include/errno.h"
#include "include/string.h"
#include "include/unistd.h"
#include "include/types.h"
#include "include/shell.h"
#include "include/sched.h"


#define ARGV_NUM 3

unsigned char mklxtfs(struct task_struct *task, void *argc)
{
	int ret;
	int opt_cnt, n;
	struct command_opts argv[ARGV_NUM];
	kd_bool format_force = kd_false;

TASK_GEN_STEP_ENTRY(0) {
		kmemset(argv, 0, ARGV_NUM * sizeof(struct command_opts));

		opt_cnt = getopt(argc, argv, ARGV_NUM);
		for_each_opt(n, opt_cnt) {
			__u8 *ostr = (argv + n)->opt.str, olen = (argv + n)->opt.len;
			__u8 *vstr = (argv + n)->val.str, vlen = (argv + n)->val.len;
			switch (fixopt(ostr, olen)) {
				case 'f':
					format_force = kd_true;
					break;
			}
		}

		TASK_NEXT_STEP();
	}

TASK_GEN_STEP_ENTRY(1) {	
		if (nrfs_nand_check_busy())
			DPRINT("Nrfs nand is busy.\r\n");
		else
			nrfs_nand_check_format(format_force);
	}

	return 0;

few_argc:
	DPRINT("invalid option -- %c\r\n", (argv + n)->opt.str[0]);
	return -EINVAL;
}

EXPORT_CMD(mklxtfs,
		""
	);
