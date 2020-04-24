#include <include/init.h>
#include <include/types.h>
#include <include/stdio.h>
#include <i2c.h>
#include <include/errno.h>
#include <include/sched.h>
#include <include/task.h>
#include <include/platform.h>
#include <include/irq.h>
#include <include/gpio.h>
#include <include/fs.h>

#include "drv2605.h"
#include "bq25120.h"

struct drv2605_info {
	struct i2c_client *client;
};

struct init_reg {
	__u8	reg;
	__u32	val;
};

extern int factory_items[7];
const struct i2c_board_info drv2605_board_info = {
					I2C_BOARD_INFO(DRV2605_I2C_TYPE, I2C_FLAGS_DATA_1BYTE | I2C_FLAGS_WRITE_DIRECTLY, DRV2605_I2C_ADDR)
				};
static struct drv2605_info drv2605_infos = {0};
static const struct init_reg drv2605_init_reg[] = {
	#include "drv2605_init.h"
};

int drv2605_factory_test(void)
{
	int ret;
	struct drv2605_info *drv = &drv2605_infos;

	ret = i2c_write_bytes(drv->client, 0x0C, 0x01);
	return ret;
}

int drv2605_dump_regs(void)
{
	int idx, ret;
	int data_len = ARRAY_ELEM_NUM(drv2605_init_reg);
	__u32 reg, val;
	struct drv2605_info *drv = &drv2605_infos;

	for (idx = 0; idx < data_len; idx++) {
		ret = i2c_read_bytes(drv->client,
					drv2605_init_reg[idx].reg, &val);
		DRV_LOG("idx = %02d, reg = 0x%02x, val = 0x%02x(%c= 0x%02x), ret = %d\r\n",
				idx,
				drv2605_init_reg[idx].reg,
				val,
				drv2605_init_reg[idx].val == val ? '=' : '!',
				drv2605_init_reg[idx].val,
				ret);
	}

	return 0;
}

static int drv2605_local_init(void)
{
	int ret;
	int idx;
	int data_len = ARRAY_ELEM_NUM(drv2605_init_reg);
	struct drv2605_info *drv = &drv2605_infos;

	DRV_LOG("drv2605 reg init\r\n");

	for (idx = 0; idx < data_len; idx++) {
		if (drv2605_init_reg[idx].reg == 0xFF)
			mdelay(drv2605_init_reg[idx].val);
		else {
			ret = i2c_write_bytes(drv->client,
						drv2605_init_reg[idx].reg, drv2605_init_reg[idx].val);
			if (ret)
				DRV_LOG("idx = %02d, reg = 0x%02x, val = 0x%02x, ret = %d\r\n",
						idx,
						drv2605_init_reg[idx].reg,
						drv2605_init_reg[idx].val,
						ret);
		}
	}

	drv2605_dump_regs();

	ret = i2c_write_bytes(drv->client, 0x0C, 0x01);

	return 0;
}

static int drv2605_probe(struct platform_device *dev)
{
	int ret;
	struct drv2605_info *drv = &drv2605_infos;
	__u8 id = 0;

	drv->client = i2c_register_board_info(DRV2605_I2C_PORT, &drv2605_board_info, 1);
	if (!drv->client)
		return -ENOMEM;

	bq25120_lock_ldout();
	// Delay at least 250us
	udelay(300);

	// Active mode
	gpio_set_dir(EXGPIO_5, GPIO_DIR_OUT, GPIO_CFG_OUT_HI_DISABLE);
	gpio_set_out(EXGPIO_5, GPIO_OUT_ONE);

	// IN/TRIG connect to GDN
	gpio_set_dir(GPIO_17, GPIO_DIR_OUT, 0);
	gpio_set_out(GPIO_17, GPIO_OUT_ZERO);

	ret = i2c_read_bytes(drv->client, 0x00, &id);
	DRV_LOG("id = 0x%x, ID is %s.\r\n", id, DRV2605_CHIP_ID == id ? "OK" : "FAIL");

	if (!ret && DRV2605_CHIP_ID == id)
		factory_items[3] = 1;

	drv2605_local_init();

	return 0;
}

static int drv2605_suspend(struct platform_device *dev, pm_message_t state)
{
	bq25120_unlock_ldout();
	return 0;
}

static int drv2605_resume(struct platform_device *dev)
{
	return 0;
}

static struct platform_driver drv2605_pldrv = {
	.probe = drv2605_probe,
	.suspend = drv2605_suspend,
	.resume = drv2605_resume,
	.driver = {
		.name  = "drv2605_pl",
	}
};

static struct platform_device drv2605_pldev = {
	.name = "drv2605_pl",
};

int __init drv2605_init(void)
{
	int ret;

	ret = platform_device_register(&drv2605_pldev);
	if (ret)
		goto init_err;
	ret = platform_driver_register(&drv2605_pldrv);
	if (ret)
		goto init_err;

init_err:
	return 0;
}

module_init(drv2605_init);

MODULE_DESCRIPTION("This is Vibrator driver on nrf platform");
MODULE_AUTHOR("xiaowen.huangg@gmail.com");
MODULE_LICENSE("GPL");
