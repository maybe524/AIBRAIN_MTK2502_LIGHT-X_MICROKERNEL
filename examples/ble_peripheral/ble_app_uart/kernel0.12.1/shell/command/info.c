#include "include/errno.h"
#include "include/string.h"
#include "include/assert.h"
#include "include/unistd.h"
#include "include/dirent.h"
#include "include/types.h"
#include "include/info/user.h"
#include "include/malloc.h"

static struct user_info user = {0, "root", 0, 0, 0, 0, "1989.05.24"};

int get_user_info(struct user_info **info)
{
	*info = &user;

	return 0;
}

int set_user_info(struct user_info *info)
{
	if (!info)
		return -EINVAL;

	kmemcpy(&user, info, sizeof(struct user_info));

	return 0;
}

unsigned char info(struct task_struct *task, void *argc)
{
	int ret; // index;
	void *p;

	// User info
	printk("User info:\r\n"
			"\tId \t%d\r\n"
			"\tName \t%s\r\n"
			"\tSex \t%d\r\n"
			"\tAge \t%d\r\n"
			"\tWeight \t%d\r\n"
			"\tHeight \t%d\r\n"
			"\tBirthday  %s\r\n",
			user.id, user.name, user.sex, user.age, user.weight, user.height, user.birthday);
	
	return 0;
}

EXPORT_CMD(info,
		""
	);