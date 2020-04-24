#include "include/init.h"
#include "include/stdio.h"
#include "include/errno.h"
#include "include/assert.h"
#include "include/block.h"
#include "include/drive.h"
#include "include/malloc.h"
#include "include/string.h"

extern int g_bdev_count;

int disk_drive_register(struct bdev_type *bd_type)
{
	int ret;

	ksnprintf(bd_type->bdev->name, 10, BDEV_NAME_FLASH "%c", '0' + g_bdev_count);
	kstrncpy(bd_type->bdev->label, "mtd", sizeof(bd_type->bdev->label));

	printk("Register disk drive \"%s\", devt = %d.\r\n", bd_type->bdev->name, bd_type->dev_t);

	ret = bdev_create(bd_type);

	return ret;
}
