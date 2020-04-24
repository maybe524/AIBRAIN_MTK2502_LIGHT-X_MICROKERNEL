#include "include/io.h"
#include "include/init.h"
#include "include/errno.h"
#include "include/malloc.h"
#include "include/mtd/nand.h"
#include "include/mtd/data_flash.h"
#include "include/drive.h"
#include "include/fs/ltxfs.h"
#include "include/swab.h"
#include "include/stdio.h"
#include "include/buffer.h"
#include "include/assert.h"
#include "include/wait.h"
#include "include/sched.h"
#include "include/task.h"
#include "include/lock.h"

#include "nrf.h"
#include "fstorage.h"

#define SUPER_BLOCK_OFF			KB(0)
#define FISRT_DENTRY_ADDR		KB(1)
#define PAGE_SIZE				KB(4)

#define FLASH_START		(0x00060000)
#define PRIVATE_DATA	(FLASH_START - KB(1))
#define PAGE_CNT_ALL	(50)
#define FLASH_SIZE		(KB(PAGE_CNT_ALL))

static kd_bool nrfs_nand_erase_compl = kd_false, nrfs_nand_store_compl = kd_false;
DECLARE_WAIT_QUEUE_HEAD(WAIT_QUEUE_FLASH, nrfs_nand_waiter_queue);

kd_bool nrfs_nand_check_busy(void)
{
	wait_queue_head_t *nwq = &nrfs_nand_waiter_queue;

	return check_lock(&nwq->lock);
}

static void nrfs_nand_fs_event_handler(fs_evt_t const * const p_evt, fs_ret_t result)
{
	switch (p_evt->id) {
		case FS_EVT_ERASE:
#ifdef NFFS_NAND_DEBUG
			printk("FS_EVT_ERASE\r\n");
#endif
			nrfs_nand_erase_compl = kd_true;
			break;
		case FS_EVT_STORE:
#ifdef NFFS_NAND_DEBUG
			printk("FS_EVT_STORE\r\n");
#endif
			nrfs_nand_store_compl = kd_true;
			break;
		default:
			break;
	}
}

// Our fstorage configuration.
FS_REGISTER_CFG(fs_config_t nrfs_nand_fs_config) = {
    .callback  = nrfs_nand_fs_event_handler,
    .num_pages = PAGE_CNT_ALL,
    // We register with the highest priority in order to be assigned
    // the pages with the highest memory address (closest to the bootloader).
    .priority  = 0xFF
};

static int nrfs_page_erase(__u32 page_off, uint32_t page_num)
{
	int ret;
	fs_config_t *fs_cfg = &nrfs_nand_fs_config;
	wait_queue_head_t wq, *nwq = &nrfs_nand_waiter_queue;

	if (check_lock(&nwq->lock))
		return -EBUSY;

	lock(&nwq->lock);

	nrfs_nand_erase_compl = kd_false;
	ret = fs_erase(fs_cfg, (uint32_t *)((uint32_t)fs_cfg->p_start_addr + page_off), page_num);
	if (ret) {
		printk("Fs erase ret: %d\r\n", ret);
		goto erase_fail;
	}

	wait_event_timeout(wq, NULL, nrfs_nand_erase_compl == kd_true, Second(10));

erase_fail:
	unlock(&nwq->lock);

	return ret;
}

static int nrfs_del_blocks(struct disk_drive *drive, u_long off, __size_t size)
{
	__size_t tmp = ALIGN_UP(size, KB(4));

	return nrfs_page_erase((__u32)off, tmp / PAGE_SIZE);
}

static int nrfs_write_buff(__u32 off, const __u8 *buff, int size)
{
	int ret, i;
	fs_config_t *fs_cfg = &nrfs_nand_fs_config;
	wait_queue_head_t wq, *nwq = &nrfs_nand_waiter_queue;

	if (!buff || (size % sizeof(__u32)))
		return -ERANGE;

	if (check_lock(&nwq->lock))
		return -EBUSY;
	lock(&nwq->lock);

	nrfs_nand_store_compl = kd_false;
	ret = fs_store(fs_cfg, (uint32_t *)((uint32_t)fs_cfg->p_start_addr + off), (__u32 *)buff, size / 4);
	if (ret) {
		printk("Fs store ret: "
				"%d, off: 0x%x, buff: 0x%p, size: 0x%x\r\n",
				ret, off, buff, size);
		goto write_fail;
	}

	wait_event_timeout(wq, NULL, nrfs_nand_store_compl == kd_true, Second(10));

write_fail:
	unlock(&nwq->lock);

	return ret;
}

static int
nrfs_put_data(struct disk_drive *drive,
			u_long off, __size_t size, const void *buff)
{
	int ret;

	if (!buff || !size)
		return -EIO;

	return nrfs_write_buff(off, buff, size);
}

static int nrfs_get_data(struct disk_drive *drive, u_long off, __size_t size, void *buff)
{
	fs_config_t *fs_cfg = &nrfs_nand_fs_config;

	if (!buff || !size)
		return 0;

	kmemset(buff, 0, size);
	kmemcpy(buff, (void *)((u_long)fs_cfg->p_start_addr + off), size);

	return 0;
}

/*
*  Lxtfs map is follow:
*	1. ltx super block
*	2. ltx inode bitmap
*	3. ltx block bitmap
*	4. ltx dentry bitmap
*	5. ltx inode table
*	6. ltx direntry table
*/
int nrfs_nand_check_format(kd_bool force_format)
{
	int ret;
	char *buff;
	struct ltx_super_block *lx_sb;
	struct ltx_inode *in_root;
	struct ltx_direntry *de_root;
	uint32_t *inode_bitmap, *block_bitmap, *dentry_bitmap;
	struct buffer_head *bh;
	fs_config_t *fs_cfg = &nrfs_nand_fs_config;

	assert(bh = find_free_buffer());	// temporary occupation
	buff = (container_of(bh, struct bh_data, bh))->data;

	nrfs_get_data(NULL, (u_long)SUPER_BLOCK_OFF, sizeof(struct ltx_super_block), buff);
	lx_sb = (struct ltx_super_block *)buff;
	printk("Detect flash magic number: %x, " \
			"%s format it!!!\r\n", lx_sb->s_magic, \
			lx_sb->s_magic == LXTFS_MAGIC_NUMBER ? "No need" : "Will to");

	if (!force_format && lx_sb->s_magic == LXTFS_MAGIC_NUMBER) {
		kmemset(buff, 0, KB(4));
		return 0;
	}

	kmemset(buff, 0, KB(4));
	ret = nrfs_page_erase(SUPER_BLOCK_OFF, 1);

	// Supper block init
	lx_sb->s_super_block_addr = (__u32)fs_cfg->p_start_addr;
	lx_sb->s_magic = LXTFS_MAGIC_NUMBER;
	lx_sb->s_page_size = PAGE_SIZE;
	lx_sb->s_sblock_size = sizeof(struct ltx_super_block);
	lx_sb->s_inode_size  = sizeof(struct ltx_inode);
	lx_sb->s_dentry_size = sizeof(struct ltx_direntry);
	lx_sb->s_sysdb_size  = sizeof(struct ltx_dbase);

	lx_sb->s_blocks_count = (KB(512) - FLASH_START) / lx_sb->s_page_size;
	lx_sb->s_inodes_count = (KB(512) - FLASH_START) / lx_sb->s_page_size;
	lx_sb->s_dentry_count = 32;
	lx_sb->s_sysdb_count  = 32;

	lx_sb->s_inode_bitmap_addr  = sizeof(struct ltx_super_block);
	lx_sb->s_block_bitmap_addr  = lx_sb->s_inode_bitmap_addr + (lx_sb->s_inodes_count / 32 * 4);
	lx_sb->s_dentry_bitmap_addr = lx_sb->s_block_bitmap_addr + (lx_sb->s_blocks_count / 32 * 4);
	lx_sb->s_sysdb_bitmap_addr  = lx_sb->s_dentry_bitmap_addr + (lx_sb->s_dentry_count / 32 * 4);

	lx_sb->s_root_inode_addr = lx_sb->s_sysdb_bitmap_addr + (lx_sb->s_sysdb_count / 32 * 4);
	lx_sb->s_root_direntry_addr = lx_sb->s_root_inode_addr + lx_sb->s_inodes_count * lx_sb->s_inode_size;
	lx_sb->s_root_sysdb_addr = lx_sb->s_root_direntry_addr + lx_sb->s_dentry_count * lx_sb->s_dentry_size;

	lx_sb->s_inode_size = sizeof(struct ltx_inode);
	lx_sb->s_free_blocks_count = lx_sb->s_blocks_count - 1;
	lx_sb->s_free_inodes_count = lx_sb->s_inodes_count - 3;
	lx_sb->s_free_dentry_count = lx_sb->s_dentry_count - 3;

	// Inode bitmap
	inode_bitmap = (uint32_t *)((uint32_t)lx_sb + lx_sb->s_inode_bitmap_addr);
	inode_bitmap[0] = 7;

	// Block bitmap
	block_bitmap = (uint32_t *)((uint32_t)lx_sb + lx_sb->s_block_bitmap_addr);
	block_bitmap[0] = 1;

	// Dentry bitmap
	dentry_bitmap = (uint32_t *)((uint32_t)lx_sb + lx_sb->s_dentry_bitmap_addr);
	dentry_bitmap[0] = 7;

	// Inode table
	in_root = (struct ltx_inode *)((uint32_t)lx_sb + lx_sb->s_root_inode_addr);
	in_root->i_block[0] = 0;
	in_root->i_block[1] = 1;
	in_root->i_block[2] = 2;
	in_root->i_mode = S_IFDIR;
	in_root->i_size = 3;
	in_root->i_parent = 0;
	in_root->i_child = 0;

	in_root++;
	in_root->i_block[0] = 1;
	in_root->i_mode = S_ISDBF;
	in_root->i_size = 1;
	in_root->i_parent = 0;
	in_root->i_child = 0;

	in_root++;
	in_root->i_block[0] = 2;
	in_root->i_mode = S_IFREG;
	in_root->i_size = 1;
	in_root->i_parent = 0;
	in_root->i_child = 0;

	// dentry table
	de_root = (struct ltx_direntry *)((uint32_t)lx_sb + lx_sb->s_root_direntry_addr);
	de_root->inode = 0;
	kstrcpy(de_root->name, "lost+found");
	de_root->rec_len = 0;
	de_root->name_len = 10;
	de_root->file_type = S_IFREG;

	de_root++;
	de_root->inode = 1;
	kmemcpy(de_root->name, "sys.dbase", 9);
	de_root->rec_len = 0;
	de_root->name_len = 9;
	de_root->file_type = S_ISDBF;

	de_root++;
	de_root->inode = 2;
	kstrcpy(de_root->name, "..", 2);
	de_root->rec_len = 0;
	de_root->name_len = 2;
	de_root->file_type = S_IFREG;

	ret = nrfs_put_data(NULL, (u_long)SUPER_BLOCK_OFF, KB(4), buff);
	kmemset(buff, 0, KB(4));
	printk("Format to ltxfs done.\r\n");

	return 0;
}

static struct disk_drive nrfs_ddr =
{
	.bdev.bd_dev = 100,
	.get_rowdat = nrfs_get_data,
	.put_rowdat = nrfs_put_data,
	.del_blocks = nrfs_del_blocks,
};

static int nrfs_nand_local_init(void)
{
	int ret;
	fs_config_t *fs_cfg = &nrfs_nand_fs_config;

	ret = fs_init();
	printk("Nrfs nand addr 0x%08x--0x%08x(%d Page).\r\n", 
				fs_cfg->p_start_addr, fs_cfg->p_end_addr, 
				((uint32_t)fs_cfg->p_end_addr - (uint32_t)fs_cfg->p_start_addr) / PAGE_SIZE);
	
	//nrfs_page_erase(SUPER_BLOCK_OFF, 1);
	nrfs_nand_check_format(kd_false);
	return 0;
}

int __init nrfs_nand_init(void)
{
	int ret;
	struct bdev_type bdev_type = {nrfs_ddr.bdev.bd_dev, &nrfs_ddr.bdev};

	nrfs_nand_local_init();
	
	ret = disk_drive_register(&bdev_type);
	wait_queue_register(&nrfs_nand_waiter_queue);

	return ret;
}

module_init(nrfs_nand_init);
