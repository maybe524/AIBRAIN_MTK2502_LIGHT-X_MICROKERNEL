#include "include/errno.h"
#include "include/stdio.h"
#include "include/string.h"
#include "include/malloc.h"
#include "include/fcntl.h"
#include "include/assert.h"
#include "include/fs.h"

struct mount *lookup_mnt(struct path *path);

static inline void path_to_nameidata(const struct path *path,
					struct nameidata *nd)
{
	nd->path.mnt = path->mnt;
	nd->path.dentry = path->dentry;
}

struct dentry *d_lookup(struct dentry *parent, struct qstr *unit)
{
	struct dentry *de;
	struct list_head *iter;

	list_for_each(iter, &parent->d_subdirs) {
		de = container_of(iter, struct dentry, d_child);
		if (de->d_name.len == unit->len && \
				!kstrncmp(de->d_name.name, unit->name, unit->len))
			return de;
	}

	return NULL;
}

int real_lookup(struct dentry *parent,
		struct qstr *unit, struct nameidata *nd,
		struct dentry **result)
{
	struct dentry *dentry = d_alloc(parent, unit);

	if (dentry) {
		int ret;
		struct inode *dir = parent->d_inode;

		ret = dir->i_op->lookup(dir, dentry, nd);
		if (ret < 0) {
			GEN_DBG("fail to lookup \"%s\" (ret = %d)!\r\n", dentry->d_name.name, ret);
			dput(dentry);
			return ret;
		}

#if 0
		if (result && result != dentry) {
			GEN_DBG("%s -> %s\n", dentry->d_name.name, result->d_name.name);
			dput(dentry);
			dentry = result;
		}
#endif

		*result = dentry;
	}

	return 0;
}

static int __follow_mount(struct path *path)
{
	int res = 0;

	if (path->dentry->d_mounted) {
		struct mount *mnt = lookup_mnt(path);
		if (mnt) {
			path->mnt = mnt;
			path->dentry = mnt->vfsmnt.root;
		}
	}

	return res;
}

int follow_up(struct path *path)
{
	path->dentry = path->mnt->mountpoint;
	path->mnt = path->mnt->mnt_parent;

	return 0;
}

int do_lookup(struct nameidata *nd, struct qstr *name,
		struct path *path)
{
	struct mount *mnt = nd->path.mnt;
	struct dentry *dentry;

	if ('.' == name->name[0]) {
		// TODO: avoid running here
		if (1 == name->len) {
			*path = nd->path;
			return 0;
		}

		if ('.' == name->name[1] && 2 == name->len) {
			if (path->dentry == path->mnt->vfsmnt.root && path->mnt->mnt_parent)
				follow_up(&nd->path);

			path->dentry = nd->path.dentry->d_parent;
			path->mnt = nd->path.mnt;

			return 0;
		}
	}

	dentry = d_lookup(nd->path.dentry, name);
	if (!dentry) {
		int ret;

		ret = real_lookup(nd->path.dentry, name, nd, &dentry);
		if (ret < 0)
			return ret;
	}

	path->mnt = mnt;
	path->dentry = dentry;

	__follow_mount(path);

	return 0;
}

static inline kd_bool cannot_lookup(const struct inode *in)
{
	return in->i_op->lookup == NULL;
}

int path_walk(const char *path, struct nameidata *nd)
{
	int ret;
	struct path next;
	struct inode *inode;
	unsigned int lookup_flags = nd->flags;

	if ('/' == *path)
		get_fs_root(&nd->path);
	else
		get_fs_pwd(&nd->path);

	while (kd_true) {
		struct qstr this;

		while ('/' == *path) path++;
		if (!*path)
			break;

		this.name = path;
		do {
			path++;
		} while (*path && '/' != *path);
		this.len = path - this.name;

		nd->last = this;
		
		if (cannot_lookup(nd->path.dentry->d_inode)) { // right here?
			printk("dentry \"%s\" cannot lookup! i_op = %p\r\n",
				nd->path.dentry->d_name.name, nd->path.dentry->d_inode->i_op);
			return -ENOTDIR;
		}

		ret = do_lookup(nd, &this, &next);
		if (ret < 0)
			return ret;

		path_to_nameidata(&next, nd);

		inode = next.dentry->d_inode;
#if 0
		if (lookup_flags == LOOKUP_DIRECTORY && \
				(!inode->i_op || !inode->i_op->lookup))
			return -ENOTDIR;
		else if (lookup_flags == LOOKUP_CONTINUE && \
				(!inode->i_op || !inode->i_op->lookup))
			return -ENFILE;
#endif
	}

	return 0;
}

static int __dentry_open(struct dentry *dir, struct file *fp)
{
	int ret;
	struct inode *inode = dir->d_inode;

	fp->f_dentry = dir;
	fp->f_pos = 0;

	fp->f_op = inode->i_fop;
	if (!fp->f_op) {
		GEN_DBG("No fops for \"%s\"!\r\n", dir->d_name.name);
		return -ENODEV;
	}

	// TODO: check flags

	if (fp->f_op->open) {
		ret = fp->f_op->open(fp, inode);
		if (ret < 0) {
			GEN_DBG("open failed! (ret = %d)\r\n", ret);
			return ret;
		}
	}

	return 0;
}

static int sys_open(const char *path, int flags, int mode)
{
	int fd, ret;
	struct file *fp;
	struct nameidata nd;

	fd = get_unused_fd();
	if (fd < 0)
		return fd;

	ret = path_walk(path, &nd);
	if (ret < 0) {
		printk("fail to find \"%s\"!\r\n", path);
		return ret;
	}

	fp = (struct file *)kzalloc(sizeof(*fp), GFP_NORMAL); // use malloc instead of zalloc
	if (!fp) {
		ret = -ENOMEM;
		goto no_mem;
	}

	fp->f_flags = flags;

	ret = __dentry_open(nd.path.dentry, fp);
	if (ret < 0)
		goto fail;

	fd_install(fd, fp);

	return fd;

fail:
	kfree(fp);
no_mem:
	return ret;
}

int open(const char *path, int flags, ...)
{
	int mode = 0;

	return sys_open(path, flags, mode);
}

int close(int fd)
{
	struct file *fp;

	fp = fget(fd);
	if (!fp)
		return -ENOENT;

	fd_install(fd, NULL);

	if (fp->f_op && fp->f_op->close)
		fp->f_op->close(fp);

	kfree(fp);

	return 0;
}

ssize_t read(int fd, void *buff, __size_t size)
{
	struct file *fp;

	fp = fget(fd);
	if (!fp || !fp->f_op->read)
		return -ENOENT;

	return fp->f_op->read(fp, buff, size, &fp->f_pos);
}

ssize_t write(int fd, const void *buff, __size_t size)
{
	struct file *fp;

	fp = fget(fd);
	if (!fp || !fp->f_op->write)
		return -ENOENT;

	return fp->f_op->write(fp, buff, size, &fp->f_pos);
}

int sys_ioctl(int fd, int cmd, unsigned long arg)
{
	struct file *fp;

	fp = fget(fd);
	if (!fp || !fp->f_op->ioctl)
		return -ENOENT;

	return fp->f_op->ioctl(fp, cmd, arg);
}

int ioctl(int fd, int cmd, ...)
{
	unsigned long arg;

	arg = *((unsigned long *)&cmd + 1); // fixme

	return sys_ioctl(fd, cmd, arg);
}

loff_t lseek(int fd, loff_t offset, int whence)
{
	struct file *fp;

	fp = fget(fd);
	if (!fp)
		return -ENOENT;

	if (fp->f_op->lseek)
		return fp->f_op->lseek(fp, offset, whence);

	switch (whence) {
		case SEEK_SET:
			fp->f_pos = offset;
			break;
		case SEEK_CUR:
			fp->f_pos += offset;
			break;
		case SEEK_END:
			fp->f_pos += fp->f_dentry->d_inode->i_size;
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

int fstat(int fd, struct stat *stat)
{
	struct file *fp;
	struct inode *inode;

	fp = fget(fd);
	if (!fp)
		return -ENOENT;

	inode = fp->f_dentry->d_inode;

	stat->st_size = inode->i_size;
	//stat->st_mtime = inode->i_mtime;
	stat->st_mode = inode->i_mode;

	return 0;
}

static int __dentry_remove(struct dentry *dir)
{
	int ret = -ENOTSUPP;

	if (!dir || !dir->d_inode)
		return -EINVAL;

	if (dir->d_inode->i_op)
		ret = dir->d_inode->i_op->rmdir(dir->d_inode, dir);

	list_del(&dir->d_child);

	return ret;
}


int remove(char *filename, umode_t flags)
{
	int ret;
	struct nameidata nd;

	if (!filename)
		return -EINVAL;

	ret = path_walk(filename, &nd);
	if (ret < 0) {
		printk("fail to find \"%s\"!\r\n", filename);
		return ret;
	}

	ret = __dentry_remove(nd.path.dentry);

	return ret;
}
