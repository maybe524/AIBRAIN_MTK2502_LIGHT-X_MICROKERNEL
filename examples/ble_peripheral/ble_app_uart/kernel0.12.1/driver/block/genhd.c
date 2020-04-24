#include "include/gendisk.h"
#include "include/malloc.h"
#include "include/errno.h"
#include "include/init.h"

#define MTD_SYS_PARENT	"/sys/mtd/"
#define MTD_DEV_NODE		"/dev/"

/*
 * http://blog.csdn.net/fudan_abc/article/details/1954261
 */
void register_disk(struct gendisk *disk)
{
	kobj_add_internal("", MTD_SYS_PARENT);
}

void add_disk(struct gendisk *disk)
{
	register_disk(disk);
}

struct gendisk *alloc_disk(int minors)
{
	struct gendisk *disk;

	disk = (struct gendisk *)kzalloc(sizeof(struct gendisk), GFP_NORMAL);
	if (!disk)
		return NULL;

	disk->node_id = NUMA_NO_NODE;	
	disk->major = minors;

	return disk;
}

int register_mtd_blktrans(void)
{
	struct gendisk *gd;

	gd = alloc_disk(0);
	if (!gd)
		return -ENOMEM;
	
	add_disk(gd);
}

int mtdblock_init(void)
{
	//register_mtd_blktrans();
}

module_init(mtdblock_init);
