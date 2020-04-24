#include <include/stdio.h>
#include <include/errno.h>
#include <include/string.h>
#include <include/unistd.h>
#include <include/fcntl.h>
#include <include/dirent.h>
#include <include/shell.h>
#include <include/getopt.h>
#include <include/dirent.h>
#include <include/sched.h>
#include <include/fs.h>

#define ARGV_NUM		3
#define PATH_LEN		32

static struct command_opts *opts = NULL, *opt_path = NULL;
static char *buff = NULL;
static struct task_struct *task_ls = NULL;

unsigned char ls(struct task_struct *task, void *argc)
{
	int n, opt_cnt;
	DIR *dir;
	struct dirent *entry;
	struct command_opts argv[ARGV_NUM];
	char path[PATH_LEN];
	kd_bool print_bylist = kd_false;

TASK_GEN_STEP_ENTRY(0) {
		kmemset(path, 0, PATH_LEN);
		kmemset(argv, 0, ARGV_NUM * sizeof(struct command_opts));
		path[0] = '.';

		opt_cnt = getopt(argc, argv, ARGV_NUM);
		if (opt_cnt) {
			for (n = 0; n < opt_cnt; n++) {
				switch ((argv + n)->opt.len) {
				case 1:
					if (!kstrncmp((argv + n)->opt.str, "p", 1))
						kmemcpy(path, (argv + n)->val.str, (argv + n)->val.len);
					else if (!kstrncmp((argv + n)->opt.str, "l", 1))
						print_bylist = kd_true;
					else
						goto L1;
					break;
				default:
					goto L1;
				}
			}
		}
		
		TASK_NEXT_STEP();
	}

TASK_GEN_STEP_ENTRY(1) {
		dir = opendir(path);
		if (!dir) {
			printk("cannot access \"%s\"\r\n", path);
			return -ENOENT;
		}

		while ((entry = readdir(dir))) {
			if (print_bylist)
				kprintf("%s%c\r\n", entry->d_name, S_ISDIR(entry->d_type) ? '/' : ' ');
			else
				kprintf("%s%c\t", entry->d_name, S_ISDIR(entry->d_type) ? '/' : ' ');
			kfree(entry);
		}

		closedir(dir);

		TASK_SET_STEP(0);
	}

	return 0;

L1:
	DPRINT("invalid option -- %c\r\n", (argv + n)->opt.str[0]);
	return -EINVAL;
}

EXPORT_CMD(ls,
		""
	);
