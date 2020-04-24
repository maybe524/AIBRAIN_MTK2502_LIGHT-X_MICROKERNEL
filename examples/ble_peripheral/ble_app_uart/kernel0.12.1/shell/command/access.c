/*
int _access( const char *path, int mode );
Return Value
Each of these functions returns 0 if the file has the given mode. 
The function returns –1 if the named file does not exist or is not 
accessible in the given mode; in this case, errno is set as follows:
EACCES
Access denied: file’s permission setting does not allow specified access.
ENOENT
Filename or path not found.
Parameters
path
File or directory path
mode
Permission setting
Remarks
When used with files, the _access function determines whether the 
specified file exists and can be accessed as specified by the value of mode. 
When used with directories, _access determines only whether the specified directory exists; 
in Windows NT, all directories have read and write access.
mode Value            Checks File For 
00                              Existence only 
02                              Write permission 
04                              Read permission 
06                              Read and write permission 


*/

#include "include/stdio.h"
#include "include/errno.h"
#include "include/string.h"
#include "include/unistd.h"
#include "include/fcntl.h"
#include "include/dirent.h"
#include "include/block.h"
#include "include/shell.h"
#include "include/getopt.h"

#define ARGV_NUM    2
#define PATH_LEN    10
#define CMD_TAG		"access: "

int ___access(const char *path, int mode)
{
    return 0;
}

int __capi access(char *argc)
{
	int n, opt_cnt;
	DIR *dir;
	struct dirent *entry;
    struct command_opts argv[ARGV_NUM];
    char path[PATH_LEN];

    kmemset((void *)path, 0, PATH_LEN);
    kmemset((void *)argv, 0, ARGV_NUM * sizeof(struct command_opts));
    kmemcpy((void *)path, ".", 1);

    opt_cnt = getopt(argc, argv, ARGV_NUM);
    if (opt_cnt) {
		for (n = 0; n < opt_cnt; n++) {
			switch ((argv + n)->opt.len) {
			case 1:
				if (!u_strncmp((argv + n)->opt.str, "p", 1))
					u_memcpy(path, (argv + n)->val.str, (argv + n)->val.len);
				else
					goto opts_err;
				break;
			default:
				goto opts_err;
			}
		}
    }

	dir = opendir(path);
	if (!dir) {
		DPRINT("cannot access \"%s\"\r\n", path);
		return -ENOENT;
	}

	while ((entry = readdir(dir))) {
		DPRINT("%s\r\n", entry->d_name);
	}

	closedir(dir);

	return 0;

opts_err:
	DPRINT(CMD_TAG "invalid option -- %c\r\n", (argv + n)->opt.str[0]);
	return -EINVAL;
}

EXPORT_CMD(access);
