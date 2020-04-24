#include "include/errno.h"

#define PATCH_DEFUALT_LEN	64


int kobj_add_internal(char *name, char *parent)
{
	int ret;
	char strcomb[PATCH_DEFUALT_LEN];

	if (!name || !parent)
		return -EINVAL;

	ret = kstrcomb(strcomb, sizeof(strcomb), parent, name);
	if (ret < 0)
		return ret;

	sys_create(strcomb, 0755);
	return 0;
}