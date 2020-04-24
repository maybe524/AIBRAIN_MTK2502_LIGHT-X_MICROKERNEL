#pragma once

struct command_info {
	unsigned char (*call_func)(struct task_struct *, void *);
	const char *index, *help;
};

struct estr {
	char *str;
	__u8 len;
};

struct command_opts {
	struct estr opt, val;
};


#define IS_STR_END(p)  \
            ('\r' == *p || '\n' == *p || '\0' == *p)

#define EXPORT_CMD(cmd, hlp)     \
		struct command_info cmd##_info = {	\
			.call_func = cmd, .index = #cmd, .help = hlp	\
			};

#define IMPORT_CMD(cmd)     \
		extern struct command_info cmd##_info;

#define INCLUD_CMD(cmd)     \
		&cmd##_info

IMPORT_CMD(mkdir);
IMPORT_CMD(ls);
IMPORT_CMD(cd);
IMPORT_CMD(help);
IMPORT_CMD(touch);
IMPORT_CMD(access);
IMPORT_CMD(clear);
IMPORT_CMD(mountfs);
IMPORT_CMD(info);
IMPORT_CMD(echo);
IMPORT_CMD(cat);
IMPORT_CMD(rm);
IMPORT_CMD(mklxtfs);
IMPORT_CMD(sync);
IMPORT_CMD(xdb);
IMPORT_CMD(factory);
IMPORT_CMD(gpio2s);
IMPORT_CMD(fbtools);