#include "include/list.h"
#include "include/stdio.h"
#include "include/string.h"
#include "include/block.h"
#include "include/assert.h"
#include "include/malloc.h"
#include "include/fs.h"
#include "include/drive.h"
#include "include/fs/devfs.h"
#include "include/errno.h"

#define MTD_BLK_PATCH	"/sys/mtd/"
#define MAX_BDEV		5	//256

int g_bdev_count = 0;
static LIST_HEAD(g_bdev_list);

struct block_device *bdev_get(const char *name)
{
	struct list_head *iter;

	list_for_each(iter, &g_bdev_list) {
		struct block_device *bdev;

		bdev = container_of(iter, struct block_device, bdev_node);
		if (!kstrcmp(bdev->name, name))
			return bdev;
	}

	return NULL;
}

struct block_device *bdev_by_devt(int devt)
{
	struct list_head *iter;

	list_for_each(iter, &g_bdev_list) {
		struct block_device *bdev;

		bdev = container_of(iter, struct block_device, bdev_node);
		if (bdev->bd_dev == devt)
			return bdev;
	}

	return NULL;
}

int bdev_register(struct block_device *bdev)
{
	int ret;

	list_add_tail(&bdev->bdev_node, &g_bdev_list);

	printk("Block info: 0x%08x - 0x%08x %s (%s)\r\n",
		bdev->base, bdev->base + bdev->size, bdev->name, bdev->label[0] ? bdev->label : "N/A");

	//ret = kobj_add_internal(bdev->name, MTD_BLK_PATCH);

	return ret;
}

int bdev_create(struct bdev_type *bdev_type)
{
	int ret;

	bdev_register(bdev_type->bdev);

	dev_mknod(bdev_type->bdev->name, 0666 | S_IFBLK, bdev_type->dev_t);

	g_bdev_count++;

	return 0;
}

struct bio *bio_alloc(void)
{
	struct bio *bio;

	bio = (struct bio *)kzalloc(sizeof(*bio), GFP_NORMAL);

	return bio;
}

void bio_free(struct bio *bio)
{
	kfree(bio);
}

void submit_bio(int rw, struct bio *bio)
{
	int ret;
	__size_t len;
	sector_t sect = bio->sect;
	struct block_device *bdev = bio->bdev;
	struct disk_drive *drive;

	assert(bdev);

	drive = container_of(bdev, struct disk_drive, bdev);

	for (;;) {
		switch (rw) {
			case READ:
				ret = drive->get_rowdat(drive, bio->off, bio->size, bio->data);
				break;
			case WRITE:
				ret = drive->put_rowdat(drive, bio->off, bio->size, bio->data);
				break;
			case ERASE:
				ret = drive->del_blocks(drive, bio->off, bio->size);
				break;
			default:
				assert(0);
		}

		if (!ret)
			break;
		else {
			bio->flags = 1;
			sleep_on(bio->wait);
			continue;
		}
	}
}

int devfs_bdev_open(struct file *fp, struct inode *inode)
{
	return 0;
}

