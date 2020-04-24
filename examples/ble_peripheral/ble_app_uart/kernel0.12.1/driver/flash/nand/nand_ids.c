#include "include/mtd/nand.h"

const struct nand_desc g_nand_chip_desc[] = {
    NAND_CHIP_DESC("nRF52s",   0x33, 512, 16, 0x4000, 0),
    NAND_CHIP_DESC(NULL, 0, 0, 0, 0, 0)
};

const struct nand_vendor_name g_nand_vendor_id[] = {
    {NAND_MFR_TOSHIBA,  "Toshiba"},
    {0x0,               "Unknown"}
};
