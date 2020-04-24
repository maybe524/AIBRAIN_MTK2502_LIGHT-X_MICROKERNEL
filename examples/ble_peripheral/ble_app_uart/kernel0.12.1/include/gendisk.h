#pragma once

#include "include/device.h"

#define DISK_NAME_LEN	5

struct gendisk {
	/* major, first_minor and minors are input parameters only,
	 * don't use directly.  Use disk_devt() and disk_max_parts().
	 */
	int major;			/* major number of driver */
	int first_minor;
	int minors;                     /* maximum number of minors, =1 for
                                         * disks that can't be partitioned. */

	char disk_name[DISK_NAME_LEN];	/* name of major driver */
	char *(*devnode)(struct gendisk *gd, umode_t *mode);

	unsigned int events;		/* supported events */
	unsigned int async_events;	/* async events, subset of all */

	/* Array of pointers to partitions indexed by partno.
	 * Protected with matching bdev lock but stat and other
	 * non-critical accesses use RCU.  Always access through
	 * helpers.
	 */
	//struct disk_part_tbl __rcu *part_tbl;
	//struct hd_struct part0;

	//const struct block_device_operations *fops;
	//struct request_queue *queue;
	void *private_data;

	int flags;
	struct device *driverfs_dev;  // FIXME: remove
	//struct kobject *slave_dir;

	//struct timer_rand_state *random;
	//atomic_t sync_io;		/* RAID */
	//struct disk_events *ev;
#ifdef  CONFIG_BLK_DEV_INTEGRITY
	struct blk_integrity *integrity;
#endif
	int node_id;
};

#define NUMA_NO_NODE	(-1)