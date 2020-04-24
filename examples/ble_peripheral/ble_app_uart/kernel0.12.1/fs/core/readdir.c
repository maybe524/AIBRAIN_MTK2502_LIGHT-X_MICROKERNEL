#include "include/errno.h"
#include "include/string.h"
#include "include/fs.h"
#include "include/stdio.h"

int filldir(void *de, const char *name,
		int size, loff_t offset, u64 ino, unsigned type)
{
	__size_t reclen;
	struct linux_dirent *lde = de;

	lde->d_ino  = ino;
	lde->d_off  = offset;
	lde->d_type = type;

	reclen = (__size_t)(&((struct linux_dirent *)0)->d_name) + size + 2;
	ALIGN_UP2(reclen, sizeof(long));
	lde->d_reclen = reclen;

	kmemcpy(lde->d_name, name, size);
	lde->d_name[size] = '\0';

	return 0;
}

int sys_getdents(unsigned int fd, struct linux_dirent *lde, unsigned int count)
{
	int ret;
	struct file *fp;

	fp = fget(fd);
	if (!fp || !fp->f_op) {
		DPRINT("no fp or f_op\r\n");
		return -ENODEV;
	}

	if (!fp->f_op->readdir) {
		DPRINT("no readdir function!\r\n");
		return -ENOTSUPP;
	}

	ret = fp->f_op->readdir(fp, lde, filldir);
	return ret;
}
