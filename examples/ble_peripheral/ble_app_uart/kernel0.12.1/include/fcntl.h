#pragma once

#include "include/types.h"

#define O_RDONLY	1
#define O_WRONLY	2
#define O_RDWR		3
#define O_CREAT		4

struct stat
{
	dev_t	st_dev;		/* Equivalent to drive number 0=A 1=B ... */
	__u16	st_ino;		/* Always zero ? */
	umode_t	st_mode;	/* See above constants */
	short	st_nlink;	/* Number of links. */
	short	st_uid;		/* User: Maybe significant on NT ? */
	short	st_gid;		/* Group: Ditto */
	dev_t	st_rdev;	/* Seems useless (not even filled in) */
	__size_t	st_size;	/* File size in bytes */
	time_t	st_atime;	/* Accessed date (always 00:00 hrs local
				 * on FAT) */
	time_t	st_mtime;	/* Modified time */
	time_t	st_ctime;	/* Creation time */
};

int open(const char *name, int flags, ...);

int close(int fd);

ssize_t read(int fd, void *buff, __size_t count);

ssize_t write(int fd, const void *buff, __size_t count);

int ioctl(int fd, int cmd, ...);

#define SEEK_SET   1
#define SEEK_END   2
#define SEEK_CUR   3

loff_t lseek(int fd, loff_t offset, int whence);

int fstat(int fd, struct stat *stat);

int GAPI mount(const char *, const char *, const char *, unsigned long);

int GAPI umount(const char *mnt);
