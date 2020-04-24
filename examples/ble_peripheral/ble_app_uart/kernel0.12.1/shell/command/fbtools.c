#include "include/stdio.h"
#include "include/errno.h"
#include "include/string.h"
#include "include/unistd.h"
#include "include/fcntl.h"
#include "include/dirent.h"
#include "include/shell.h"
#include "include/getopt.h"
#include "include/dirent.h"
#include "include/sched.h"
#include "include/fs.h"
#include "include/fb.h"

#define ARGV_NUM		4

static struct fb_regioninfo fbtools_regioninfo = {
	.xreg = 32,
	.yreg = 64,
	.width	= 32,
	.height = 32,
};

struct fbtools_dat {
	kd_bool is_regionfo_register;
};

struct fbtools_dat this_dat = {0};

static void usage(void)
{
	IMPORT_CMD(fbtools);
	DPRINT("%s\r\n", INCLUD_CMD(fbtools).help);
}

static int fbtools_regioninfo_init(void)
{
	int fd, ret;

	if (!this_dat.is_regionfo_register) {
		fd  = open("/dev/fb0", O_WRONLY);
		ret = ioctl(fd, FB_IOCTRL_REGISTER_REGIONINFO, (__u32)&fbtools_regioninfo);
		if (!ret)
			this_dat.is_regionfo_register = kd_true;
		close(fd);
	}

	return 0;
}

unsigned char fbtools(struct task_struct *task, void *argc)
{
	int ret;
	int n, opt_cnt;
	struct command_opts argv[ARGV_NUM] = {0};
	char *fbmem;
	int fd;

	opt_cnt = getopt((char *)argc, argv, ARGV_NUM);
	if (!opt_cnt)
		goto normal_end;
	for_each_opt(n, opt_cnt) {
		__u8 *ostr = (argv + n)->opt.str, olen = (argv + n)->opt.len;
		__u8 *vstr = (argv + n)->val.str, vlen = (argv + n)->val.len;
		switch (fixopt(ostr, olen)) {
			case 'draw':
				fbtools_regioninfo_init();
				if (eqlopt(vstr, vlen, "font")) {
					fd = open("/dev/fb0", O_WRONLY);
					fbmem = fbtools_regioninfo.regionbase;
					kmemset(fbmem, 0, 300);
					ret = ioctl(fd, FB_IOCTRL_FILREGION, (__u32)&fbtools_regioninfo);
					ret = ioctl(fd, FB_IOCTRL_SYNC, 0);
					close(fd);
				}
				break;
			case 'init':
				fbtools_regioninfo_init();
				break;
			case 'help':
				usage();
				goto normal_end;
			default:
				goto unknown_opt;
				break;
		}
	}

normal_end:
	return 0;

unknown_opt:
	DPRINT("invalid option -- %c\r\n", (argv + n)->opt.str[0]);
	return -EINVAL;
}

EXPORT_CMD(fbtools,
		#include "factory.hlp"
	);

