#include "include/malloc.h"
#include "include/errno.h"
#include "include/string.h"
#include "include/fs.h"
#include "include/stdio.h"

#define IS_FILENAME_END(p)  ('\r' == *p || '\n' == *p || '\0' == *p)

#define MAX_FDS 5 //256
#define PWALK_DEPTH	5

static struct file *fd_array[MAX_FDS];
static struct fs_struct g_fs;

struct super_block *
sget(struct file_system_type *type, void *data)
{
	struct super_block *sb;

	sb = (struct super_block *)kzalloc(sizeof(*sb), GFP_NORMAL);
	if (!sb)
		return NULL;

	sb->s_bdev = data;

	return sb;
}

struct dentry *
mount_bdev(struct file_system_type *fs_type,
		int flags, const char *dev_name, void *data,
		int (*fill_super)(struct super_block *, void *, int))
{
	int ret;
	struct super_block *sb;
	struct block_device *bdev;
	struct nameidata nd;

	nd.flags |= LOOKUP_OPEN;
	ret = path_walk(dev_name, &nd);
	if (ret < 0) {
		GEN_DBG("\"%s\" does NOT exist!\r\n", dev_name);
		return NULL;
	}
	dev_name = nd.last.name;

	bdev = bdev_get(dev_name);
	if (!bdev) {
		DPRINT("fail to open block device \"%s\"!\r\n", dev_name);
		return NULL;
	}

	sb = sget(fs_type, bdev);
	if (!sb)
		return NULL;

	ret = fill_super(sb, data, 0);
	if (ret < 0)
		return NULL;

	return sb->s_root;
}

struct dentry *
d_alloc(struct dentry *parent, const struct qstr *str)
{
	struct dentry *de;

	de = __d_alloc(parent->d_sb, str);
	if (!de)
		return NULL;

	de->d_parent = parent;
	list_add_tail(&de->d_child, &parent->d_subdirs);

	return de;
}

struct dentry *
___install_dentry(struct dentry *de,
		struct super_block *sb, const struct qstr *str)
{
	char *name;

	de->d_sb = sb;
	de->d_parent = de;
	INIT_LIST_HEAD(&de->d_child);
	INIT_LIST_HEAD(&de->d_subdirs);

	de->d_name.len = str->len;
	if (str->len >= DNAME_INLINE_LEN) {
		name = (char *)kmalloc(str->len + 1, GFP_NORMAL);
		if (!name) {
			kfree(de);
			DPRINT("fail to malloc dentry name\r\n");
			return NULL;
		}
	}
	else {
		name = de->d_iname;
	}
	de->d_name.name = name;
	kstrncpy(name, str->name, str->len);
	name[str->len] = '\0';

	return de;
}

struct dentry *
__d_alloc(struct super_block *sb,
				const struct qstr *str)
{
	struct dentry *de;

	de = (struct dentry *)kzalloc(sizeof(*de), GFP_NORMAL);
	if (!de)
		return NULL;

	return ___install_dentry(de, sb, str);
}

struct dentry *d_make_root(struct inode *root_inode)
{
	struct dentry *root_dir = NULL;

	if (root_inode) {
		static const struct qstr name = {.name = "/", .len = 1};

		root_dir = __d_alloc(root_inode->i_sb, &name);
		if (root_dir)
			root_dir->d_inode = root_inode;
	}

	return root_dir;
}

void dput(struct dentry *dentry)
{
	list_del(&dentry->d_child);
}

struct inode *iget(struct super_block *sb, unsigned long ino)
{
	struct inode *inode;
	// TODO: search

	inode = (struct inode *)kzalloc(sizeof(*inode), GFP_NORMAL); // fixme
	if (!inode)
		return NULL;

	inode->i_sb = sb;
	inode->i_ino = ino;
	
	return inode;
}

static struct inode *__i_alloc(struct super_block *sb)
{
	struct inode *inode;

	inode = (struct inode *)kzalloc(sizeof(*inode), GFP_NORMAL);
	if (!inode)
		return NULL;

	inode->i_sb = sb;

	return inode;
}

struct inode *i_alloc(struct dentry *parent, umode_t mode)
{
	struct inode *inode;

	inode = __i_alloc(parent->d_sb);
	if (!inode)
		return NULL;

	inode->i_mode = mode;
	inode->i_size = 0;
	inode->i_op = parent->d_inode->i_op;
	inode->i_fop = parent->d_inode->i_fop;

	return inode;
}

int get_unused_fd(void)
{
	int fd;

	for (fd = 0; fd < MAX_FDS; fd++) {
		if (!fd_array[fd])
			return fd;
	}

	return -EBUSY;
}

int fd_install(int fd, struct file *fp)
{
	fd_array[fd] = fp;
	return 0;
}

struct file *fget(unsigned int fd)
{
	if (fd < 0 || fd >= MAX_FDS)
		return NULL; // -EINVAL;

	return fd_array[fd];
}

int vfs_mkdir(struct inode *dir, struct dentry *dentry, int mode)
{
	int errno = -ENOTSUPP;

	if (dir->i_op->mkdir)
		errno = dir->i_op->mkdir(dir, dentry, mode);

	return errno;
}

int vfs_mknod(struct inode *dir,
		struct dentry *dentry, int mode, dev_t devt)
{
	if (!dir->i_op->mknod)
		return -ENOTSUPP;

	return dir->i_op->mknod(dir, dentry, mode, devt);
}

struct fs_struct *get_curr_fs()
{
	return &g_fs;
}

void set_fs_root(const struct path *root)
{
	g_fs.root = *root;
}

void set_fs_pwd(const struct path *pwd)
{
	g_fs.pwd = *pwd;
}

void get_fs_root(struct path *root)
{
	*root = g_fs.root;
}

void get_fs_pwd(struct path *pwd)
{
	*pwd = g_fs.pwd;
}

long sys_mkdir(const char *name, unsigned int mode)
{
	int ret;
	struct nameidata nd;
	struct dentry *de;
	struct qstr unit;

	ret = path_walk(name, &nd);
	if (!ret)
		return -EEXIST;

	de = d_alloc(nd.path.dentry, &nd.last);
	if (!de)
		return -ENOMEM;

	ret = vfs_mkdir(nd.path.dentry->d_inode, de, mode | S_IFDIR);
	if (ret < 0) {
		list_del(&de->d_child);
		kfree(de);
	}

	return ret;
}

long sys_chdir(const char *path)
{
	int ret;
	struct nameidata nd;

	ret = path_walk(path, &nd);
	if (ret < 0)
		return ret;

	set_fs_pwd(&nd.path);

	return 0;
}

#define PATH_STACK_SIZE 256

long sys_getcwd(char *buff, unsigned long size)
{
	int top = 0, count;
	struct dentry *stack[PATH_STACK_SIZE];
	struct path cwd, root;

	get_fs_pwd(&cwd);
	get_fs_root(&root);

	while (cwd.dentry != root.dentry) {
		if (cwd.dentry == cwd.mnt->vfsmnt.root) {
			follow_up(&cwd);
			continue; // yes, we'd jump to next loop
		}

		stack[top] = cwd.dentry;
		top++;
		if (top >= PATH_STACK_SIZE) {
			// ret = -EOVERFLOW;
			break;
		}

		cwd.dentry = cwd.dentry->d_parent;
	}

	if (0 == top) {
		buff[0] = '/';
		buff[1] = '\0';
		return 1;
	}

	count = 0;
	while (--top >= 0) {
		buff[count++] = '/';
		kstrcpy(buff + count, stack[top]->d_name.name);
		count += stack[top]->d_name.len;

		if (count >= size) {
			// ret = -EOVERFLOW;
			break;
		}
	}

	return count;
}

void d_instantiate(struct dentry *entry, struct inode * inode)
{
	entry->d_inode = inode;
}

int vfs_create(struct inode *dir,
		struct dentry *dentry, umode_t mode)
{
	int errno;
	struct inode *inode;

	if (!dir->i_op->create)
		return -ENOTSUPP;

	errno = dir->i_op->create(dir, dentry, mode, NULL);
	if (errno < 0)
		return errno;
	
	return errno;
}

long sys_create(const char *filename, umode_t mode)
{
	int ret;
	struct nameidata nd;
	struct dentry *de;

	nd.flags |= LOOKUP_CONTINUE;
	ret = path_walk(filename, &nd);
	if (!ret)
		return -EEXIST;
	
	de = d_alloc(nd.path.dentry, &nd.last);
	if (!de)
		return -ENOMEM;

	ret = vfs_create(nd.path.dentry->d_inode, de, mode | S_IRUSR);
	if (ret < 0) {
		list_del(&de->d_child);
		kfree(de);
	}

	return ret;
}

