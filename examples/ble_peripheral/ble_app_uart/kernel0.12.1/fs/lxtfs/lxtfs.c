#include "include/stdio.h"
#include "include/init.h"
#include "include/malloc.h"
#include "include/string.h"
#include "include/errno.h"
#include "include/assert.h"
#include "include/block.h"
#include "include/dirent.h"
#include "include/fs.h"
#include "include/types.h"
#include "include/fs/ltxfs.h"
#include "include/cache.h"
#include "include/swab.h"
#include "include/buffer.h"
#include "include/lock.h"
#include <include/sched.h>
#include <include/task.h>
#include <include/msg.h>
#include <include/wait.h>

TASK_EXTERN(ltx_thread);

#define for_bitmap_each_bit(v)	\
			for ((v) = 0; (v) < 32; (v)++)
#define for_bitmap(v, a) \
			for ((v) = 0; (v) < (a); (v)++)


static const struct inode_operations
ltx_reg_inode_operations =
{
	.create = ltx_reg_create,
	.rmdir  = ltx_rmdir,
};

static const struct inode_operations
ltx_dir_inode_operations =
{
	.create = ltx_reg_create,
	.lookup = ltx_lookup,
	.mkdir  = ltx_mkdir,
	.rmdir  = ltx_rmdir,
};

static const struct file_operations
ltx_reg_file_operations =
{
	.open  = ltx_open,
	.close = ltx_close,
	.read  = ltx_read,
	.write = ltx_write,
};

static const struct inode_operations
ltx_db_inode_operations =
{
	.rmdir  = ltx_db_rm,
};

static const struct file_operations
ltx_db_file_operations =
{
	.open  = ltx_db_open,
	.close = ltx_db_close,
	.read  = ltx_db_read,
	.write = ltx_db_write,
};

static const struct file_operations
ltx_dir_file_operations =
{
	.open = ltx_opendir,
	.readdir = ltx_readdir,
};

static struct super_operations ltx_sops =
{
	.alloc_inode = ltx_alloc_inode,
	.read_inode = ltx_read_inode,
};

static int ltx_db_rm(struct inode *inode, struct dentry *dentry)
{
	int ret;
	char *temp;
	struct super_block *sb = inode->i_sb;
	struct ltx_super_block *lxsb;
	struct ltx_dbase *lxdb;
	struct ltx_qstr *lxqs;

	ret = ltx_read_block(sb, &temp, 0, 0, KB(4));
	if (ret < 0)
		return -EBUSY;

	lxqs = (struct ltx_qstr *)temp;
	lxsb = (struct ltx_super_block *)temp;
	return 0;
}

static struct ltx_dbase *
ltx_db_byname(struct ltx_super_block *lxsb, struct ltx_qstr *lqs)
{
	struct ltx_dbase *lxdb_root, *fndb;
	char *db_bitmap;

	lxdb_root = (struct ltx_dbase *)((__u32)lxsb + lxsb->s_root_sysdb_addr);
	db_bitmap = (char *)lxsb + lxsb->s_sysdb_bitmap_addr;

	int i;
	for_bitmap(i, (ALIGN_UP(lxsb->s_sysdb_count, 32) / 32)) {
		__u32 bitmap = *((__u32 *)db_bitmap + i);
		__u32 dbno;

		for_bitmap_each_bit(dbno) {
			if (bitmap & (0x01 << dbno)) {
				fndb = lxdb_root + dbno;
				if (!kstrncmp(fndb->name, lqs->name, lqs->nlen)) {
					lqs->id = i * 32 + dbno;
					return fndb;
				}
			}
		}
	}

	return NULL;
}

static struct ltx_dbase *
ltx_new_db(struct ltx_super_block *lxsb)
{
	struct ltx_dbase *lxdb_root, *fndb;
	char *db_bitmap;
	__u16 fndb_no;

	lxdb_root = (struct ltx_dbase *)((__u32)lxsb + lxsb->s_root_sysdb_addr);
	db_bitmap = (char *)lxsb + lxsb->s_sysdb_bitmap_addr;

	__u32 dbno, i;
	for_bitmap(i, (ALIGN_UP(lxsb->s_sysdb_count, 32) / 32)) {
		__u32 bitmap = *((__u32 *)db_bitmap + i);

		for_bitmap_each_bit(dbno) {
			if (!(bitmap & (0x01 << dbno))) {
				*((__u32 *)db_bitmap + i) |= 0x01 << dbno;
				goto L1;
			}
		}
	}

	return NULL;

L1:
	fndb_no = i * 32 + dbno;
	fndb = lxdb_root + fndb_no;
	kmemset(fndb, 0, sizeof(struct ltx_dbase));
	fndb->id = fndb_no;

	return fndb;
}

static int ltx_del_db(struct ltx_super_block *lxsb, struct ltx_qstr *lqs)
{
	char *db_bitmap;
	__u32 bitmap;
	struct ltx_dbase *lxdb;

	db_bitmap = (char *)lxsb + lxsb->s_sysdb_bitmap_addr;

repeat:
	if (lqs->id) {
		__u16 off = lqs->id % 32;
		*((__u32 *)db_bitmap + lqs->id / 32) &= ~(0x01 << off);
	}
	else {
		lxdb = ltx_db_byname(lxsb, lqs);
		if (!lxdb)
			return -ENODATA;
		else
			goto repeat;
	}

	return 0;
}

static int ltx_db_open(struct file *fp, struct inode *inode)
{
	int ret;
	char *buff;
	struct super_block *sb = fp->f_dentry->d_inode->i_sb;
	struct ltx_super_block *lxsb;

	ret = ltx_read_block(sb, &buff, 0, 0, KB(4));
	if (ret < 0)
		return -EBUSY;

	return 0;
}

static int ltx_db_close(struct file *fp)
{
	return 0;
}

static ssize_t
ltx_db_read(struct file *fp,
		void *buff, __size_t size, loff_t *off)
{
	int ret;
	char *temp;
	struct super_block *sb = fp->f_dentry->d_inode->i_sb;
	struct ltx_super_block *lxsb;
	struct ltx_dbase *lxdb;
	struct ltx_qstr *lxqs;

	ret = ltx_read_block(sb, &temp, 0, 0, KB(4));
	if (ret < 0)
		return -EBUSY;

	lxqs = (struct ltx_qstr *)buff;
	lxsb = (struct ltx_super_block *)temp;

	lxdb = ltx_db_byname(lxsb, lxqs);
	if (!lxdb)
			return -EBUSY;
	kmemcpy(lxqs->val, lxdb->val, lxdb->val_len);

	return size;
}

static ssize_t
ltx_db_write(struct file *fp,
		const void *buff, __size_t size, loff_t *off)
{
	int ret;
	char *temp;
	struct super_block *sb = fp->f_dentry->d_inode->i_sb;
	struct ltx_super_block *lxsb;
	struct ltx_dbase *lxdb;
	struct ltx_qstr *lxqs;

	lxqs = (struct ltx_qstr *)buff;
	if (!lxqs->name || !lxqs->val || !lxqs->nlen || !lxqs->vlen)
		return -EINVAL;

	ret = ltx_read_block(sb, &temp, 0, 0, KB(4));
	if (ret < 0)
		return -EBUSY;

	lxsb = (struct ltx_super_block *)temp;

	lxdb = ltx_db_byname(lxsb, lxqs);
	if (!lxdb) {
		if (!(lxdb = ltx_new_db(lxsb)))
			return -EBUSY;
		kmemcpy(lxdb->name, lxqs->name, lxqs->nlen);
		lxdb->name_len = lxqs->nlen;
	}
	kmemcpy(lxdb->val, lxqs->val, lxqs->vlen);
	lxdb->val_len = lxqs->vlen;
	ltx_write_block(sb, temp, 0, 0, KB(4), LTXFL_CLEAR);

	return 0;
}


static int 
ltx_reg_create(struct inode *parent,
		struct dentry *dentry, umode_t mode,
		struct nameidata *nd)
{
	int ret;
	char *buff, *in_bitmap, *de_bitmap, *bl_bitmap;
	struct super_block *sb;
	struct ltx_super_block *lxsb;
	struct ltx_inode *lxin, *lxin_parent;
	struct ltx_direntry *lxde;
	short int ide, iin, ibl;
	char *lxbl;
	struct inode *in;

	if (!parent || !dentry)
		return -EINVAL;

	in = i_alloc(dentry->d_parent, mode | S_IFREG);
	if (!in)
		return -ENOMEM;
	in->i_fop = &ltx_reg_file_operations;
	in->i_op  = &ltx_reg_inode_operations;

	sb = dentry->d_sb;
	ret = ltx_read_block(sb, &buff, 0, 0, KB(4));
	if (ret < 0)
		goto L1;

	lxsb = (struct ltx_super_block *)buff;

	in_bitmap = buff + lxsb->s_inode_bitmap_addr;
	de_bitmap = buff + lxsb->s_dentry_bitmap_addr;
	bl_bitmap = buff + lxsb->s_block_bitmap_addr;

	lxin = ltx_new_inode(sb, in_bitmap, &iin);
	if (!lxin || !iin)
		goto L1;
	lxde = ltx_new_dentry(sb, de_bitmap, &ide);
	if (!lxde || !ide)
		goto L1;
	lxbl = ltx_new_block(sb, bl_bitmap, &ibl);
	if (!lxbl || !ibl)
		goto L1;

	in->i_ino = iin;
	lxin->i_block[0] = ide;
	lxin->i_block[1] = ibl;
	lxin->i_parent = 0;
	lxin->i_child = 0;
	lxin->i_size = 0;
	lxin->i_mode = in->i_mode;
	lxin->i_blocks = 1;
	lxde->inode = iin;
	ltx_d_install(lxde, dentry, S_IFREG);

	lxin_parent = (struct ltx_inode *)((__u32)buff + lxsb->s_root_inode_addr);
	lxin_parent->i_block[lxin_parent->i_size] = ide;
	lxin_parent->i_size++;

	//lxin->i_parent = lxin_parent->i

	lxsb->s_free_dentry_count--;
	lxsb->s_free_inodes_count--;
	lxsb->s_free_blocks_count--;

	ltx_write_block(sb, buff, 0, 0, KB(4), LTXFL_CLEAR);
	kmemcpy(sb->s_fs_info, lxsb, sizeof(struct ltx_super_block));

	sb->s_root->d_inode->i_size = lxin_parent->i_size;
	dentry->d_inode = in;

	return ret;

L1:
	kfree(in);
	return ret;
}

static int ltx_opendir(struct file *fp, struct inode *inode)
{
	//printk("%s %d\r\n", __func__, __LINE__);
	return 0;
}

static struct inode *
ltx_alloc_inode(struct super_block *sb)
{
	struct ltx_inode_info *lii;

	lii = (struct ltx_inode_info *)kzalloc(sizeof(*lii), GFP_NORMAL);
	if (!lii)
		return NULL;

	lii->vfs_inode.i_sb = sb; // fixme

	return &lii->vfs_inode;
}

static void ltx_read_inode(struct inode *inode)
{
	assert(!inode);

	if (S_ISREG(inode->i_mode)) {
		inode->i_op  = &ltx_reg_inode_operations;
		inode->i_fop = &ltx_reg_file_operations;
	}
	else if (S_ISDIR(inode->i_mode)) {
		inode->i_op  = &ltx_dir_inode_operations;
		inode->i_fop = &ltx_dir_file_operations;
	}
	else {
		BUG();
	}
}

static ssize_t
ltx_read_block(struct super_block *sb,
			void **buff, int blk_no, __size_t off,	/* off: offset in one block */
			__size_t size)
{
	struct buffer_head *bh = NULL;

	if (off > KB(4))
		return -EINVAL;

	while ((!bh && !(bh = getblk(TASK_INFO(ltx_thread), sb->s_bdev->bd_dev, blk_no))) || check_lock(&bh->b_lock))
		sleep_on(TASK_INFO(ltx_thread));

	readblk(bh, buff, off, size);

	return size;
}

int ltx_set_block(struct super_block *sb, int blk_no, __size_t aoff)
{
	struct buffer_head *bh = NULL;

	while ((!bh && !(bh = getblk(TASK_INFO(ltx_thread), sb->s_bdev->bd_dev, blk_no))) || check_lock(&bh->b_lock))
		sleep_on(TASK_INFO(ltx_thread));

	setblk(bh, BH_STAT_MARKBOFF_VALID, aoff, BH_SET_MODE_NO_REPET);

	return 0;
}

static ssize_t
ltx_write_block(struct super_block *sb,
			void *buff, int blk_no, __size_t moff,
			__size_t wsize, umode_t mode)
{
	struct buffer_head *bh = NULL;
	umode_t bh_mode = 0;

	if (moff > KB(4))
		return -EINVAL;

	while ((!bh && !(bh = getblk(TASK_INFO(ltx_thread), sb->s_bdev->bd_dev, blk_no))) || check_lock(&bh->b_lock))
		sleep_on(TASK_INFO(ltx_thread));

	if (mode & LTXFL_CLEAR)
		bh_mode |= BLK_CLEAR;

	writeblk(bh, buff, moff, wsize, bh_mode);

	return wsize;
}

static struct ltx_inode *
ltx_get_inode(struct super_block *sb, int ino)
{
	int ino_off;
	struct ltx_inode *lx_in;
	struct ltx_super_block *lx_sb = sb->s_fs_info;
	char *buff;

	lx_in = kmalloc(lx_sb->s_inode_size, GFP_NORMAL);
	if (!lx_in)
		return NULL;

	ino_off = lx_sb->s_root_inode_addr + ino * lx_sb->s_inode_size;

	ltx_read_block(sb, &buff, 0, ino_off, lx_sb->s_inode_size);
	kmemcpy(lx_in, buff, lx_sb->s_inode_size);

	return lx_in;
}

static kd_bool ltx_check_fstype(void *buff, __size_t size)
{
	struct ltx_super_block *lx_sb = buff;

	if (size >= sizeof(struct ltx_super_block) && LXTFS_MAGIC_NUMBER == lx_sb->s_magic)
		return kd_true;

	return kd_false;
}

static struct inode *
ltx_iget(struct super_block *sb, unsigned long ino)
{
	struct inode *inode;
	struct ltx_inode *lx_in;
	struct ltx_inode_info *lx_ini;

	lx_in = ltx_get_inode(sb, ino);
	if (!lx_in)
		return NULL;

	inode = ltx_alloc_inode(sb);
	if (!inode)
		return NULL;

	inode->i_ino  = ino;
	inode->i_mode = lx_in->i_mode;
	inode->i_size = lx_in->i_size;
	inode->i_private = lx_in;

	lx_ini = LXT_I(inode);
	lx_ini->i_lxin = lx_in; // fixme

	if (S_ISREG(inode->i_mode)) {
		inode->i_op  = &ltx_reg_inode_operations;
		inode->i_fop = &ltx_reg_file_operations;
	}
	else if (S_ISDIR(inode->i_mode)) {
		inode->i_op  = &ltx_dir_inode_operations;
		inode->i_fop = &ltx_dir_file_operations;
	}
	else if (S_ISDBASE(inode->i_mode)) {
		inode->i_op  = &ltx_db_inode_operations;
		inode->i_fop = &ltx_db_file_operations;
	}
	else {
		BUG();
	}

	return inode;
}

void show_stats(struct ltx_super_block *sb)
{
	kprintf("super block information:\r\n");
	kprintf("\tFlash Addr: 0x%08x--0x%08x\r\n",
			sb->s_super_block_addr, 
			sb->s_super_block_addr + sb->s_blocks_count * sb->s_page_size);
	kprintf("\tBitmap offset: inode = 0x%x, block = 0x%x, dentry = 0x%x\r\n",
			sb->s_inode_bitmap_addr, sb->s_block_bitmap_addr,
			sb->s_dentry_bitmap_addr);
	kprintf("\tBitmap take(x4 Byte): inode = %d, block = %d, dentry = %d\r\n",
			sb->s_inodes_count / 32,
			sb->s_blocks_count / 32,
			sb->s_dentry_count / 32);
	kprintf("\tTables offset: inode = 0x%x, dentry = 0x%x.\r\n",
			sb->s_root_inode_addr,
			sb->s_root_direntry_addr);
	kprintf("\tUsed status: inode = %d/%d, block = %d/%d, dentry = %d/%d.\r\n",
			sb->s_inodes_count - sb->s_free_inodes_count,
			sb->s_inodes_count,
			sb->s_blocks_count - sb->s_free_blocks_count,
			sb->s_blocks_count,
			sb->s_dentry_count - sb->s_free_dentry_count,
			sb->s_dentry_count);
	kprintf("\tStruct size: inode = %d, sblock = %d, page = %d, dentry = %d\r\n",
			sb->s_inode_size, sb->s_sblock_size,
			sb->s_page_size, sb->s_dentry_size);

	return;
}

static int ltx_fill_super(struct super_block *sb)
{
	int ret;
	struct ltx_sb_info *lx_sbi;
	struct ltx_super_block *lx_sb;
	struct bio *bio;
	char buff[sizeof(*lx_sbi)];

	bio = bio_alloc();
	if (!bio)
		return -ENOMEM;

	bio->off = 0;
	bio->bdev = sb->s_bdev;
	bio->size = sizeof(buff);
	bio->data = buff;
	bio->wait = TASK_INFO(ltx_thread);
	submit_bio(READ, bio);
	bio_free(bio);

	if (!ltx_check_fstype(buff, sizeof(struct ltx_super_block))) {
		printk("Invalid Lightx-fs magic number!!!\r\n");
		ret = -EINVAL;
		goto L1;
	}

	lx_sbi = (struct ltx_sb_info *)kzalloc(sizeof(*lx_sbi), GFP_NORMAL);
	if (!lx_sbi) {
		ret = -ENOMEM;
		goto L1;
	}

	lx_sb = &lx_sbi->lx_sb;
	kmemcpy(lx_sb, buff, sizeof(*lx_sb));
	sb->s_fs_info = lx_sbi;
	sb->sops = &ltx_sops;

	show_stats(lx_sb);
	return 0;

L2:
	kfree(lx_sbi);
L1:
	return ret;
}

static int ltx_read_super(struct super_block *sb, void *data, int flags)
{
	int ret;
	struct inode *in;
	struct dentry *root;

	ret = ltx_fill_super(sb);
	if (ret < 0) {
		DPRINT("fail to fill super, ret = %d\r\n", ret);
		return ret;
	}

	in = ltx_iget(sb, 0);
	if (!in) {
		DPRINT("fail to get inode, in = %d\r\n", in);
		return -EINVAL;
	}

	root = d_make_root(in);
	if (!root)
		return -EINVAL;

	sb->s_root = root;

	return 0;
}

static unsigned long
ltx_inode_by_name(struct ltx_super_block *lxsb,
		struct inode *inode, struct qstr *unit)
{
	int ret;
	char *in_bitmap;
	struct super_block *sb;
	struct ltx_direntry *lxde_root, *fndde;
	struct ltx_inode *lxin_root, *fndin;
	struct ltx_direntry *lxde;

	if (!inode || !unit || !lxde)
		return -EINVAL;

	lxde_root = (struct ltx_direntry *)((char *)lxsb + lxsb->s_root_direntry_addr);
	lxin_root = (struct ltx_inode *)((char *)lxsb + lxsb->s_root_inode_addr);
	in_bitmap = (char *)lxsb + lxsb->s_inode_bitmap_addr;

	int i;
	for_bitmap(i, (ALIGN_UP(lxsb->s_dentry_count, 32) / 32)) {
		__u32 bitmap = *((__u32 *)in_bitmap + i);
		__u32 ino;

		for_bitmap_each_bit(ino) {
			if (bitmap & (0x01 << ino)) {
				fndin = lxin_root + ino;
				fndde = lxde_root + fndin->i_block[0];
				if (!kstrncmp(fndde->name, unit->name, unit->len))
					return ino;
			}
		}
	}

	return 0;
}

static int ltx_open(struct file *fp, struct inode *inode)
{
	return 0;
}

static int ltx_close(struct file *fp)
{
	return 0;
}

static ssize_t
ltx_read(struct file *fp,
		void *buff, __size_t size, loff_t *off)
{
	int ret;
	char *temp;
	struct ltx_super_block *lxsb;
	struct super_block *sb;
	struct ltx_inode *lxin;
	loff_t blk_no, blk_off, den_no, file_off = *off;
	__le16 page_size, copy_size;
	struct inode *in;

	if (!fp || !buff || !size)
		return -EINVAL;

	sb = fp->f_dentry->d_sb;
	in = fp->f_dentry->d_inode;

	ret = ltx_read_block(sb, &temp, 0, 0, KB(0));
	if (ret < 0)
		return ret;

	lxsb = (struct ltx_super_block *)temp;
	lxin = (struct ltx_inode *)(temp + lxsb->s_root_inode_addr) + in->i_ino;

	blk_no  = lxin->i_block[file_off / lxsb->s_page_size + 1];
	blk_off = file_off % lxsb->s_page_size;
	page_size = lxsb->s_page_size;
	copy_size = lxin->i_size > size ? size : lxin->i_size;

	ret = ltx_read_block(sb, &temp, blk_no, 0, copy_size);
	if (ret < 0)
		return ret;
	kmemcpy(buff, temp + file_off, copy_size);

	return copy_size;
}

static ssize_t
ltx_write(struct file *fp,
		const void *buff, __size_t size, loff_t *off)
{
	struct super_block *sb;
	struct inode *in;
	struct ltx_super_block *lxsb;
	char *sbbh, *wbbh, *lxbl;		// sbbh: super block buffer head. wbbh: write block buffer head
	loff_t blk_nor, blk_off, den_no, file_off = *off;
	struct ltx_inode *lxin, *lxin_parent;
	short int iin, ibl, blk_nor_idx;
	kd_bool prv_newblock = kd_false;
	__u8 *in_bitmap, *de_bitmap, *bl_bitmap;
	__size_t size_temp, offs_temp, size_real, size_remain;
	__size_t boff_newblock, boff_inblock, already_size = 0;

	if (!fp || !buff || !size)
		return -EINVAL;

	sb = fp->f_dentry->d_sb;
	in = fp->f_dentry->d_inode;

	ltx_read_block(sb, &sbbh, 0, 0, KB(4));

	lxsb = (struct ltx_super_block *)sbbh;
	lxin = (struct ltx_inode *)(sbbh + lxsb->s_root_inode_addr) + in->i_ino;

	if (file_off > lxin->i_size)
		return -ERANGE;

	in_bitmap = sbbh + lxsb->s_inode_bitmap_addr;
	bl_bitmap = sbbh + lxsb->s_block_bitmap_addr;

	blk_nor_idx = file_off / lxsb->s_page_size + 1;
	if (blk_nor_idx > lxin->i_blocks) {
		lxbl = ltx_new_block(sb, bl_bitmap, &ibl);
		if (!lxbl || !ibl)
			return -ENOMEM;
		lxin->i_blocks++;
		lxin->i_block[lxin->i_blocks] = ibl;
		lxsb->s_free_inodes_count--;
		lxsb->s_free_blocks_count--;

		ltx_write_block(sb, lxsb, 0, 0, KB(4), LTXFL_CLEAR);
		kmemcpy(sb->s_fs_info, lxsb, sizeof(struct ltx_super_block));
	}

	blk_nor = lxin->i_block[blk_nor_idx];
	blk_off = file_off % lxsb->s_page_size;

	size_real = size;
	boff_inblock = blk_off;

filt_inblock:
	if (boff_inblock + size_real <= KB(4)) {
		size_temp = size_real;
		offs_temp = lxin->i_size % KB(4);

		ltx_read_block(sb, &wbbh, blk_nor, 0, boff_inblock);
		ltx_set_block(sb, blk_nor, offs_temp);
		ltx_write_block(sb, (void *)buff, blk_nor, offs_temp, size_temp, 0);
		already_size += size_real;

		if (prv_newblock)
			goto filt_newblock;
	}
	else {
		prv_newblock = kd_true;
		size_real = KB(4) - boff_inblock;
		goto filt_inblock;

filt_newblock:
		size_remain = size - size_real;
		boff_newblock = 0;
		prv_newblock = kd_false;

		if (lxin->i_blocks + 2 >= LIGH_N_BLOCKS) {
			struct ltx_inode *lxin_temp = ltx_new_inode(sb, in_bitmap, &iin);
			if (!lxin_temp || !iin)
				return -ENOMEM;
			lxin->i_blocks++;
			lxin->i_block[lxin->i_blocks] = iin;
			lxin_temp->i_flags = LXTFLG_IN_L1;
			lxin = lxin_temp;
		}

		lxbl = ltx_new_block(sb, bl_bitmap, &ibl);
		if (!lxbl || !ibl)
			return -ENOMEM;
		lxsb->s_free_inodes_count--;
		lxsb->s_free_blocks_count--;
		blk_nor = ibl;
		lxin->i_blocks++;
		lxin->i_block[lxin->i_blocks] = blk_nor;

		ltx_read_block(sb, &wbbh, blk_nor, 0, boff_newblock);
		ltx_write_block(sb, (void *)((__u32)buff + size_real), blk_nor, 0, size_remain, 0);
		already_size += size_remain;
	}

	*off += already_size;
	lxin->i_size += already_size;
	lxin->i_mtime = get_timestamp();
	in->i_size = lxin->i_size;
	in->i_mtime.tv_sec = lxin->i_mtime;

	ltx_write_block(sb, lxsb, 0, 0, KB(4), LTXFL_CLEAR);
	kmemcpy(sb->s_fs_info, lxsb, sizeof(struct ltx_super_block));

	return 0;
}

static int
ltx_lookup(struct inode *parent,
		struct dentry *dentry, struct nameidata *nd)
{
	int ret;
	char *buff;
	unsigned long ino;
	struct inode *inode;
	struct super_block *sb;
	struct ltx_super_block *lxsb;

	if (!parent || !dentry || !nd)
		return -EINVAL;

	sb = parent->i_sb;
	ret = ltx_read_block(sb, &buff, 0, 0, KB(4));
	if (ret < 0)
		return ret;

	lxsb = (struct ltx_super_block *)buff;
	ino = ltx_inode_by_name(lxsb, parent, &dentry->d_name);
	if (!ino)
		return -ENOENT;

	inode = ltx_iget(parent->i_sb, ino);
	if (!inode)
		return -EIO;

	d_add(dentry, inode);

	return 0;
}

static struct ltx_inode *
ltx_new_inode(struct super_block *sb,
			char *i_bitmap, short *id)
{
	struct ltx_super_block *lxsb;
	__u32 *lxin_bitmap, iroot2bitmap;
	short lxin_count, i, j;
	struct ltx_inode *lxin_root;

	if (!sb || !i_bitmap)
		goto L1;

	lxsb = sb->s_fs_info;
	iroot2bitmap = lxsb->s_root_inode_addr - lxsb->s_inode_bitmap_addr;
	lxin_bitmap  = (__u32 *)i_bitmap;
	lxin_count = lxsb->s_inodes_count / 32;
	lxin_root  = (struct ltx_inode *)((__u32)i_bitmap + iroot2bitmap);

	for_bitmap(i, lxin_count) {
		__u32 bitmap = *(lxin_bitmap + i);

		for_bitmap_each_bit(j) {
			__u32 freebit = (0x01 << j);
			if (!(bitmap & freebit)) {
				*(lxin_bitmap + i) |= freebit;
				*id = (i * 32) + j;
				goto L2;
			}
		}
	}
L1:
	return NULL;

L2:
	return lxin_root + (i * 32) + j;
}

int ltx_del_inode(struct super_block *sb,
			char *i_bitmap, short id)
{
	__u32 *lxin_bitmap, bitmap, bitoff;

	if (!i_bitmap)
		return -EINVAL;

	lxin_bitmap  = (__u32 *)i_bitmap;

	bitmap = id / 32;
	bitoff = id % 32;

	*(lxin_bitmap + bitmap) &= ~(1 << bitoff);

	return 0;
}

static struct ltx_direntry *
ltx_new_dentry(struct super_block *sb,
				char *d_bitmap, short *id)
{
	struct ltx_super_block *lxsb;
	__u32 *lxde_bitmap, droot2bitmap;
	short lxde_count, i, j;
	struct ltx_direntry *lxde_root;

	if (!sb || !d_bitmap)
		goto L1;

	lxsb = sb->s_fs_info;
	droot2bitmap = lxsb->s_root_direntry_addr - lxsb->s_dentry_bitmap_addr;
	lxde_bitmap  = (__u32 *)d_bitmap;
	lxde_count = lxsb->s_dentry_count / 32;
	lxde_root  = (struct ltx_direntry *)((__u32)d_bitmap + droot2bitmap);

	for_bitmap(i, lxde_count) {
		__u32 bitmap = *(lxde_bitmap + i);

		for_bitmap_each_bit(j) {
			__u32 freebit = (0x01 << j);
			if (!(bitmap & freebit)) {
				*(lxde_bitmap + i) |= freebit;
				*id = (i * 32) + j;
				goto L2;
			}
		}
	}
L1:
	return NULL;

L2:
	return lxde_root + (i * 32) + j; 
}

int ltx_del_dentry(struct super_block *sb,
				char *d_bitmap, short id)
{
	__u32 *lxde_bitmap, bitmap, bitoff;

	if (!d_bitmap)
		return -EINVAL;

	lxde_bitmap  = (__u32 *)d_bitmap;

	bitmap = id / 32;
	bitoff = id % 32;

	*(lxde_bitmap + bitmap) &= ~(1 << bitoff);

	return 0;
}


static char *
ltx_new_block(struct super_block *sb,
			char *b_bitmap, short *id)
{
	struct ltx_super_block *lxsb;
	__u32 *lxbl_bitmap;
	short lxbl_count, blkn, i, j;
	char *lxbl_root;
	struct bio bio;

	if (!sb || !b_bitmap)
		goto L1;

	lxsb = sb->s_fs_info;
	lxbl_bitmap  = (__u32 *)b_bitmap;
	lxbl_count = lxsb->s_inodes_count / 32;
	lxbl_root  = (char *)lxsb->s_super_block_addr;

	for_bitmap(i, lxbl_count) {
		__u32 bitmap = *(lxbl_bitmap + i);

		for_bitmap_each_bit(j) {
			__u32 freebit = (0x01 << j);
			if (!(bitmap & freebit)) {
				*(lxbl_bitmap + i) |= freebit;
				blkn = (i * 32) + j;
				goto L2;
			}
		}
	}
L1:
	return NULL;

L2:
	bio.bdev = bdev_by_devt(sb->s_bdev->bd_dev);
	bio.off  = KB(4) * blkn;
	bio.size = KB(4);
	submit_bio(ERASE, &bio);

	*id = blkn;

	return lxbl_root + blkn * lxsb->s_page_size;
}

int ltx_del_block(struct super_block *sb,
			char *b_bitmap, short id)
{
	__u32 *lxbl_bitmap, bitmap, bitoff;

	if (!b_bitmap)
		return -EINVAL;

	lxbl_bitmap  = (__u32 *)b_bitmap;

	bitmap = id / 32;
	bitoff = id % 32;

	*(lxbl_bitmap + bitmap) &= ~(1 << bitoff);

	return 0;
}

static void
ltx_d_install(struct ltx_direntry *lxde,
			struct dentry *dentry, __u16 file_type)
{
	lxde->file_type = file_type;
	// dentry->d_inode->i_mode;
	lxde->name_len  = dentry->d_name.len;
	kmemcpy(lxde->name, dentry->d_iname, dentry->d_name.len);
}

static int
ltx_mkdir(struct inode *inode,
		struct dentry *dentry, umode_t mode)
{
	int ret;
	char *buff, *in_bitmap, *de_bitmap;
	struct super_block *sb;
	struct ltx_direntry *lxde;
	struct ltx_inode *lxin, *lxin_parent;
	struct ltx_super_block *lxsb;
	short int ide, iin;

	if (!inode || !dentry)
		return -EINVAL;

	sb = dentry->d_sb;
	lxsb = (struct ltx_super_block *)buff;

	ret = ltx_read_block(sb, &buff, 0, 0, KB(4));
	if (ret < 0)
		return ret;

	in_bitmap = buff + lxsb->s_inode_bitmap_addr;
	de_bitmap = buff + lxsb->s_dentry_bitmap_addr;

	lxin = ltx_new_inode(sb, in_bitmap, &iin);
	if (!lxin)
		return -ENOIIN;
	lxde = ltx_new_dentry(sb, de_bitmap, &ide);
	if (!lxde)
		return -ENOIDE;
	lxin->i_block[0] = ide;
	lxin->i_parent = 0;
	lxin->i_child = 0;
	lxde->inode = iin;
	ltx_d_install(lxde, dentry, S_IFDIR);

	lxin_parent = (struct ltx_inode *)((__u32)buff + lxsb->s_root_inode_addr);
	lxin_parent->i_block[lxin_parent->i_size] = ide;
	lxin_parent->i_size++;

	lxsb->s_free_dentry_count--;
	lxsb->s_free_inodes_count--;

	ltx_write_block(sb, buff, 0, 0, KB(4), LTXFL_CLEAR);
	kmemcpy(sb->s_fs_info, lxsb, sizeof(struct ltx_super_block));

	sb->s_root->d_inode->i_size = lxin_parent->i_size;

	return 0;
}

static int ltx_rmdir(struct inode *inode, struct dentry *dentry)
{
	int ret;
	char *buff, *in_bitmap, *de_bitmap, *bl_bitmap;
	struct super_block *sb;
	struct ltx_inode *lxin, *lxin_parent, *lxin_root;
	struct ltx_super_block *lxsb;
	short int ide, iin;

	if (!inode || !dentry)
		return -EINVAL;

	sb = dentry->d_sb;
	ret = ltx_read_block(sb, &buff, 0, 0, KB(4));
	if (ret < 0)
		return ret;

	lxsb = (struct ltx_super_block *)(buff);
	lxin_root = (struct ltx_inode *)(buff + lxsb->s_root_inode_addr);
	lxin = lxin_root + inode->i_ino;

	de_bitmap = buff + lxsb->s_dentry_bitmap_addr;
	in_bitmap = buff + lxsb->s_inode_bitmap_addr;
	bl_bitmap = buff + lxsb->s_block_bitmap_addr;

	ltx_del_dentry(sb, de_bitmap, lxin->i_block[0]);
	ltx_del_inode(sb, in_bitmap, inode->i_ino);

	if (S_ISREG(lxin->i_mode)) {
		int blkn, ret;
		for (blkn = 0; blkn < lxin->i_blocks; blkn++) {
			if (!(ret = ltx_del_block(sb, bl_bitmap, lxin->i_block[blkn])))
				lxsb->s_free_blocks_count++;
		}
	}

	lxsb->s_free_dentry_count++;
	lxsb->s_free_inodes_count++;

	for (int i = 0; i < lxin_root->i_size; i++) {
		if (lxin_root->i_block[i] == inode->i_ino) {
			if (i == (lxin_root->i_size - 1))
				lxin_root->i_block[i] = 0;
			else if ((i + 1) < lxin_root->i_size) {
				kmemcpy(&lxin_root->i_block[i], &lxin_root->i_block[i + 1], lxin_root->i_size - 1 - i);
				lxin_root->i_block[lxin_root->i_size - 1] = 0;
			}
			lxin_root->i_size--;
			dentry->d_parent->d_inode->i_size = lxin_root->i_size;
		}
	}

	ltx_write_block(sb, buff, 0, 0, KB(4), LTXFL_CLEAR);
	kmemcpy(sb->s_fs_info, lxsb, sizeof(struct ltx_super_block));

	return 0;
}

static int
ltx_readdir(struct file *fp,
			void *dirent, filldir_t filldir)
{
	int ret;
	char *buff;
	struct inode *in;
	struct ltx_direntry *lxde;
	struct ltx_inode *lxin;
	struct super_block *sb;
	struct ltx_super_block *lxsb;

	if (!fp || !dirent)
		return 0;

	in = fp->f_dentry->d_inode;
	sb = in->i_sb;

	if (fp->f_pos == in->i_size)
		return 0;

	ret = ltx_read_block(sb, &buff, 0, 0, KB(4));
	if (ret < 0)
		return ret;

	lxsb = (struct ltx_super_block *)(buff);
	lxin = (struct ltx_inode *)(buff + lxsb->s_root_inode_addr) + in->i_ino;
	lxde = (struct ltx_direntry *)(buff + lxsb->s_root_direntry_addr) + lxin->i_block[fp->f_pos];

	filldir(dirent, lxde->name, lxde->name_len, fp->f_pos /*fixme*/,
		in->i_ino, lxde->file_type);

	fp->f_pos++;

	return fp->f_pos;
}

struct dentry *
ltx_mount(struct file_system_type *fs,
		int flags, const char *dev_name, void *data)
{
	return mount_bdev(fs, flags, dev_name, data, ltx_read_super);
}

static struct file_system_type ltxfs_type = {
	.name	= "ltxfs",
	.mount	= ltx_mount,
};

static unsigned char
	ltx_thread(struct task_struct *task, void *argc)
{
	return TSLEEP;
}
TASK_INIT(CMD_LXT_THREAD, ltx_thread, kd_true, NULL, 
		EXEC_TRUE,
		SCHED_PRIORITY_MIDE);


int __init ltxfs_init(void)
{
	sched_task_create(TASK_INFO(ltx_thread), 
						SCHED_COMMON_TYPE,
						EXEC_TRUE);
	return register_filesystem(&ltxfs_type);
}

module_init(ltxfs_init);

MODULE_DESCRIPTION("This is i2c ltx File system driver on nrf platform");
MODULE_AUTHOR("xiaowen.huangg@gmail.com");
MODULE_LICENSE("GPL");
