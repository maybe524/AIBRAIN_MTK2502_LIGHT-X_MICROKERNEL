#include <io.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <mtd/mtd.h>
#include <mtd/nand.h>

static int check_pattern(__u8 *buf, int len, int paglen, struct nand_bad_blk *td)
{
	int i, end = 0;
	__u8 *p = buf;

	end = paglen + td->offs;
	if (td->flags & NAND_BBT_SCANEMPTY) {
		for (i = 0; i < end; i++) {
			if (p[i] != 0xff)
				return -1;
		}
	}
	p += end;

	for (i = 0; i < td->len; i++) {
		if (p[i] != td->pattern[i])
			return -1;
	}

	if (td->flags & NAND_BBT_SCANEMPTY) {
		p += td->len;
		end += td->len;
		for (i = end; i < len; i++) {
			if (*p++ != 0xff)
				return -1;
		}
	}

	return 0;
}

static int check_short_pattern(__u8 *buf, struct nand_bad_blk *td)
{
	int i;
	__u8 *p = buf;

	for (i = 0; i < td->len; i++) {
		if (p[td->offs + i] != td->pattern[i])
			return -1;
	}

	return 0;
}

static int read_bbt(struct nand_chip *nand,
				__u8 *buf,
				int page,
				int num,
				int bits,
				int offs,
				int reserved_block_code)
{
	int ret, i, j, act = 0;
	struct mtd_info *mtd = NAND_TO_FLASH(nand);
	size_t retlen, len, totlen;
	__u32 from;
	__u8 msk = (__u8) ((1 << bits) - 1);

	totlen = (num * bits) >> 3;
	from = ((__u32) page) << mtd->write_shift;

	while (totlen) {
		len = min(totlen, (__u32) (1 << nand->bbt_erase_shift));
		ret = mtd->read(mtd, from, len, &retlen, buf);

		if (ret < 0) {
			if (retlen != len) {
				printf("%s(): Error reading bad block table\n", __func__);
				return ret;
			}
			printf("%s(): ECC error while reading bad block table\n", __func__);
		}

		for (i = 0; i < len; i++) {
			__u8 dat = buf[i];

			for (j = 0; j < 8; j += bits, act += 2) {
				__u8 tmp = (dat >> j) & msk;

				if (tmp == msk)
					continue;
				if (reserved_block_code && (tmp == reserved_block_code)) {
					printf("%s(): Reserved block at 0x%08x\n",
						__func__,
						((offs << 2) + (act >> 1)) << nand->bbt_erase_shift
						);
					nand->bbt[offs + (act >> 3)] |= 0x2 << (act & 0x06);
					mtd->eccstat.bbtblocks++;
					continue;
				}

				printf("%s(): Bad block at 0x%08x\n", __func__,
					((offs << 2) + (act >> 1)) << nand->bbt_erase_shift);

				if (tmp == 0)
					nand->bbt[offs + (act >> 3)] |= 0x3 << (act & 0x06);
				else
					nand->bbt[offs + (act >> 3)] |= 0x1 << (act & 0x06);
				mtd->eccstat.badblocks++;
			}
		}

		totlen -= len;
		from += len;
	}
	return 0;
}

static int read_abs_bbt(struct nand_chip *nand, __u8 *buf, struct nand_bad_blk *td, int chip)
{
	struct mtd_info *mtd = NAND_TO_FLASH(nand);
	struct nand_ctrl *nfc = nand->master;
	int ret = 0, i;
	int bits;

	bits = td->flags & NAND_BBT_NRBITS_MSK;

	if (td->flags & NAND_BBT_PERCHIP) {
		int offs = 0;

		for (i = 0; i < nfc->slaves; i++) {
			if (chip == -1 || chip == i)
				ret = read_bbt(nand, buf, td->pages[i], mtd->chip_size >> nand->bbt_erase_shift, bits, offs, td->reserved_block_code);

			if (ret)
				return ret;

			offs += mtd->chip_size >> (nand->bbt_erase_shift + 2);
		}
	} else {
		ret = read_bbt(nand, buf, td->pages[0], mtd->chip_size >> nand->bbt_erase_shift, bits, 0, td->reserved_block_code);

		if (ret)
			return ret;
	}
	return 0;
}

static int scan_read_raw(struct nand_chip *nand,
					__u8 *buf, __u32 offs, __u32 len)
{
	struct mtd_oob_ops ops;
	struct mtd_info *mtd = NAND_TO_FLASH(nand);

	ops.mode    = FLASH_OOB_RAW;
	ops.ooboffs = 0;
	ops.ooblen  = mtd->oob_size;
	ops.oobbuf  = buf;
	ops.datbuf  = buf;
	ops.len  = len;

	return mtd->read_oob(mtd, offs, &ops);
}

static int scan_write_bbt(struct nand_chip *nand,
						__u32 offs,
						__u32 len,
						__u8 *buf,
						__u8 *oob
						)
{
	struct mtd_oob_ops ops;
	struct mtd_info *mtd = NAND_TO_FLASH(nand);

	ops.mode = FLASH_OOB_PLACE;
	ops.ooboffs = 0;
	ops.ooblen = mtd->oob_size;
	ops.datbuf = buf;
	ops.oobbuf = oob;
	ops.len = len;

	return mtd->write_oob(mtd, offs, &ops);
}

static int read_abs_bbts(struct nand_chip *nand,
						__u8 *buf,
						struct nand_bad_blk *td,
						struct nand_bad_blk *md
						)
{
	struct mtd_info *mtd = NAND_TO_FLASH(nand);

	if (td->flags & NAND_BBT_VERSION) {
		scan_read_raw(nand,
				buf,
				td->pages[0] << mtd->write_shift,
				mtd->write_size
				);
		td->version[0] = buf[mtd->write_size + td->veroffs];
		printf("Bad block table at page %d, version 0x%02X\n", td->pages[0], td->version[0]);
	}

	if (md && (md->flags & NAND_BBT_VERSION)) {
		scan_read_raw(nand, buf,
				md->pages[0] << mtd->write_shift,
				mtd->write_size);

		md->version[0] = buf[mtd->write_size + md->veroffs];
		printf("Bad block table at page %d, version 0x%02X\n", md->pages[0], md->version[0]);
	}

	return 1;
}

static int scan_block_full(struct nand_chip *nand,
						struct nand_bad_blk *bd,
						__u32 offs,
						__u8 *buf,
						__u32 readlen,
						int scanlen,
						int len
						)
{
	int ret, j;
	struct mtd_info *mtd = NAND_TO_FLASH(nand);

	ret = scan_read_raw(nand, buf, offs, readlen);
	if (ret)
		return ret;

	for (j = 0; j < len; j++, buf += scanlen) {
		if (check_pattern(buf, scanlen, mtd->write_size, bd))
			return 1;
	}

	return 0;
}

static int scan_block_fast(struct nand_chip *nand,
						struct nand_bad_blk *bd,
						__u32 offs,
						__u8 *buf,
						int len
						)
{
	struct mtd_oob_ops ops;
	int j, ret;
	struct mtd_info *mtd = NAND_TO_FLASH(nand);

	ops.ooblen = mtd->oob_size;
	ops.oobbuf  = buf;
	ops.ooboffs = 0;
	ops.datbuf  = NULL;
	ops.mode    = FLASH_OOB_PLACE;

	for (j = 0; j < len; j++) {
		/*
		 * read the full oob until read_oob is fixed to
		 * handle single byte reads for 16 bit
		 * buswidth
		 */
		ret = mtd->read_oob(mtd, offs, &ops);
		if (ret)
			return ret;

		if (check_short_pattern(buf, bd))
			return 1;

		offs += mtd->write_size;
	}

	return 0;
}

static int create_bbt(struct nand_chip *nand,
				__u8 *buf, struct nand_bad_blk *bd, int chip)
{
	struct mtd_info *mtd = NAND_TO_FLASH(nand);
	struct nand_ctrl *nfc = nand->master;
	int i, numblocks, len, scanlen;
	int startblock;
	__u32 from;
	__u32 readlen;

	if (bd->flags & NAND_BBT_SCANALLPAGES)
		len = 1 << (nand->bbt_erase_shift - mtd->write_shift);
	else {
		if (bd->flags & NAND_BBT_SCAN2NDPAGE)
			len = 2;
		else
			len = 1;
	}

	if (!(bd->flags & NAND_BBT_SCANEMPTY)) {

		scanlen = 0;
		readlen = bd->len;
	} else {

		scanlen = mtd->write_size + mtd->oob_size;
		readlen = len * mtd->write_size;
	}

	if (chip == -1) {
		/* Note that numblocks is 2 * (real numblocks) here, see i+=2
		 * below as it makes shifting and masking less painful */
		numblocks = mtd->chip_size >> (nand->bbt_erase_shift - 1);
		startblock = 0;
		from = 0;
	} else {
		if (chip >= nfc->slaves) {
			printf("create_bbt(): chipnr (%d) > available chips (%d)\n",
				chip + 1, nfc->slaves);
			return -EINVAL;
		}
		numblocks = mtd->chip_size >> (nand->bbt_erase_shift - 1);
		startblock = chip * numblocks;
		numblocks += startblock;
		from = startblock << (nand->bbt_erase_shift - 1);
	}

	for (i = startblock; i < numblocks; ) {
		int ret;

		if (bd->flags & NAND_BBT_SCANALLPAGES)
			ret = scan_block_full(nand, bd, from, buf, readlen,
						scanlen, len);
		else
			ret = scan_block_fast(nand, bd, from, buf, len);

		if (ret < 0)
			return ret;

		if (ret) {
			nand->bbt[i >> 3] |= 0x03 << (i & 0x6);
			printf("Bad eraseblock %d at 0x%08x\n", i >> 1, from);
			mtd->eccstat.badblocks++;
		}

		i += 2;
		from += (1 << nand->bbt_erase_shift);
	}
	return 0;
}

static int search_bbt(struct nand_chip *nand, __u8 *buf, struct nand_bad_blk *td)
{
	struct mtd_info *mtd = NAND_TO_FLASH(nand);
	struct nand_ctrl *nfc = nand->master;
	int i, chips;
	int startblock, block, dir;
	int scanlen = mtd->write_size + mtd->oob_size;
	int bbtblocks;
	int blocktopage = nand->bbt_erase_shift - mtd->write_shift;

	if (td->flags & NAND_BBT_LASTBLOCK) {
		startblock = (mtd->chip_size >> nand->bbt_erase_shift) - 1;
		dir = -1;
	} else {
		startblock = 0;
		dir = 1;
	}

	if (td->flags & NAND_BBT_PERCHIP) {
		chips = nfc->slaves;
		bbtblocks = mtd->chip_size >> nand->bbt_erase_shift;
		startblock &= bbtblocks - 1;
	} else {
		chips = 1;
		bbtblocks = mtd->chip_size >> nand->bbt_erase_shift;
	}

	// bits = td->flags & NAND_BBT_NRBITS_MSK;

	for (i = 0; i < chips; i++) {

		td->version[i] = 0;
		td->pages[i] = -1;

		for (block = 0; block < td->maxblocks; block++) {

			int actblock = startblock + dir * block;
			__u32 offs = actblock << nand->bbt_erase_shift;

			scan_read_raw(nand, buf, offs, mtd->write_size);

			if (!check_pattern(buf, scanlen, mtd->write_size, td)) {
				td->pages[i] = actblock << blocktopage;
				if (td->flags & NAND_BBT_VERSION)
					td->version[i] = buf[mtd->write_size + td->veroffs];

				break;
			}
		}
		startblock += mtd->chip_size >> nand->bbt_erase_shift;
	}

	for (i = 0; i < chips; i++) {
		if (td->pages[i] == -1)
			printf("Bad block table not found for chip %d\n", i);
		else
			printf("Bad block table found at page %d, version 0x%02X\n", td->pages[i],
				td->version[i]);
	}

	return 0;
}

static int search_read_bbts(struct nand_chip *nand,
							__u8 * buf,
							struct nand_bad_blk *td,
							struct nand_bad_blk *md)
{

	search_bbt(nand, buf, td);

	if (md)
		search_bbt(nand, buf, md);

	return 1;
}

static int write_bbt(struct nand_chip *nand,
				__u8 *buf,
				struct nand_bad_blk *td,
				struct nand_bad_blk *md,
				int chipsel)
{
	struct mtd_info *mtd = NAND_TO_FLASH(nand);
	struct nand_ctrl *nfc = nand->master;
	struct erase_info einfo;
	int i, j, ret, chip = 0;
	int bits, startblock, dir, page, offs, numblocks, sft, sftmsk;
	int nrchips, bbtoffs, pageoffs, oob_off;
	__u8 msk[4];
	__u8 rcode = td->reserved_block_code;
	size_t retlen, len = 0;
	__u32 to;
	struct mtd_oob_ops ops;

	ops.ooblen = mtd->oob_size;
	ops.ooboffs = 0;
	ops.datbuf = NULL;
	ops.mode = FLASH_OOB_PLACE;

	if (!rcode)
		rcode = 0xff;

	if (td->flags & NAND_BBT_PERCHIP) {
		numblocks = (int)(mtd->chip_size >> nand->bbt_erase_shift);

		if (chipsel == -1)
			nrchips = nfc->slaves;
		else {
			nrchips = chipsel + 1;
			chip = chipsel;
		}
	} else {
		numblocks = (int)(mtd->chip_size >> nand->bbt_erase_shift);
		nrchips = 1;
	}

	for (; chip < nrchips; chip++) {
		/* There was already a version of the table, reuse the page
		 * This applies for absolute placement too, as we have the
		 * page nr. in td->pages.
		 */
		if (td->pages[chip] != -1) {
			page = td->pages[chip];
			goto write;
		}

		if (td->flags & NAND_BBT_LASTBLOCK) {
			startblock = numblocks * (chip + 1) - 1;
			dir = -1;
		} else {
			startblock = chip * numblocks;
			dir = 1;
		}

		for (i = 0; i < td->maxblocks; i++) {
			int block = startblock + dir * i;

			switch ((nand->bbt[block >> 2] >> (2 * (block & 0x03))) & 0x03) {
			case 0x01:
			case 0x03:
				continue;
			}

			page = block << (nand->bbt_erase_shift - mtd->write_shift);

			if (!md || md->pages[chip] != page)
				goto write;
		}

		printf("No space left to write bad block table\n");
		return -ENOSPC;

write:
		bits = td->flags & NAND_BBT_NRBITS_MSK;
		msk[2] = ~rcode;
		switch (bits) {
		case 1: sft = 3; sftmsk = 0x07; msk[0] = 0x00; msk[1] = 0x01;
			msk[3] = 0x01;
			break;
		case 2: sft = 2; sftmsk = 0x06; msk[0] = 0x00; msk[1] = 0x01;
			msk[3] = 0x03;
			break;
		case 4: sft = 1; sftmsk = 0x04; msk[0] = 0x00; msk[1] = 0x0C;
			msk[3] = 0x0f;
			break;
		case 8: sft = 0; sftmsk = 0x00; msk[0] = 0x00; msk[1] = 0x0F;
			msk[3] = 0xff;
			break;
		default:
			return -EINVAL;
		}

		bbtoffs = chip * (numblocks >> 2);

		to = ((__u32) page) << mtd->write_shift;

		if (td->flags & NAND_BBT_SAVECONTENT) {
			to &= ~((__u32) ((1 << nand->bbt_erase_shift) - 1));
			len = 1 << nand->bbt_erase_shift;
			ret = mtd->read(mtd, to, len, &retlen, buf);
			if (ret < 0) {
				if (retlen != len) {
					printf("nand_bbt: Error reading block for writing "
						"the bad block table\n");
					return ret;
				}

				printf("nand_bbt: ECC error "
					"while reading block for writing "
					"bad block table\n");
			}

			ops.ooblen = (len >> mtd->write_shift) * mtd->oob_size;
			ops.oobbuf = &buf[len];
			ret = mtd->read_oob(mtd, to + mtd->write_size, &ops);
			if (ret < 0 || ops.oobretlen != ops.ooblen)
				goto outerr;

			pageoffs = page - (int)(to >> mtd->write_shift);
			offs = pageoffs << mtd->write_shift;

			memset(&buf[offs], 0xff, (__u32) (numblocks >> sft));
			oob_off = len + (pageoffs * mtd->oob_size);
		} else {
			len = (__u32) (numblocks >> sft);

			len = (len + (mtd->write_size - 1)) & ~(mtd->write_size - 1);

			memset(buf, 0xff, len +	(len >> mtd->write_shift) * mtd->oob_size);
			offs = 0;
			oob_off = len;

			memcpy(&buf[oob_off + td->offs], td->pattern, td->len);
		}

		if (td->flags & NAND_BBT_VERSION)
			buf[oob_off + td->veroffs] = td->version[chip];

		for (i = 0; i < numblocks;) {
			__u8 dat;
			dat = nand->bbt[bbtoffs + (i >> 2)];
			for (j = 0; j < 4; j++, i++) {
				int sftcnt = (i << (3 - sft)) & sftmsk;

				buf[offs + (i >> sft)] &=
					~(msk[dat & 0x03] << sftcnt);
				dat >>= 2;
			}
		}

		mtd->bad_allow = 1;
		memset(&einfo, 0, sizeof(einfo));
		einfo.addr = (__u32)to;
		einfo.len  = 1 << nand->bbt_erase_shift;
		ret = nand_erase(nand, &einfo);
		if (ret < 0)
			goto outerr;

		ret = scan_write_bbt(nand, to, len, buf, &buf[len]);
		if (ret < 0)
			goto outerr;

		printf("Bad block table written to 0x%08x, version "
			"0x%02X\n", (unsigned int)to, td->version[chip]);

		td->pages[chip] = page;
	}
	return 0;

 outerr:
	printf(
		"nand_bbt: Error while writing bad block table %d\n", ret);
	return ret;
}

static inline int nand_memory_bbt(struct nand_chip *nand, struct nand_bad_blk *bd)
{
	bd->flags &= ~NAND_BBT_SCANEMPTY;
	return create_bbt(nand, nand->buffers->data_buff, bd, -1);
}

static int check_create(struct nand_chip *nand, __u8 *buf, struct nand_bad_blk *bd)
{
	int i, chips, writeops, chipsel, ret;
	struct nand_ctrl *nfc = nand->master;

	struct nand_bad_blk *td = nand->bbt_td;
	struct nand_bad_blk *md = nand->bbt_md;
	struct nand_bad_blk *rd, *rd2;

	if (td->flags & NAND_BBT_PERCHIP)
		chips = nfc->slaves;
	else
		chips = 1;

	for (i = 0; i < chips; i++) {
		writeops = 0;
		rd = NULL;
		rd2 = NULL;

		chipsel = (td->flags & NAND_BBT_PERCHIP) ? i : -1;

		if (md) {
			if (td->pages[i] == -1 && md->pages[i] == -1) {
				writeops = 0x03;
				goto create;
			}

			if (td->pages[i] == -1) {
				rd = md;
				td->version[i] = md->version[i];
				writeops = 1;
				goto writecheck;
			}

			if (md->pages[i] == -1) {
				rd = td;
				md->version[i] = td->version[i];
				writeops = 2;
				goto writecheck;
			}

			if (td->version[i] == md->version[i]) {
				rd = td;
				if (!(td->flags & NAND_BBT_VERSION))
					rd2 = md;
				goto writecheck;
			}

			if (((char)(td->version[i] - md->version[i])) > 0) {
				rd = td;
				md->version[i] = td->version[i];
				writeops = 2;
			} else {
				rd = md;
				td->version[i] = md->version[i];
				writeops = 1;
			}

			goto writecheck;

		} else {
			if (td->pages[i] == -1) {
				writeops = 0x01;
				goto create;
			}
			rd = td;
			goto writecheck;
		}
	create:

		if (!(td->flags & NAND_BBT_CREATE))
			continue;

		create_bbt(nand, buf, bd, chipsel);

		td->version[i] = 1;
		if (md)
			md->version[i] = 1;
	writecheck:

		if (rd)
			read_abs_bbt(nand, buf, rd, chipsel);

		if (rd2)
			read_abs_bbt(nand, buf, rd2, chipsel);

		if ((writeops & 0x01) && (td->flags & NAND_BBT_WRITE)) {
			ret = write_bbt(nand, buf, td, md, chipsel);
			if (ret < 0)
				return ret;
		}

		if ((writeops & 0x02) && md && (md->flags & NAND_BBT_WRITE)) {
			ret = write_bbt(nand, buf, md, td, chipsel);
			if (ret < 0)
				return ret;
		}
	}
	return 0;
}

static void mark_bbt_region(struct nand_chip *nand, struct nand_bad_blk *td)
{
	struct mtd_info *mtd = NAND_TO_FLASH(nand);
	struct nand_ctrl *nfc = nand->master;

	int i, j, chips, block, nrblocks, update;
	__u8 oldval, newval;

	if (td->flags & NAND_BBT_PERCHIP) {
		chips = nfc->slaves;
		nrblocks = (int)(mtd->chip_size >> nand->bbt_erase_shift);
	} else {
		chips = 1;
		nrblocks = (int)(mtd->chip_size >> nand->bbt_erase_shift);
	}

	for (i = 0; i < chips; i++) {
		if ((td->flags & NAND_BBT_ABSPAGE) ||
			!(td->flags & NAND_BBT_WRITE)) {
			if (td->pages[i] == -1)
				continue;
			block = td->pages[i] >> (nand->bbt_erase_shift - mtd->write_shift);
			block <<= 1;
			oldval = nand->bbt[(block >> 3)];
			newval = oldval | (0x2 << (block & 0x06));
			nand->bbt[(block >> 3)] = newval;
			if ((oldval != newval) && td->reserved_block_code)
				nand_update_bbt(nand, block << (nand->bbt_erase_shift - 1));
			continue;
		}
		update = 0;
		if (td->flags & NAND_BBT_LASTBLOCK)
			block = ((i + 1) * nrblocks) - td->maxblocks;
		else
			block = i * nrblocks;
		block <<= 1;
		for (j = 0; j < td->maxblocks; j++) {
			oldval = nand->bbt[(block >> 3)];
			newval = oldval | (0x2 << (block & 0x06));
			nand->bbt[(block >> 3)] = newval;
			if (oldval != newval)
				update = 1;
			block += 2;
		}
		/* If we want reserved blocks to be recorded to mtd, and some
		   new ones have been marked, then we need to update the stored
		   bbts.  This should only happen once. */
		if (update && td->reserved_block_code)
			nand_update_bbt(nand, (block - 2) << (nand->bbt_erase_shift - 1));
	}
}

int nand_scan_bad_block(struct nand_chip *nand, struct nand_bad_blk *bd)
{
	struct mtd_info *mtd = NAND_TO_FLASH(nand);
	int len, ret = 0;
	__u8 *buf;
	struct nand_bad_blk *td = nand->bbt_td;
	struct nand_bad_blk *md = nand->bbt_md;

	len = mtd->chip_size >> (nand->bbt_erase_shift + 2);

	nand->bbt = zalloc(len);
	if (!nand->bbt) {
		DPRINT("%s(): Out of memory\n", __func__);
		return -ENOMEM;
	}

	/* If no primary table decriptor is given, scan the device
	 * to build a memory based bad block table
	 */
	if (!td) {
		if ((ret = nand_memory_bbt(nand, bd))) {
			printf("nand_bbt: Can't scan mtd and build the RAM-based BBT\n");
			free(nand->bbt);
			nand->bbt = NULL;
		}
		return ret;
	}

	len = (1 << nand->bbt_erase_shift);
	len += (len >> mtd->write_shift) * mtd->oob_size;
	buf = malloc(len);
	if (!buf) {
		printf("nand_bbt: Out of memory\n");
		free(nand->bbt);
		nand->bbt = NULL;
		return -ENOMEM;
	}

	if (td->flags & NAND_BBT_ABSPAGE) {
		ret = read_abs_bbts(nand, buf, td, md);
	} else {

		ret = search_read_bbts(nand, buf, td, md);
	}

	if (ret)
		ret = check_create(nand, buf, bd);

	mark_bbt_region(nand, td);
	if (md)
		mark_bbt_region(nand, md);

	free(buf);
	return ret;
}

int nand_update_bbt(struct nand_chip *nand, __u32 offs)
{
	struct mtd_info *mtd = NAND_TO_FLASH(nand);
	int len, ret = 0, writeops = 0;
	int chip, chipsel;
	__u8 *buf;
	struct nand_bad_blk *td = nand->bbt_td;
	struct nand_bad_blk *md = nand->bbt_md;

	if (!nand->bbt || !td)
		return -EINVAL;

	len = mtd->chip_size >> (nand->bbt_erase_shift + 2);

	len = (1 << nand->bbt_erase_shift);
	len += (len >> mtd->write_shift) * mtd->oob_size;
	buf = malloc(len);
	if (!buf) {
		printf("nand_update_bbt: Out of memory\n");
		return -ENOMEM;
	}

	writeops = md != NULL ? 0x03 : 0x01;

	if (td->flags & NAND_BBT_PERCHIP) {
		chip = (int)(offs >> mtd->chip_shift);
		chipsel = chip;
	} else {
		chip = 0;
		chipsel = -1;
	}

	td->version[chip]++;
	if (md)
		md->version[chip]++;

	if ((writeops & 0x01) && (td->flags & NAND_BBT_WRITE)) {
		ret = write_bbt(nand, buf, td, md, chipsel);
		if (ret < 0)
			goto out;
	}

	if ((writeops & 0x02) && md && (md->flags & NAND_BBT_WRITE)) {
		ret = write_bbt(nand, buf, md, td, chipsel);
	}

 out:
	free(buf);
	return ret;
}

static __u8 g_bb_patt_ff[] = {0xff, 0xff};

static struct nand_bad_blk g_bbSmallPageMemBased = {
	.flags = NAND_BBT_SCAN2NDPAGE,
	.offs = 5,
	.len = 1,
	.pattern = g_bb_patt_ff
};

static struct nand_bad_blk g_bbLargePageMemBased = {
	.flags = 0,
	.offs = 0,
	.len = 2,
	.pattern = g_bb_patt_ff
};

static struct nand_bad_blk g_bbSmallPageFlashBased = {
	.flags = NAND_BBT_SCAN2NDPAGE,
	.offs = 5,
	.len = 1,
	.pattern = g_bb_patt_ff
};

static struct nand_bad_blk g_bbLargePageFlashBased = {
	.flags = NAND_BBT_SCAN2NDPAGE,
	.offs = 0,
	.len = 2,
	.pattern = g_bb_patt_ff
};

static __u8 g_bb_patt_agand[] = {0x1C, 0x71, 0xC7, 0x1C, 0x71, 0xC7};

static struct nand_bad_blk g_bbt_agand_flash_based = {
	.flags = NAND_BBT_SCANEMPTY | NAND_BBT_SCANALLPAGES,
	.offs = 0x20,
	.len = 6,
	.pattern = g_bb_patt_agand
};

static __u8 g_bbt_main_patt[] = {'B', 'b', 't', '0' };
static __u8 g_bbt_mirror_patt[] = {'1', 't', 'b', 'B'};

static struct nand_bad_blk g_bbt_main_desc = {
	.flags = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
			| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs =	8,
	.len = 4,
	.veroffs = 12,
	.maxblocks = 4,
	.pattern = g_bbt_main_patt
};

static struct nand_bad_blk g_bbt_mirror_desc = {
	.flags = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
			| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs =	8,
	.len = 4,
	.veroffs = 12,
	.maxblocks = 4,
	.pattern = g_bbt_mirror_patt
};

int nand_scan_bbt(struct nand_chip *nand)
{
	struct mtd_info *mtd = NAND_TO_FLASH(nand);

	if (nand->flags & NAND_IS_AND) {
		if (!nand->bbt_td) {
			nand->bbt_td = &g_bbt_main_desc;
			nand->bbt_md = &g_bbt_mirror_desc;
		}

		nand->flags |= NAND_USE_FLASH_BBT;

		return nand_scan_bad_block(nand, &g_bbt_agand_flash_based);
	}

	if (nand->flags & NAND_USE_FLASH_BBT) {
		if (!nand->bbt_td) {
			nand->bbt_td = &g_bbt_main_desc;
			nand->bbt_md = &g_bbt_mirror_desc;
		}

		if (!nand->bad_blk_patt) {
			nand->bad_blk_patt = (mtd->write_size > 512) ?
				&g_bbLargePageFlashBased : &g_bbSmallPageFlashBased;
		}
	} else {
		nand->bbt_td = NULL;
		nand->bbt_md = NULL;

		if (!nand->bad_blk_patt) {
			nand->bad_blk_patt = (mtd->write_size > 512) ?
				&g_bbLargePageMemBased : &g_bbSmallPageMemBased;
		}
	}

	return nand_scan_bad_block(nand, nand->bad_blk_patt);
}

int nand_is_bad_bbt(struct nand_chip *nand, __u32 offs)
{
	struct mtd_info *mtd = NAND_TO_FLASH(nand);
	int block;
	__u8 ret;

	block = (int)(offs >> (nand->bbt_erase_shift - 1));
	ret = (nand->bbt[block >> 3] >> (block & 0x06)) & 0x03;

	DPRINT("%s(): bbt info for offs 0x%08x: (block %d) 0x%02x\n",
		__func__, offs,	block >> 1, ret);

	switch (ret) {
	case 0x00:
	case 0x01:
		return ret;

	case 0x02:
		return mtd->bad_allow ? 0 : 1;
	}

	return 1;
}

