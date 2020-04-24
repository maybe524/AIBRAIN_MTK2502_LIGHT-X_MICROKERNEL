#include "include/buffer.h"
#include "include/sched.h"
#include "include/stdio.h"
#include "include/init.h"
#include "include/types.h"
#include "include/block.h"
#include "include/errno.h"
#include "include/lock.h"
#include "include/task.h"
#include "include/time.h"
#include "include/assert.h"
#include "include/types.h"

#define BUFFER_HEAD_NUM 4

#define BUFF_DEBUG(fmt, argc...)		printk("[BUFFER] "fmt, ##argc);

static kd_bool buffer_request = kd_false;
struct bh_data bh2data[BUFFER_HEAD_NUM];
kd_bool buffer_update = kd_false;

LIST_HEAD(bh_free_list);
LIST_HEAD(bh_used_list);

void lock_buffer(struct buffer_head *bh)
{
	lock(&bh->b_lock);
}

void unlock_buffer(struct buffer_head *bh)
{
	unlock(&bh->b_lock);
}

int sync_request(void)
{
	if (buffer_update)
		return -EBUSY;
	else
		buffer_update = kd_true;

	return 0;
}

kd_bool sync_status(void)
{
	return buffer_update;
}

static struct buffer_head *find_used_buffer(short int dev, int block)
{
	struct buffer_head *bh;
	struct list_head *iter;

	list_for_each(iter, &bh_used_list) {
		bh = container_of(iter, struct buffer_head, bh_list);
		if (bh->b_dev == dev && \
				bh->b_blocknr == block)
			return bh;
	}

	return NULL;
}

struct buffer_head *find_free_buffer(void)
{
	struct buffer_head *bh;
	struct list_head *iter;

	list_for_each(iter, &bh_free_list) {
		bh = container_of(iter, struct buffer_head, bh_list);
			if (BH_IS_FREE(bh))
				return bh;
	}

	return NULL;
}

int sync_dev(struct task_struct *task,
		struct buffer_head *bh,
		int devt, int block)
{
	struct bio *bio;
	struct bh_data *bhd;

	if (!bh->b_dirty && bh->b_uptodate)
		return 0;

	while (check_lock(&bh->b_lock))
		sleep_on(task);
	lock(&bh->b_lock);

	bhd = container_of(bh, struct bh_data, bh);
	bio = bio_alloc();
	if (!bio) {
		unlock(&bh->b_lock);
		return -ENOMEM;
	}

	bio->wait = task;
	bio->bdev = bdev_by_devt(devt);

	/*
	*  NOTE:
	*    In order to reduce store/restore/erase count, we must think about it:
	*    1) if this page is have been erased(when alloc one page, it is alreay erased),
	*       just put all buffer data into it.
	*    2) if this page is no need erase, just put offset to one buffer's data to it.
	*/
	if (bh->b_dirty) {
		if (likely(bh->b_need_erase == BH_NEED_ERASE)) {
			bio->size = KB(4);
			bio->off  = block * KB(4);
			bio->data = bhd->data;

			submit_bio(ERASE, bio);
			submit_bio(WRITE, bio);
		}
		else {
			bio->size = ALIGN_UP(bhd->bh.b_msize, 4);
			bio->off  = block * KB(4) + bhd->bh.b_moff;
			bio->data = bhd->data + bhd->bh.b_moff;
			submit_bio(WRITE, bio);
		}

		bh->b_moff = 0;
		bh->b_msize = 0;
		bh->b_need_erase = 0;
		bh->b_dirty = kd_false;
 	}
	else if (!bh->b_uptodate) {
		bio->size = KB(4);
		bio->off  = block * KB(4);
		bio->data = bhd->data;
		submit_bio(READ, bio);
	}

	bh->b_uptodate = kd_true;
	bh->b_mtime = get_systime();
	bh->b_status &= ~BH_STAT_MARKBOFF_VALID;

	unlock(&bh->b_lock);
	bio_free(bio);

	return 0;
}

struct buffer_head *getbuff(short int dev, int block)
{
	struct buffer_head *tmp, *bh = NULL;

	if (!(bh = find_used_buffer(dev, block))) {
		struct list_head *iter;

		list_for_each(iter, &bh_free_list) {	/* try to seek out buffer in free list */
			tmp = container_of(iter, struct buffer_head, bh_list);
			if (tmp->b_blocknr == block)
				return tmp;
		}
	}

	return bh;
}

struct buffer_head *
getblk(struct task_struct *task, short int dev,
			int block)
{
	struct buffer_head *bh;

repeat:
	if (bh = getbuff(dev, block)) {
		if (BH_IS_FREE(bh))		/* buffer is found in free list */
			goto init;
		else
			return bh;
	}

	bh = find_free_buffer();
	if (!bh) {
		buffer_request = kd_true;
		sleep_on(task);
		goto repeat;
	}

	bh->b_uptodate = kd_false;
init:
	bh->b_wait = task;
	bh->b_dev = dev;
	bh->b_dirty = kd_false;
	bh->b_lock = kd_false;
	bh->b_blocknr = block;
	bh->b_mtime = get_systime();
	bh->b_moff = 0;
	bh->b_msize = 0;
	bh->b_need_erase = 0;
	bh->b_status = 0;
	bh->b_boff = 0;

	printk("Getblk new number %d\r\n", bh->b_blocknr);

	sync_dev(task, bh, dev, block);

	list_del(&bh->bh_list);
	list_add(&bh->bh_list, &bh_used_list);

	return bh;
}

int writeblk(struct buffer_head *bh,
		void *src, ssize_t moff, __size_t msize, umode_t mode)
{
	char *dest;
	__size_t boff = 0;

	if (moff + msize > KB(4))
		return -ERANGE;
	if (check_lock(&bh->b_lock))
		return -EBUSY;

	lock_buffer(bh);
	dest = (char *)(container_of(bh, struct bh_data, bh))->data + moff;

	if (bh->b_status & BH_STAT_MARKBOFF_VALID)
		boff = bh->b_boff;

	/*
	*  Dest equal to SRC is meaning update super-block most time,
	*  super-block is so big and no situation have other buffer filt that.
	*  In other words, ...
	*  otherwise meaning copy some byte from little buffer.
	*/
	if (dest != src) {
		kmemcpy(dest, src, msize);
		kmemset(dest + msize, 0, ALIGN_UP(boff + msize, 4) - (boff + msize));
	}

	bh->b_dirty = kd_true;
	bh->b_mtime = get_systime();
	bh->b_uptodate = kd_false;

	/*
	*  In order to protect flash(such as reduce write/erase count, etc),
	*  according to whether erased status, recorde modify size and offset value,
	*  so that, sync funtion can be know how to do it.
	*/
	if (!bh->b_need_erase) {
		if (mode & BLK_CLEAR || ALIGN_CHECK(boff, 4))
			clearblk(bh, BH_NEED_ERASE);
		else
			clearblk(bh, BH_NO_NEED_ERASE);
	}

	/*
	*  Some situation when no need erase,
	*  We need modify offset and write size.
	*/
	if (unlikely(bh->b_need_erase == BH_NEED_ERASE)) {
		bh->b_msize += msize;
		if (moff < bh->b_boff)				/* If modify palce is front of one buffer, mark it can be erased */
			clearblk(bh, BH_NEED_ERASE);
		else {
			/*
			*  Recode modify offset fisrt write.
			*  Maybe fix me ... some day..
			*/
			if (unlikely(bh->b_status & BH_STAT_MARKMOFF_VALID) && moff < KB(4)) {
				bh->b_status |= BH_STAT_MARKMOFF_VALID;
				bh->b_moff = ALIGN_DOWN(moff, 4);
			}
		}
	}

	unlock_buffer(bh);

	return msize;
}

int readblk(struct buffer_head *bh,
		void **dest, __size_t off, __size_t size)
{
	if (check_lock(&bh->b_lock))
		return -EBUSY;
	lock_buffer(bh);

	*dest = (char *)(container_of(bh, struct bh_data, bh))->data + off;
	bh->b_mtime = get_systime();

	unlock_buffer(bh);

	return size;
}

int setblk(struct buffer_head *bh, __u32 item, __u32 value, umode_t mode)
{
	int ret = 0;

	if (check_lock(&bh->b_lock))
		return -EBUSY;
	lock_buffer(bh);

	if (item & BH_STAT_MARKBOFF_VALID) {
		if ((bh->b_status & BH_STAT_MARKBOFF_VALID) && (mode & BH_SET_MODE_NO_REPET))
			ret = -EALREADY;
		else {
			bh->b_boff = value;
			bh->b_status |= BH_STAT_MARKBOFF_VALID;
		}
	}

	unlock_buffer(bh);

	return 0;
}

int clearblk(struct buffer_head *bh, umode_t mode)
{
	return bh->b_need_erase = mode;
}

static unsigned char __sched
buffer_thread(struct task_struct *task, void *data)
{
	__u32 mtime = 0;
	struct buffer_head *bh, *bhtmp = NULL;
	struct list_head *iter;

TASK_GEN_STEP_ENTRY(0) {
		/*
		*  TODO:
		*  check some situation follow:
		*	1) buffer update request command
		*	2) buffer request when some one want to getblk()
		*	3) low battery, decide to sync buffer data to flash
		*/
		list_for_each(iter, &bh_used_list) {
			task->data = bh = container_of(iter, struct buffer_head, bh_list);
			if (bh->b_dirty && (buffer_update || (get_systime() - bh->b_mtime == Second(5)))) {
				TASK_NEXT_STEP();
				BUFF_DEBUG("Block-%d need update.\r\n", bh->b_blocknr);
				return TAGAIN;
			}
			if (get_systime() - bh->b_mtime > Second(50)) {
				TASK_SET_STEP(2);
				return TAGAIN;
			}
		}
		if (iter == &bh_used_list && buffer_update)
			buffer_update = kd_false;

		// handle buffer request
		if (buffer_request)
			TASK_SET_STEP(3);

		return TSLEEP;
	}

TASK_GEN_STEP_ENTRY(1) {
		bh = task->data;
		if (check_lock(&bh->b_lock))
			return TSLEEP;
		sync_dev(task, bh, bh->b_dev, bh->b_blocknr);
		buffer_update = kd_false;

		TASK_SET_STEP(0);
		return TSLEEP;
	}

TASK_GEN_STEP_ENTRY(2) {
		bh = task->data;
		if (check_lock(&bh->b_lock))
			return TSLEEP;
		lock(&bh->b_lock);

		bh->b_mtime = 0;
		bh->b_dev = BH_UNUSED;

		list_del(&bh->bh_list);
		list_add(&bh->bh_list, &bh_free_list);
		unlock(&bh->b_lock);

		TASK_SET_STEP(0);
		return TSLEEP;
	}

TASK_GEN_STEP_ENTRY(3) {
		if (find_free_buffer()) {	/* find free list first */
			buffer_request = kd_false;
			TASK_SET_STEP(0);
			return TAGAIN;
		}

		list_for_each(iter, &bh_used_list) {
			__u32 time_sub;
			bhtmp = container_of(iter, struct buffer_head, bh_list);
			time_sub = get_timestamp() - bh->b_mtime;
			if (time_sub > mtime) {
				mtime = time_sub;
				bh = bhtmp;
			}
		}
		if (bh->b_dirty)
			sync_dev(task, bh, bh->b_dev, bh->b_blocknr);
		bh->b_mtime = 0;
		bh->b_dev = BH_UNUSED;
		buffer_request = kd_false;

		list_del(&bh->bh_list);
		list_add(&bh->bh_list, &bh_free_list);

		TASK_SET_STEP(0);
		return TSLEEP;
	}

	return TSLEEP;
}
TASK_INIT(CMD_BUFFER_HEAD, buffer_thread, kd_false,
		NULL, 
		EXEC_TRUE,
		SCHED_PRIORITY_HIGH);

int __init buffer_init(void)
{
	int i;

	kmemset(bh2data, 0, sizeof(struct bh_data) * BUFFER_HEAD_NUM);

	for (i = 0; i < BUFFER_HEAD_NUM; i++) {
		bh2data[i].bh.b_blocknr = BH_UNUSED;
		bh2data[i].bh.b_dev = BH_UNUSED;
		list_add(&bh2data[i].bh.bh_list, &bh_free_list);
	}

	sched_task_create(TASK_INFO(buffer_thread),
				SCHED_COMMON_TYPE,
				EXEC_TRUE);
	return 0;
}

module_init(buffer_init);
