#include "include/stdio.h"
#include "include/errno.h"
#include "include/fs.h"
#include "include/fcntl.h"
#include "include/shell.h"
#include "include/sched.h"
#include "include/malloc.h"
#include "include/getopt.h"

#define ARGV_NUM	3
#define NAME_LEN	20

struct mount_info {
	char dev_name[NAME_LEN];
	char mount_point[10];
};

static void usage(void)
{
	IMPORT_CMD(mountfs);
	DPRINT("%s\r\n", mountfs_info.help);
}

unsigned char mountfs(struct task_struct *task, void *argc)
{
	int ret;
	struct command_opts argv[ARGV_NUM];
	int n, opt_cnt;
	char type[10];
	struct mount_info *minfo, mi;

TASK_GEN_STEP_ENTRY(0) {
		opt_cnt = getopt(argc, argv, ARGV_NUM);
		if (!opt_cnt)
			goto L3;
		for_each_opt(n, opt_cnt) {
			char *ostr = (argv + n)->opt.str, olen = (argv + n)->opt.len;
			char *vstr = (argv + n)->val.str, vlen = (argv + n)->val.len;
			switch (fixopt(ostr, olen)) {
				case 'type':
					ksnprintf(type, vlen + 1, "%s", vstr);
					break;
				case 'dev':
					ksnprintf(mi.dev_name, vlen + 1, "%s", vstr);
					break;
				case 'poin':
					ksnprintf(mi.mount_point, vlen + 1, "%s", vstr);
					break;
				case 'help':
					usage();
					goto normal_end;
				default:
					goto unknown_opt;
					break;
			}
		}
	}

TASK_GEN_STEP_ENTRY(1) {
		minfo = kmalloc(sizeof(*minfo), GFP_NORMAL);
		if (!minfo)
			goto L1;
		kmemcpy(minfo, &mi, sizeof(*minfo));

		ret = sys_mount(minfo->dev_name, minfo->mount_point, type, 0);
		if (ret < 0)
			goto L2;
		else
			kprintf("mount success.\r\n");

		return ret;
	}

unknown_opt:
	printk("invalid option -- %c\r\n", (argv + n)->opt.str[0]);
	return -EINVAL;

L3:
	list_mount();	
	return ret;

L2:
	kfree(minfo);
L1:
	printk("fail to mount %s (ret = %d)\r\n", mi.dev_name, ret);
	return ret;

normal_end:
	return 0;
}

EXPORT_CMD(mountfs,
		""
	);
