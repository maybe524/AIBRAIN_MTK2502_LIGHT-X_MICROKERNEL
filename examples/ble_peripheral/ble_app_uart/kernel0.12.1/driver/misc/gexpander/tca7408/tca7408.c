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

#include "tca7408.h"
#include "bq25120.h"

//#define TCA_DEBUG_MODE

struct tca7408_info {
	struct i2c_client *client;
};

extern int factory_items[7];
const struct i2c_board_info tca7408_board_info = {
		I2C_BOARD_INFO(TCA7408_I2C_TYPE, I2C_FLAGS_DATA_1BYTE | I2C_FLAGS_WRITE_DIRECTLY, TCA7408_I2C_ADDR)};
static struct tca7408_info tca7408_infos = {0};


static int ___tca7408_gpio_set_dir(__u32 pin, __u8 dir, __u32 config)
{
	int ret;
	struct tca7408_info *tca = &tca7408_infos;
	__u8 dir_iomap, pul_iomap;

	if (!tca->client || pin >= TCA7408_GPIO_MAX || (dir > GPIO_DIR_OUT))
		return -ERANGE;

	/*
	 *	Set direction
	 */
	ret = i2c_read_bytes(tca->client, TCA7408_REG_IO_DIRECTION, &dir_iomap);
	if (ret)
		return -EIO;
	if (dir != ((dir_iomap >> pin) & 0x01)) {
		dir_iomap = (dir == GPIO_DIR_OUT ? (dir_iomap | (0x01 << pin)) : (dir_iomap &= ~(0x01 << pin)));
		ret = i2c_write_bytes(tca->client, TCA7408_REG_IO_DIRECTION, dir_iomap);
		if (ret)
			return -EIO;
	}

	/*
	 *	Set config, include pull down/up, HI enable/disable
	 */
	if (config) {
		if (config & (GPIO_CFG_PULLDOWN | GPIO_CFG_PULLUP)) {
			__u8 pul_val;
			kd_bool need_update = kd_false;

			ret = i2c_read_bytes(tca->client, TCA7408_REG_IO_PULL_SELET, &pul_iomap);
			if (ret)
				return ret;
			pul_val = (pul_iomap >> pin) & 0x01;
				
			if (config & GPIO_CFG_PULLDOWN && pul_val) {
				need_update = kd_true;
				pul_iomap |= 0x01 << pin;
			}
			else if (config & GPIO_CFG_PULLUP && !pul_val) {
				need_update = kd_true;
				pul_iomap &= ~(0x01 << pin);
			}

			if (need_update)
				ret = i2c_write_bytes(tca->client, TCA7408_REG_IO_PULL_SELET, pul_iomap);
		}

		if (config & (GPIO_CFG_OUT_HI_ENABLE | GPIO_CFG_OUT_HI_DISABLE)) {
			__u8 hi_iomap, hi_val;
			kd_bool need_update = kd_false;

			ret = i2c_read_bytes(tca->client, TCA7408_REG_IO_OUTPUT_HI, &hi_iomap);
			if (ret)
				return ret;
			hi_val = (hi_iomap >> pin) & 0x01;

			if (config & GPIO_CFG_OUT_HI_ENABLE && !hi_val) {
				need_update = kd_true;
				hi_iomap |= 0x01 << pin;
			}
			else if (config & GPIO_CFG_OUT_HI_DISABLE && hi_val) {
				need_update = kd_true;
				hi_iomap &= ~(0x01 << pin);
			}
			if (need_update)
				ret = i2c_write_bytes(tca->client, TCA7408_REG_IO_OUTPUT_HI, hi_iomap);
#ifdef TCA_DEBUG_MODE
			ret = i2c_read_bytes(tca->client, TCA7408_REG_IO_OUTPUT_HI, &hi_iomap);
			TCA_LOG("OUTPUT_HI = 0x%02x, need_update = %d\r\n", hi_iomap, need_update);
#endif
		}
	}

	return 0;
}

static int tca7408_gpio_set_dir(__u32 pin, __u8 dir, __u32 config)
{
	return ___tca7408_gpio_set_dir(pin & 0xFF, dir, config);
}

static int ___tca7408_gpio_set_out(__u32 pin, __u8 out)
{
	int ret;
	struct tca7408_info *tca = &tca7408_infos;
	__u8 out_iomap;

	if (!tca->client || pin >= TCA7408_GPIO_MAX || out > GPIO_OUT_ONE)
		return -ERANGE;
	
	ret = i2c_read_bytes(tca->client, TCA7408_REG_IO_OUTPUT_VAL, &out_iomap);
	if (ret)
		return -EIO;
	if (out == (out_iomap & (0x01 << pin)))
		return 0;
	out_iomap = (out == GPIO_OUT_ONE ? (out_iomap | (0x01 << pin)) : (out_iomap &= ~(0x01 << pin)));
	ret = i2c_write_bytes(tca->client, TCA7408_REG_IO_OUTPUT_VAL, out_iomap);

#ifdef TCA_DEBUG_MODE
	ret = i2c_read_bytes(tca->client, TCA7408_REG_IO_OUTPUT_VAL, &out_iomap);
	TCA_LOG("OUTPUT_VAL = 0x%02x\r\n", out_iomap);
#endif
	return ret;
}

static int tca7408_gpio_set_out(__u32 pin, __u8 out)
{
	return ___tca7408_gpio_set_out(pin & 0xFF, out);
}

int tca7408_local_init(void)
{
	int ret;
	struct tca7408_info *tca = &tca7408_infos;
	__u8 id;

	bq25120_lock_ldout();
	gpio_set_dir(TCA7408_RESET_PIN, GPIO_DIR_OUT, GPIO_CFG_NOPULL);
	
	TCA7408_RESET_HIGH();
	mdelay(5);
	TCA7408_RESET_LOW();
	mdelay(1);
	TCA7408_RESET_HIGH();

	ret = i2c_read_bytes(tca->client, TCA7408_REG_ID_AND_CTRL, &id);
	TCA_LOG("id = 0x%x, (correct id = 0x%x), ret = %d\r\n", id, TCA7408_ID_NUMBR, ret);
	if (id != TCA7408_ID_NUMBR || ret)
		return -EIO;

	factory_items[1] = 1;

	return 0;
}

static const struct gpio_operations tca7408_gpio_operations = {
	.set_dir = tca7408_gpio_set_dir,
	.set_out = tca7408_gpio_set_out,
};

static struct gpio_board_info tca7408_gpio_board_info = {
	.name = "tca7408_gpio",
	.rang_begin = 0x0100, 
	.rang_end	= 0x01FF - 1,
	.g_op	= &tca7408_gpio_operations,
};

static int tca7408_probe(struct platform_device *dev)
{
	int ret;
	struct tca7408_info *tca = &tca7408_infos;

	tca->client = i2c_register_board_info(TCA7408_I2C_PORT, &tca7408_board_info, 1);
	if (!tca->client)
		return -ENOMEM;

	ret = tca7408_local_init();
	if (ret < 0) {
		kfree(tca->client);
		goto err;
	}

	ret = gpio_register_board_info(&tca7408_gpio_board_info, 1);

err:
	TCA_LOG("TCA init %s\r\n", ret ? "FAIL" : "OK");

	return ret;
}

static int tca7408_suspend(struct platform_device *dev, pm_message_t state)
{
	bq25120_unlock_ldout();
	return 0;
}

static int tca7408_resume(struct platform_device *dev)
{
	return 0;
}

static struct platform_driver tca7408_pldrv = {
	.probe = tca7408_probe,
	.suspend = tca7408_suspend,
	.resume = tca7408_resume,
	.driver = {
		.name  = "tca7408_pl",
	}
};

static struct platform_device tca7408_pldev = {
	.name = "tca7408_pl",
};

int __init tca7408_init(void)
{
	int ret;

	ret = platform_device_register(&tca7408_pldev);
	if (ret)
		goto init_err;
	ret = platform_driver_register(&tca7408_pldrv);
	if (ret)
		goto init_err;

	return 0;

init_err:
	return 0;
}

module_init(tca7408_init);

MODULE_DESCRIPTION("This is GPIO expander driver on nrf platform");
MODULE_AUTHOR("xiaowen.huangg@gmail.com");
MODULE_LICENSE("GPL");
