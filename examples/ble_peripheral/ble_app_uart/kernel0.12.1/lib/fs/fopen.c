#include <include/stdio.h>
#include <include/string.h>
#include <include/fcntl.h>
#include <include/fs.h>

#define MAX_KFILE		8

static KFILE kfile_array[MAX_KFILE] = {0};

KFILE *get_unused_kfile(void)
{
	int kfl;

	for (kfl = 0; kfl < MAX_KFILE; kfl++) {
		if (!kfile_array[kfl]._fp)
			return &kfile_array[kfl];
	}

	return NULL;
}

int kfstat(struct stat *stat, KFILE *stream)
{
	return fstat(stream->_fd, stat);
}

KFILE *kfopen(const char *path, const char *mode)
{
	__u8 fd;
	__u32 file_seek = 0;
	KFILE *kfile;
	struct file *fp;
	struct stat file_stat;

	if (!path || !mode)
		return NULL;

	fd = open(path, O_WRONLY);
	if (fd < 0)
		return NULL;
	fp = fget(fd);
	fstat(fd, &file_stat);

	kfile = get_unused_kfile();
	if (!kfile)
		return NULL;
	kfile->_fp = fp;
	kfile->_fd = fd;

	if (!kstrncmp(mode, "a+", 2) ||		\
			!kstrncmp(mode, "ab+", 3) || !kstrncmp(mode, "a+b", 3))
		file_seek = file_stat.st_size;
	else if (!kstrncmp(mode, "w+", 2) ||	\
			 !kstrncmp(mode, "wb+", 3) || !kstrncmp(mode, "w+b", 3))
		file_seek = 0;
	else if (!kstrncmp(mode, "r+", 2) ||	\
			 !kstrncmp(mode, "rb+", 3) || !kstrncmp(mode, "r+b", 3))
		file_seek = 0;

	lseek(fd, file_seek, SEEK_SET);

	return kfile;
}

__size_t kfwrite(const void *buffer, __size_t size, __size_t count, KFILE *stream)
{
	return write(stream->_fd, buffer, size);
}

__size_t kfread(void *buffer, __size_t size, __size_t count, KFILE *stream)
{
	return read(stream->_fd, buffer, size);
}

int kfclose(KFILE *stream)
{
	int ret;

	ret = close(stream->_fd);
	kmemset(stream, 0, sizeof(KFILE));

	return ret;
}

loff_t kflseek(loff_t offset, int whence, KFILE *stream)
{
	return lseek(stream->_fd, offset, whence);
}
