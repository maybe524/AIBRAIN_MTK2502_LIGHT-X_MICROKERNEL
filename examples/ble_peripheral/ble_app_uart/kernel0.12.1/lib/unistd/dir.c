#include "include/syscalls.h"
#include "include/fcntl.h"
#include "include/dirent.h"
#include "include/errno.h"
#include "include/unistd.h"
#include "include/string.h"
#include "include/malloc.h"
#include "include/assert.h"
#include "include/fs.h"
#include "include/stdio.h"

int chdir(const char *path)
{
	long ret;

	ret = sys_chdir(path);
	return ret;
}

char *getcwd(char *buff, __size_t size)
{
	long ret;
	__size_t max_size = min(size, PATH_MAX);

	ret = sys_getcwd(buff, max_size);
	if (ret < 0) {
		GEN_DBG("ret = %d\n", ret);
		return NULL;
	}

	return buff;
}

char *get_current_dir_name(void)
{
	int ret;
	char cwd[PATH_MAX];

	ret = sys_getcwd(cwd, PATH_MAX);
	if (ret < 0)
		return NULL;

	cwd[PATH_MAX - 1] = '\0'; // fixme!
	return NULL;    //strdup(cwd);
}

DIR __capi *opendir(const char *name)
{
	int fd;
	DIR *dir;

	fd = open(name, O_RDONLY);
	if (fd < 0)
		return NULL;

	dir = (DIR *)kzalloc(sizeof(*dir), GFP_NORMAL);
	if (!dir)
		return NULL;

	dir->fd = fd;

	return dir;
}

struct dirent __capi *readdir(DIR *dir)
{
	int ret;
	struct dirent *de;
	struct linux_dirent lde;

	assert(dir);

	ret = sys_getdents(dir->fd, &lde, 1); // fixme
	if (ret <= 0)
		return NULL;

	de = (struct dirent *)kmalloc(lde.d_reclen, GFP_NORMAL);
	if (!de)
		return NULL;

	de->d_ino    = lde.d_ino;
	de->d_off    = lde.d_off;
	de->d_reclen = lde.d_reclen;
	de->d_type   = lde.d_type; // fixme
	kstrcpy(de->d_name, lde.d_name);

	return de;
}

int __capi closedir(DIR *dir)
{
	assert(dir);

	close(dir->fd);
	kfree(dir);

	return 0;
}

int __tapi mkdir(const char *name, unsigned int /*fixme*/ mode)
{
	return sys_mkdir(name, mode);
}
