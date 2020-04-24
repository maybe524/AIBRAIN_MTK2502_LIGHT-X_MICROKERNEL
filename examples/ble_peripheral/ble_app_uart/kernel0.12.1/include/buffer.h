#pragma once
#include "include/list.h"

struct buffer_head {
	// __u8 *b_data;			/* pointer to data block (1024 bytes) */
	// __u16 ino;
	short b_blocknr;	/* block number */
	short b_dev;		/* device (0 = free) */
	__u8  b_count;		/* users using this block */
	kd_bool b_uptodate;
	kd_bool b_dirty;		/* 0-clean,1-dirty */
	kd_bool b_lock;		/* 0 - ok, 1 -locked */

	__u16 b_moff;		/* recode min offset */
	__u16 b_boff;		/* recode block already offset */
	__u16 b_msize;
	__u8  b_need_erase;
	__u8  b_status;

	struct task_struct	*b_wait;
	struct list_head bh_list;
	__u32 b_mtime;
};

struct bh_data {
	struct buffer_head bh;
	char data[KB(4)];
};

#define BLK_CLEAR	(1 << 0)
#define BLK_NO_CLEAR	(1 << 1)

#define BH_NEED_ERASE	1
#define BH_NO_NEED_ERASE	2

#define NR_HASH	3
#define BLOCK_SIZE	1024
#define BH_UNUSED -1

#define BH_SET_MODE_NO_REPET		(1 << 0)

#define BH_STAT_MARKBOFF_VALID		(1 << 0)
#define BH_STAT_MARKMOFF_VALID		(1 << 1)

#define BH_IS_FREE(v) ((v)->b_dev < 0)

struct buffer_head *getblk(struct task_struct *task, short int dev, int block);
struct buffer_head *find_free_buffer();
int clearblk(struct buffer_head *bh, umode_t mode);