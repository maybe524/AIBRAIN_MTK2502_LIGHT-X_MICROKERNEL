#pragma once

#define DRV_TAG		"[DRV2605]"
#define DRV_LOG(fmt, arg...)		printk(DRV_TAG fmt, ##arg)

#define DRV2605_I2C_TYPE	"drv2605"
#define DRV2605_I2C_ADDR	0x5A
#define DRV2605_I2C_PORT	0x01
#define DRV2605_CHIP_ID		0x60

