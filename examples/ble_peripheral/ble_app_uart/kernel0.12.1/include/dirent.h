#pragma once

#include "include/list.h"
#include "include/types.h"

#define MAX_DENT_NAME_SIZE  256

typedef unsigned long ino_t;

typedef struct {
	int fd;
} DIR;

// copy from Linux man page
struct dirent {
	ino_t          d_ino;       /* inode number */
	loff_t         d_off;       /* offset to the next dirent */
	unsigned short d_reclen;    /* length of this record */
	unsigned char  d_type;      /* type of file; not supported by all file system types */
	char           d_name[16];   /* filename */
};

DIR __capi *opendir(const char *name);
struct dirent __capi * readdir(DIR *dir);
int __capi closedir(DIR *dir);
int __capi mkdir(const char *name, unsigned int mode);