#include "include/stdio.h"
#include "include/errno.h"
#include "include/string.h"
#include "include/types.h"
#include "include/shell.h"
#include "include/fs.h"
#include "include/sched.h"
#include "include/fs/ltxfs.h"
#include "include/fcntl.h"
#include "include/getopt.h"

#define ARGV_NUM 3
#define CONTENT_LEN	20

#define DB_SET 1
#define DB_GET 2

struct xdb_dat {
	char name[CONTENT_LEN], val[CONTENT_LEN];
};

unsigned char xdb(struct task_struct *task, void *argc)
{
	int opt_cnt, ret, n;
	char buff[20];
	struct command_opts argv[ARGV_NUM];
	int fd;
	struct ltx_qstr lxdb;
	struct xdb_dat lxdb_data;
	char db_opt = 0;
	struct file *fp;

TASK_GEN_STEP_ENTRY(0) {
		kmemset(argv, 0, ARGV_NUM * sizeof(struct command_opts));
		kmemset(&lxdb_data, 0, sizeof(struct xdb_dat));
		kmemset(&lxdb, 0, sizeof(struct ltx_qstr));
		lxdb.val = lxdb_data.val;
		lxdb.name = lxdb_data.name;

		opt_cnt = getopt(argc, argv, ARGV_NUM);
		if (!opt_cnt)
			goto few_argc;
		for_each_opt(n, opt_cnt) {
			__u8 *ostr = (argv + n)->opt.str, olen = (argv + n)->opt.len;
			__u8 *vstr = (argv + n)->val.str, vlen = (argv + n)->val.len;
			switch (fixopt(ostr, olen)) {
				case 'name':
					kmemcpy(lxdb.name, vstr, vlen);
					lxdb.nlen = vlen;
					break;
				case 'val':
					kmemcpy(lxdb.val, vstr, vlen);
					lxdb.vlen = vlen;
					break;
				case 'set':
					db_opt = DB_SET;
					break;
				case 'get':
					db_opt = DB_GET;
					break;
				case 'del':
					break;
				case 'rst':
					break;
				case 'pfmt':
					break;
				default:
					break;
			}
		}

		TASK_NEXT_STEP();
	}

TASK_GEN_STEP_ENTRY(1) {
		if (!db_opt)
			goto few_argc;

		fd = open("/data/sys.dbase", O_RDONLY);
		if (fd < 0)
			return fd;

		switch (db_opt) {
			case DB_SET:
				write(fd, (void *)&lxdb, 0);
			break;
			case DB_GET:
				kmemset(lxdb_data.val, 0, CONTENT_LEN);
				read(fd, (void *)&lxdb, 0);
			break;
		}

		kprintf("$%s=%s\r\n", lxdb.name, lxdb.val);
		close(fd);
	}

	return 0;

L2:
	DPRINT("invalid option -- %c\r\n", (argv + n)->opt.str[0]);
	return -EINVAL;

few_argc:
	DPRINT("too few arguments\r\n");
	return -EINVAL;
}

EXPORT_CMD(xdb,
		""
	);
