#pragma once

#include "include/list.h"
#include "include/block.h"

struct disk_drive {
	struct block_device bdev;

	__size_t sect_size;

	int (*get_rowdat)(struct disk_drive *drive, u_long start, __size_t size, void *buff);
	int (*put_rowdat)(struct disk_drive *drive, u_long start, __size_t size, const void *buff);
	int (*del_blocks)(struct disk_drive *drive, u_long start, __size_t size);
};

int disk_drive_register(struct bdev_type *bd_type);
