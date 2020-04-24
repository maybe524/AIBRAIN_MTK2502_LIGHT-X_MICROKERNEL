#include "include/stdio.h"
#include "include/errno.h"
#include "include/string.h"
#include "include/malloc.h"
#include "include/assert.h"
#include "include/fcntl.h"
#include "include/fs.h"
#include "include/mtd/mtd.h"

// fixme

int set_bdev_file_attr(struct file *fp)
{
#if 0
	char file_attr[CONF_ATTR_LEN];
	char file_val[CONF_VAL_LEN];
	struct block_device *bdev;

	assert(fp != NULL);

	bdev = fp->bdev;

	// set fp name
	snprintf(file_attr, CONF_ATTR_LEN, "bdev.%s.image.name", bdev->name);
	if (conf_set_attr(file_attr, fp->name) < 0) {
		conf_add_attr(file_attr, fp->name);
	}

	// set fp size
	snprintf(file_attr, CONF_ATTR_LEN, "bdev.%s.image.size", bdev->name);
	val_to_dec_str(file_val, fp->size);
	if (conf_set_attr(file_attr, file_val) < 0) {
		conf_add_attr(file_attr, file_val);
	}
#endif
	return 0;
}

const struct file_operations flash_fops = {
	.open  = NULL,
	.read  = NULL,
	.write = NULL,
	.close = NULL,
	.ioctl = NULL,
};

int flash_fops_init(struct block_device *bdev)
{
	bdev->fops = &flash_fops;

	return 0;
}
