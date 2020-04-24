#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

unsigned char pwd(struct task_struct *task, void *argc)
{
	char path[PATH_MAX];

	if (getcwd(path, PATH_MAX) != NULL)
		printf("%s\n", path);

	return 0;
}

EXPORT_CMD(pwd,
		"sync file system\r\n"
	);
