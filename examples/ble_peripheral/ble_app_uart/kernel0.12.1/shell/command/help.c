#include "include/stdio.h"
#include "include/errno.h"
#include "include/string.h"
#include "include/unistd.h"
#include "include/fcntl.h"
#include "include/dirent.h"
#include "include/block.h"
#include "include/shell.h"

extern struct command_info *cmd_info_list[];

int __capi help(char *argv)
{
	int i = 0;
	struct command_info **cmd_info = cmd_info_list;

	while (kd_true) {
		if (!cmd_info[i])
			break;
		printk("%02d\t%s\r\n", i, cmd_info[i]->index);
		i++;
	}

	return 0;
}

EXPORT_CMD(help,
		"usage: help -[OPTION]=[CMD]\r\n"
		"  -all \t printf all command info\r\n"
		"  -cmd \t printf one command info\r\n"
	);
