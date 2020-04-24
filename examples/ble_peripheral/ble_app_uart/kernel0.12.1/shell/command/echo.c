#include "include/stdio.h"
#include "include/errno.h"
#include "include/string.h"
#include "include/unistd.h"
#include "include/fcntl.h"
#include "include/types.h"
#include "include/shell.h"
#include "include/fs.h"
#include "include/sched.h"

#define ARGV_NUM 		3
#define PATH_LEN 		30
#define CONTENT_LEN		100

struct echo_dat {
	char content[CONTENT_LEN];
	__u16 size;
};

static void usage(void)
{
	IMPORT_CMD(echo);
	DPRINT("%s\r\n", echo_info.help);
}

unsigned char echo(struct task_struct *task, void *argc)
{
	int ret;
	int opt_cnt, n;
	char path[PATH_LEN];
	struct command_opts argv[ARGV_NUM];
	struct echo_dat echo_dat;
	char *mode = NULL;
	KFILE *kfile;

TASK_GEN_STEP_ENTRY(0) {
		kmemset(path, 0, PATH_LEN);
		kmemset(argv, 0, ARGV_NUM * sizeof(struct command_opts));
		kmemset(echo_dat.content, 0, CONTENT_LEN);

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
				case 'cont':
					kmemcpy(echo_dat.content, vstr, vlen);
					echo_dat.size = vlen;
					break;
				case 'mode':
					mode = vstr;
					break;
				case 'help':
				default:
					usage();
					goto unknown_opt;
					break;
			}
		}

		TASK_NEXT_STEP();
	}

TASK_GEN_STEP_ENTRY(1) {
		if (!path[0]) {
			DPRINT("%s\r\n", echo_dat.content);
			goto normal_end;
		}

		kfile = kfopen(path, mode);
		if (!kfile)
			goto normal_end;
		kfwrite(&echo_dat.content, echo_dat.size, 0, kfile);
		kfclose(kfile);
	}

normal_end:
	return 0;

unknown_opt:
	DPRINT("invalid option -- %c\r\n", (argv + n)->opt.str[0]);
	return -EINVAL;

few_argc:
	DPRINT("too few arguments\r\n");
	return -EINVAL;
}

EXPORT_CMD(echo,
		#include "echo.hlp"
	);
