#include "include/io.h"
#include "include/errno.h"
#include "include/delay.h"
#include "include/malloc.h"
#include "include/assert.h"
#include "include/string.h"
#include "include/bitops.h"
#include "include/mtd/mtd.h"
#include "include/mtd/nand.h"

#define BBT_PAGE_MASK	0xffffff3f

struct nand_ctrl *nand_ctrl_new(void)
{
	struct nand_ctrl *nfc;

	nfc = (struct nand_ctrl *)kzalloc(sizeof(struct nand_ctrl), GFP_NORMAL);
	if (!nfc)
		return NULL;

	INIT_LIST_HEAD(&nfc->nand_list);

	nfc->chip_delay     = 5;
	nfc->slaves         = 0;
	nfc->max_slaves     = 1;
	nfc->ecc_mode       = NAND_ECC_NONE;

	return nfc;
}

static struct nand_chip *new_nand_chip(struct nand_ctrl *master, int bus_id)
{
	struct nand_chip *nand;
	struct mtd_info *mtd;

	nand = (struct nand_chip *)kzalloc(sizeof(struct nand_chip), GFP_NORMAL);
	if (!nand)
		return NULL;

	mtd = NAND_TO_FLASH(nand);

	// mtd->part_tab = (struct partition *)malloc(sizeof(*mtd->part_tab) * MAX_FLASH_PARTS);

	nand->master = master;
	nand->bus_idx = bus_id;

	list_add_tail(&nand->nand_node, &master->nand_list);

	mtd->type = MTD_NANDFLASH;
	mtd->bad_allow = kd_true;

	return nand;
}

static void delete_nand_chip(struct nand_chip *nand)
{
	// struct mtd_info *mtd;

	list_del(&nand->nand_node);

	// mtd = NAND_TO_FLASH(nand);
	// fixme: free all buffer.
	// free(mtd->part_tab);

	kfree(nand);
}

struct nand_chip *nand_probe(struct nand_ctrl *nfc, int bus_idx)
{
	struct nand_chip *nand;

	nand = new_nand_chip(nfc, bus_idx);
	if (!nand)
		return NULL;

	return nand;
}

int nand_register(struct nand_chip *nand)
{
	int i, ret;
	struct mtd_info *mtd;
	struct nand_ctrl *nfc;
	char vendor_name[64];

	nfc = nand->master;
	mtd = NAND_TO_FLASH(nand);

	ksnprintf(mtd->name, sizeof(mtd->name), "%s.%d", nfc->name, nand->bus_idx);

	mtd->bdev.base = 0;
	mtd->bdev.size = mtd->chip_size;

	ret = flash_register(mtd);
	if (ret < 0) {
		printk("%s(): fail to register deivce!\n");
	}

	nfc->state = FL_READY;

	return 0;
}

int nand_ctrl_register(struct nand_ctrl *nfc)
{
	int idx, ret;
	struct nand_chip *nand;

	for (idx = 0; idx < nfc->max_slaves; idx++) {
		nand = nand_probe(nfc, idx);
		if (NULL == nand)
			continue;

		ret = nand_register(nand);
		if (ret < 0)
			return ret;
	}

	// printf("Total: %d nand %s detected\n", nfc->slaves, nfc->slaves > 1 ? "chips" : "chip");

	nfc->state = FL_READY;

	return nfc->slaves == 0 ? -ENODEV : nfc->slaves;
}
