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

#define ARGV_NUM 	3
#define PATH_LEN 	25
#define CONTENT_LEN		128

#define PRINFMT_HEX			0x01
#define PRINFMT_STRING		0x02

static void usage(void)
{
	IMPORT_CMD(cat);
	DPRINT("%s\r\n", INCLUD_CMD(cat).help);
}

unsigned char cat(struct task_struct *task, void *argc)
{
	int ret, fd;
	int opt_cnt, n;
	char path[PATH_LEN], buff[CONTENT_LEN + 1];
	struct command_opts argv[ARGV_NUM];
	__u32 want_read_size, read_offs = 0, real_size, already_read_size = 0;
	__u8 prin_fmt = 0;
	KFILE *kfile;
	struct stat kfile_stat;
	int loop_cnt = 0, temp1, temp2;
	int m = 0;

TASK_GEN_STEP_ENTRY(0) {
		__u8 temp[8] = {0};

		kmemset(path, 0, PATH_LEN);
		kmemset(argv, 0, ARGV_NUM * sizeof(struct command_opts));

		opt_cnt = getopt(argc, argv, ARGV_NUM);
		if (!opt_cnt)
			goto L1;
		for_each_opt(n, opt_cnt) {
			__u8 *ostr = (argv + n)->opt.str, olen = (argv + n)->opt.len;
			__u8 *vstr = (argv + n)->val.str, vlen = (argv + n)->val.len;
			switch (fixopt(ostr, olen)) {
				case 'file':
				case 'f':
					kmemcpy(path, vstr, vlen);
					break;
				case 'size':
				case 's':
					kmemcpy(temp, vstr, vlen);
					str_to_val(temp, &want_read_size);
					break;
				case 'offs':
					__u8 temp[10] = {0};
					kmemcpy(temp, vstr, vlen);
					str_to_val(temp, &read_offs);
					break;
				case 'pfmt':
					if (eqlopt(vstr, vlen, "hex"))
						prin_fmt = PRINFMT_HEX;
					else if (eqlopt(vstr, vlen, "string"))
						prin_fmt = PRINFMT_STRING;
					break;
				default:
					usage();
					goto unknown_opt;
					break;
			}
		}
		TASK_NEXT_STEP();
	}

TASK_GEN_STEP_ENTRY(1) {
		kfile = kfopen(path, "rb+");
		if (!kfile)
			goto normal_end;
		kfstat(&kfile_stat, kfile);

		temp1 = ALIGN_UP(kfile_stat.st_size, CONTENT_LEN) / CONTENT_LEN;
		temp2 = ALIGN_UP(want_read_size, CONTENT_LEN) / CONTENT_LEN;
		if (!temp1 || !temp2)
			goto normal_end;
		loop_cnt = temp1 > temp2 ? temp2 : temp1;

		already_read_size += read_offs;
		while (kd_true) {
			if (m == loop_cnt)
				break;

			kmemset(buff, 0, sizeof(buff));
			kflseek(already_read_size, SEEK_SET, kfile);
			if (m == loop_cnt - 1)
				real_size = (want_read_size > kfile_stat.st_size ? kfile_stat.st_size : want_read_size) % CONTENT_LEN;
			else
				real_size = CONTENT_LEN;
			kfread(buff, real_size, 0, kfile);

			switch (prin_fmt) {
				case PRINFMT_HEX:
					for (int i = 0; i < real_size; i++)
						kprintf("%02x ", buff[i]);
					break;
				case PRINFMT_STRING:
				default:
					kprintf("%s", buff);
					break;
			}

			already_read_size += real_size;
			m++;
		}

		kfclose(kfile);
		kprintf("\r\n");
	}

normal_end:
	return 0;

unknown_opt:
	DPRINT("invalid option -- %c\r\n", (argv + n)->opt.str[0]);
	return -EINVAL;

L1:
	DPRINT("too few arguments\r\n");
	return -EINVAL;
}

EXPORT_CMD(cat, 
		""
	);
