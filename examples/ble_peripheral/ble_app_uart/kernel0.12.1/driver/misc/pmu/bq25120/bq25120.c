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
#include <include/msg.h>

#include "bq25120.h"

#define MASK_EN_SYS_OUT(n)	((n) << 7)
#define MASK_EN_LDO_OUT(n)	((n) << 7)
#define MASK_EN_ICH_OUT(n)	((n) << 1)

struct bq25120_info {
	struct i2c_client *client;
	__u8 ldout_ucnt;
};

extern int factory_items[7];
const struct i2c_board_info bq25120_board_info = {
					I2C_BOARD_INFO(BQ25120_I2C_TYPE, I2C_FLAGS_DATA_1BYTE | I2C_FLAGS_WRITE_DIRECTLY, BQ25120_I2C_ADDR)
				};
static struct bq25120_info bq25120_infos = {0};
static __u32 wathdog_detect_time = 0;
static __u8 bq25120_mode = 0;
static __u8 bq25120_stat = 0, bq25120_vinf = 0;
static kd_bool bq25120_ischarging = 0;

kd_bool bq25120_check_charging(void)
{
	return bq25120_ischarging;
}

int bq25120_set_lcsctrl(kd_bool switch_to)
{
	gpio_set_dir(BQ_LCSCTRL_ENABLE_PIN, GPIO_DIR_OUT, GPIO_CFG_NOPULL);
	gpio_set_out(BQ_LCSCTRL_ENABLE_PIN, switch_to);

	mdelay(50);

	return switch_to;
}

int bq25120_set_sysout(__u8 val)
{
	int ret;
	struct bq25120_info *bq = &bq25120_infos;
	__u8 need_enable = 1;	//(val == BQ_SYSOUT_0_0V ? 0 : 1);

	ret = i2c_write_bytes(bq->client, BQ25120_REG_SYSOUT_CTRL, MASK_EN_SYS_OUT(need_enable) | val);

	return ret;
}

int bq25120_set_ichrge(__u8 val)
{
	int ret;
	struct bq25120_info *bq = &bq25120_infos;
	__u8 need_enable = (val == BQ_ICHRGE_00_0MA ? 1 : 0);

	ret = i2c_write_bytes(bq->client, BQ25120_REG_FAST_CHARGE, MASK_EN_ICH_OUT(need_enable) | val);

	return ret;
}

int bq25120_set_ldout(__u8 val)
{
	int ret;
	struct bq25120_info *bq = &bq25120_infos;
	__u8 need_enable = (val == BQ_LDOOUT_0_0V ? 0 : 1);
	__u8 ldoctrl = 0;

	BQ_LOG("Set ldo out, val = 0x%x\r\n", val);

	if (!need_enable)
		ret = i2c_write_bytes(bq->client, BQ25120_REG_LDO_CTRL, MASK_EN_LDO_OUT(0) | val);
	else {
		ret = i2c_read_bytes(bq->client, BQ25120_REG_LDO_CTRL, &ldoctrl);
		if ((ldoctrl >> 2 & 0x1F) == val)
			return 0;
		else {
			ret = i2c_write_bytes(bq->client, BQ25120_REG_LDO_CTRL, MASK_EN_LDO_OUT(0) | val);
			if (need_enable)
				ret = i2c_write_bytes(bq->client, BQ25120_REG_LDO_CTRL, MASK_EN_LDO_OUT(1) | val);
		}
	}

	return ret;
}

int bq25120_lock_ldout(void)
{
	int ret;
	struct bq25120_info *bq = &bq25120_infos;

	bq->ldout_ucnt++;
	if (bq->ldout_ucnt == 1)
		ret = bq25120_set_ldout(BQ_LDOOUT_3_1V);

	printk("ldout_lock: %d\r\n", bq->ldout_ucnt);
	
	return 0;
}

int bq25120_unlock_ldout(void)
{
	int ret;
	struct bq25120_info *bq = &bq25120_infos;

	if (bq->ldout_ucnt) {
		bq->ldout_ucnt--;
		if (!bq->ldout_ucnt)
			ret = bq25120_set_ldout(BQ_LDOOUT_0_0V);
	}

	printk("ldout_unlock: %d\r\n", bq->ldout_ucnt);

	return 0;
}

static void bq25120_gpio_init(void)
{
	gpio_set_dir(BQ_CD_ENABLE_PIN, GPIO_DIR_OUT, 0);
	gpio_set_out(BQ_CD_ENABLE_PIN, GPIO_OUT_ONE);
	mdelay(5);
	bq25120_mode = BQ_ACTIVE_BATTERY_MODE;

	gpio_set_dir(BQ_MR_ENABLE_PIN, GPIO_DIR_OUT, 0);
	gpio_set_out(BQ_MR_ENABLE_PIN, GPIO_OUT_ONE);

	gpio_set_dir(BQ_TO_BLE_RESET_PIN, GPIO_DIR_IN, 0);
	gpio_set_dir(BQ_INT_PIN, GPIO_DIR_IN, 0);

	gpio_set_dir(BQ_INT_PIN, GPIO_DIR_IN, 0);
}

int bq25120_dump_reg(void)
{
	int ret = 0;
	struct bq25120_info *bq = &bq25120_infos;
	__u8 temp;

	BQ_LOG("Dump REG:\r\n");
	for (int idx = 0; idx < 0x0C; idx++) {
		ret = i2c_read_bytes(bq->client, idx, &temp);
		if (ret)
			goto dump_end;
		BQ_LOG("idx = 0x%02x, val = 0x%02x\r\n", idx, temp);
	}

dump_end:
	return ret;
}

static int bq25120_update_status(__u8 flags)
{
	int ret = 0;
	struct bq25120_info *bq = &bq25120_infos;
	__u8 regval = 0, idx;

	for (idx = 0; idx < BQ25120_REG_MAX; idx++) {
		ret = i2c_read_bytes(bq->client, idx, &regval);
		if (ret)
			bq25120_mode = BQ_HIZ_MODE;
		if (flags & BQ_DEBUG_OPEN)
			BQ_LOG("idx = 0x%02x, val = 0x%02x\r\n", idx, regval);
		switch (idx) {
			case BQ25120_REG_SCTRL:
				bq25120_stat = (regval >> 6) & 0x03;
				break;
			case BQ25120_REG_TSCTRL:
				bq25120_vinf = (regval >> 6) & 0x03;
				break;
			default:
				break;
		}
	}

	if (flags & BQ_DEBUG_OPEN)
		BQ_LOG("----end----\r\n");

	return ret;
}

static unsigned char __sched
		bq25120_mishande_thread(struct task_struct *task, void *data)
{
	int ret;
	struct bq25120_info *bq = &bq25120_infos;
	__u8 temp_step, temp_msg[20];

TASK_GEN_STEP_ENTRY(0) {
		if (!wathdog_detect_time)
			wathdog_detect_time = get_systime();
		if (get_systime() - wathdog_detect_time >= Second(10))
			temp_step = TASK_STEP(10);
		else if (bq25120_mode == BQ_HIZ_MODE)
			temp_step = TASK_STEP(20);
		else if (!(bq25120_vinf & BQ_VINFAULT_UNDR))
			temp_step = TASK_STEP(30);
		else
			return TSLEEP;
		task_utils(task, TCFG_SET_STEP, temp_step);
	}

TASK_GEN_STEP_ENTRY(10) {
		wathdog_detect_time = 0;
		ret = bq25120_update_status(BQ_DEBUG_OPEN);
		task_utils(task, TCFG_SET_STEP, TASK_STEP(0));
	}

TASK_GEN_STEP_ENTRY(20) {
		BQ_SET_CD_HIGH();
		mdelay(20);
		bq25120_update_status(0);

		if (bq25120_vinf & BQ_VINFAULT_UNDR) {
			BQ_LOG("Need to check to Battery mode!!!\r\n");
			bq25120_mode = BQ_ACTIVE_BATTERY_MODE;
			bq25120_ischarging = kd_false;
		}
		if (bq25120_mode != BQ_ACTIVE_BATTERY_MODE)
			BQ_SET_CD_LOW();

		task_utils(task, TCFG_SET_STEP, TASK_STEP(0));
	}

TASK_GEN_STEP_ENTRY(30) {
		BQ_LOG("Need Check to Charge mode!!!\r\n");

		bq25120_ischarging = kd_true;
		BQ_SET_CD_LOW();
		bq25120_mode = BQ_HIZ_MODE;
		task_utils(task, TCFG_SET_STEP, TASK_STEP(0));
	}

	return TSLEEP;
}
TASK_INIT(CMD_CHARGER, bq25120_mishande_thread, kd_true, NULL, 
		EXEC_TRUE,
		SCHED_PRIORITY_MIDE);

static int bq25120_probe(struct platform_device *dev)
{
	int ret;
	struct bq25120_info *bq = &bq25120_infos;

	BQ_LOG("Charger init\r\n");

	bq25120_gpio_init();
	bq25120_set_lcsctrl(kd_true);

	bq->client = i2c_register_board_info(BQ25120_I2C_PORT, &bq25120_board_info, 1);
	if (!bq->client)
		return -ENOMEM;

	i2c_write_bytes(bq->client, BQ25120_REG_BUVLOCTRL, 0x2A);
	i2c_write_bytes(bq->client, BQ25120_REG_VINDPM, 0x7A);

	ret = bq25120_set_sysout(BQ_SYSOUT_3_3V);
	ret = bq25120_set_ichrge(BQ_ICHRGE_90_0MA);

	bq25120_update_status(BQ_DEBUG_OPEN);
	sched_task_create(TASK_INFO(bq25120_mishande_thread), SCHED_COMMON_TYPE, EXEC_TRUE);

	return 0;
}

static int bq25120_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

static int bq25120_resume(struct platform_device *dev)
{
	return 0;
}

static struct platform_driver bq25120_pldrv = {
	.probe = bq25120_probe,
	.suspend = bq25120_suspend,
	.resume = bq25120_resume,
	.driver = {
		.name  = "bq25120_pl",
	}
};

static struct platform_device bq25120_pldev = {
	.name = "bq25120_pl",
};


int __init bq25120_init(void)
{
	int ret;
	struct bq25120_info *bq = &bq25120_infos;

	bq25120_set_lcsctrl(kd_true);

	ret = platform_device_register(&bq25120_pldev);
	if (ret)
		goto init_err;
	ret = platform_driver_register(&bq25120_pldrv);
	if (ret)
		goto init_err;

init_err:
	return 0;
}

module_init(bq25120_init);

MODULE_DESCRIPTION("This is Charger bq25120 driver on nrf platform");
MODULE_AUTHOR("xiaowen.huangg@gmail.com");
MODULE_LICENSE("GPL");

