#include "include/io.h"
#include "include/init.h"
#include "include/delay.h"
#include "include/errno.h"
#include "include/string.h"
#include "include/malloc.h"
#include "include/mtd/mtd.h"
#include "include/types.h"
#include "include/stdio.h"
#include "include/sched.h"
#include "include/task.h"

static LIST_HEAD(g_master_list);
static int g_flash_count = 0;

/*
 * g-bios flash partition definition
 * original from Linux kernel (driver/mtd/cmdlinepart.c)
 *
 * mtdparts  := <mtddef>[;<mtddef]
 * <mtddef>  := <mtd-id>:<partdef>[,<partdef>]
 *              where <mtd-id> is the name from the "cat /proc/mtd" command
 * <partdef> := <size>[@offset][<name>][ro][lk]
 * <mtd-id>  := unique name used in mapping driver/device (mtd->name)
 * <size>    := standard linux memsize or "-" to denote all remaining space
 * <name>    := '(' NAME ')'
 *
 * Examples:  1 NOR Flash with 2 partitions, 1 NAND with one
 * edb7312-nor:256k(ARMboot)ro,-(root);edb7312-nand:-(home)
 *
 *
 * fixme:
 * 1. to fix cross/overlapped parts
 * 2. as an API and called from flash core
 * 3. to support <partdef> := <size>[@offset][(<label>[, <image_name>, <image_size>])]
 * 4. handle exception
 *
 */

static int __init flash_parse_part(struct mtd_info *host,
						struct part_attr *part,	const char *part_def)
{
#if 0
	int i, ret = -EINVAL, index = 0;
	__u32 curr_base = 0;
	char buff[128];
	const char *p;

	p = u_strchr(part_def, ':');
	if (!p)
		goto error;

	p++;

	while (*p && *p != ';') {
		if (curr_base >= host->chip_size)
			goto error;

		while (' ' == *p) p++;

		// part size
		if (*p == '-') {
			part->size = host->chip_size - curr_base;
			p++;
		} else {
			for (i = 0; *p; i++, p++) {
				if (*p == '@' || *p == '(' || *p == 'r' || *p == ',')
					break;
				buff[i] = *p;
			}
			buff[i] = '\0';

			ret = hr_str_to_val(buff, (unsigned long *)&part->size);
			if (ret < 0)
				goto error;

			ALIGN_UP(part->size, host->erase_size);
		}

		// part base
		if (*p == '@') {
			for (i = 0, p++; *p; i++, p++) {
				if (*p == '(' || *p == 'r' || *p == ',')
					break;
				buff[i] = *p;
			}
			buff[i] = '\0';

			ret = hr_str_to_val(buff, (unsigned long *)&part->base);
			if (ret < 0)
				goto error;

			ALIGN_UP(part->base, host->erase_size);

			curr_base = part->base;
		} else {
			part->base = curr_base;
		}

		// part label and image
		i = 0;
		if (*p == '(') {
			for (p++; *p && *p != ')'; i++, p++)
				part->label[i] = *p;

			p++;
		}

		if (*p == 'r') {
			p++;
			if (*p != 'o') {
				return -EINVAL;
			}

			part->flags |= BDF_RDONLY;
			p++;
		}

		if (',' == *p)
			p++;

		part->label[i] = '\0';

		curr_base += part->size;

		index++;
		part++;
	}

	return index;

error:
	printk("%s(): invalid part definition \"%s\"\n", __func__, part_def);
	return ret;
#endif
}

static __size_t remove_blank_space(const char *src, char *dst, __size_t size)
{
	char *p = dst;

	while (*src) {
		if (*src != ' ') {
			*p = *src;
			p++;
		}

		src++;
	}

	*p = '\0';

	return p - dst;
}

static int __init flash_scan_part(struct mtd_info *host,
		struct part_attr part[])
{
	int ret;
	char part_def[CONF_VAL_LEN];
	char conf_val[CONF_VAL_LEN];
	const char *match_parts;

	// ret = conf_get_attr("flash.parts", conf_val);
	if (ret < 0) {
		return ret;
	}

	match_parts = kstrstr(conf_val, host->name);
	if (!match_parts) {
		// TODO: add hint here
		return -ENOENT;
	}

	remove_blank_space(match_parts, part_def, sizeof(part_def));

	return flash_parse_part(host, part, part_def);
}

static int part_read(struct mtd_info *slave,
				__u32 from, __u32 len, __size_t *retlen, __u8 *buff)
{
	struct mtd_info *master = slave->ls.master;

	return master->read(master, slave->bdev.base + from, len, retlen, buff);
}

static int part_write(struct mtd_info *slave,
				__u32 to, __u32 len, __u32 *retlen, const __u8 *buff)
{
	struct mtd_info *master = slave->ls.master;

	return master->write(master, slave->bdev.base + to, len, retlen, buff);
}

static int part_erase(struct mtd_info *slave, struct erase_info *opt)
{
	struct mtd_info *master = slave->ls.master;

	opt->addr += slave->bdev.base;
	return master->erase(master, opt);
}

int flash_register(struct mtd_info *mtd)
{
	int ret;
	struct mtd_info *slave;

	list_add_tail(&mtd->ln.master_node, &g_master_list);

	ksnprintf(mtd->bdev.name, MAX_DEV_NAME, BDEV_NAME_FLASH "%c", '0' + g_flash_count);
	kstrncpy(mtd->bdev.label, "mtd", sizeof(mtd->bdev.label));

	ret = bdev_register(&mtd->bdev);	// disk_drive_register
	if (ret < 0)
		return -EINVAL;

	g_flash_count++;

	return ret;
}

int flash_unregister(struct mtd_info *mtd)
{
	// TODO: check master or not

	return 0;
}

struct mtd_info *get_mtd_device(void *nil, unsigned int num)
{
	unsigned int i = 1;
	struct list_head *iter;
	struct mtd_info *master, *mtd;
#if 0
	list_for_each_entry(master, &g_master_list, ln.master_node)
		list_for_each(iter, &master->ls.slave_list) {
			if (i == num) {
				mtd = container_of(iter, struct mtd_info, ln.slave_node);
				return mtd;
			}
			i++;
		}
#endif
	return NULL;
}


unsigned char flash_service(struct task_struct *task, void *data)
{
	return TSTOP;
}
TASK_INIT(CMD_FLASH, flash_service, kd_false, NULL, 
		EXEC_TRUE,
		SCHED_PRIORITY_HIGH);

static int __init flash_init(void)
{
	return 0;
}

subsys_init(flash_init);
