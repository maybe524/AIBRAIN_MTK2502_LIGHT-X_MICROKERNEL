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

#include "pps960.h"
#include "bq25120.h"

#define PPS_DEBUG
#define PPS_TAG		"[PPS960]"
#define PPS_LOG(fmt, arg...)	printk(PPS_TAG fmt, ##arg)
#define PPS_IRQ_TYPE	"nrfs_irq"
#define PPS_MAJOR	2

struct init_reg {
	__u8	reg;
	__u32	val;
};

struct pps960_info {
	struct i2c_client *client;
	kd_bool is_debug_mode, is_measure_start;
	__u32	irq_cnt;
	struct task_struct *locked_by;
};

DECLARE_WAIT_QUEUE_HEAD(WAIT_QUEUE_PPS960, pps960_waiter_queue);
const struct i2c_board_info pps960_board_info = {
					I2C_BOARD_INFO(AFE4404_I2C_TYPE, I2C_FLAGS_DATA_3BYTE | I2C_FLAGS_WRITE_DIRECTLY, AFE4404_I2C_ADDR)
				};
const struct init_reg pps960_init_reg[] = {
	#include "pps960_init_reg.h"
};
static struct pps960_info pps960_infos = {0};

static unsigned char __sched
		___pps960_isr(struct task_struct *task, void *data)
{
	__u32 ir, red, green, amb, temp1, temp2;
	struct pps960_info *pps = &pps960_infos;
	__u8 msg_temp[50];

	if (!pps->is_measure_start)
		goto pps960_isr_end;

	i2c_read_bytes(pps->client, 0x2A, &ir);
	i2c_read_bytes(pps->client, 0x2B, &red);
	i2c_read_bytes(pps->client, 0x2C, &green);
	i2c_read_bytes(pps->client, 0x2D, &amb);
	i2c_read_bytes(pps->client, 0x2E, &temp1);
	i2c_read_bytes(pps->client, 0x2F, &temp2);
#if 0
	PPS_LOG("%06d, 0x%06x, RED = 0x%06x, GREEN = 0x%06x, AMB = 0x%06x, &0x2E = 0x%06x, &0x2F = 0x%06x\r\n",
			pps->irq_cnt,
			ir, red, green, amb, temp1, temp2);
#endif

	ksprintf(msg_temp, "MT%cRDT:%06x,%06x,%06x,%06x,%06x,%06x\r\n",
			pps->locked_by->t_id,
			ir, red, green, amb, temp1, temp2);

	msg_create(CMD_PPS960_ISR, CMD_MT2502_MSG, MSG_TO_RECEIVER_ONLINE, msg_temp, kstrlen(msg_temp));
	wake_up_interruptible(&pps960_waiter_queue);
	irq_unmask(AFE4404_IRQ_READY_PIN);

pps960_isr_end:
	return TSTOP;
}
TASK_INIT(CMD_PPS960_ISR, ___pps960_isr, kd_true, NULL, 
		EXEC_TRUE,
		SCHED_PRIORITY_MIDE);
static struct msg_user pps960_msg_user = {.task_id = CMD_PPS960_ISR, .task = TASK_INFO(___pps960_isr)};

static void pps960_isr(__u32 pin, __u32 action)
{
	struct pps960_info *pps = &pps960_infos;

	irq_mask(AFE4404_IRQ_READY_PIN);
	pps->irq_cnt++;
	sched_task_create(TASK_INFO(___pps960_isr), SCHED_NRFS_TYPE, EXEC_TRUE);
}

int pps960_local_init(void)
{
	int idx, ret;
	int data_len = ARRAY_ELEM_NUM(pps960_init_reg);
	struct pps960_info *pps = &pps960_infos;
	__u32 reg, val;

	PPS_LOG("pps local init\r\n");

	bq25120_lock_ldout();
	gpio_set_dir(AFE4404_RESET_PIN, GPIO_DIR_OUT, GPIO_CFG_NOPULL);
	gpio_set_dir(AFE4404_POWER_DVDD, GPIO_DIR_OUT, GPIO_CFG_OUT_HI_DISABLE);

	AFE4404_PWRON_VDD();
	mdelay(1);
	AFE4404_RESET_HIGH();
	mdelay(20);
	AFE4404_RESET_LOW();
	udelay(30);
	AFE4404_RESET_HIGH();
	mdelay(10);

	i2c_write_bytes(pps->client, AFE4404_CONTROL0, 0x08);
	mdelay(5);
	for (idx = 0; idx < data_len; idx++) {
		if (pps960_init_reg[idx].reg == 0xFF)
			mdelay(pps960_init_reg[idx].val);
		else {
			ret = i2c_write_bytes(pps->client,
						pps960_init_reg[idx].reg, pps960_init_reg[idx].val);
			if (ret)
				PPS_LOG("idx = %02d, reg = 0x%02x, val = 0x%06x, ret = %d\r\n",
						idx,
						pps960_init_reg[idx].reg,
						pps960_init_reg[idx].val,
						ret);
		}
	}
	mdelay(10);
	pps960_dump_regs();

	irq_register_isr(PPS_IRQ_TYPE,
				AFE4404_IRQ_READY_PIN,
				IRQ_CFG_TRIGGLE_RISING | IRQ_CFG_PULLDOWN | IRQ_CFG_HI_ACCURACY,
				pps960_isr);
	irq_mask(AFE4404_IRQ_READY_PIN);

	return ret;
}

int pps960_local_suspend(void)
{
	gpio_set_dir(AFE4404_RESET_PIN, GPIO_DIR_OUT, GPIO_CFG_NOPULL);
	gpio_set_dir(AFE4404_POWER_DVDD, GPIO_DIR_OUT, GPIO_CFG_OUT_HI_DISABLE);

	/*
	*  RESETZ/PWDN ¨Cfunction based on (active low) width of RESETZ pulse
	*  1) > 20 to 40 us width: RESET mode active
	*  2) > 200 us width: PWDN active
	*/
	AFE4404_RESET_LOW();
	udelay(300);
	AFE4404_PWROF_VDD();

	bq25120_unlock_ldout();

	return 0;
}

int pps960_dump_regs(void)
{
	int idx, ret;
	int data_len = ARRAY_ELEM_NUM(pps960_init_reg);
	__u32 reg, val;
	struct pps960_info *pps = &pps960_infos;

	i2c_write_bytes(pps->client, AFE4404_CONTROL0, 0x01);
	mdelay(5);
	for (idx = 0; idx < data_len; idx++) {
		ret = i2c_read_bytes(pps->client,
					pps960_init_reg[idx].reg, &val);
		PPS_LOG("idx = %02d, reg = 0x%02x, val = 0x%06x(%c= 0x%06x), ret = %d\r\n",
				idx,
				pps960_init_reg[idx].reg,
				val,
				pps960_init_reg[idx].val == val ? '=' : '!',
				pps960_init_reg[idx].val,
				ret);
	}

	return 0;
}

int pps960_collect_grayraw(struct task_struct *task, kd_bool on)
{
	struct pps960_info *pps = &pps960_infos;

	if (!task)
		return -EBUSY;
	switch (on) {
		case kd_true:
			pps->is_measure_start = kd_true;
			pps->locked_by = task;
			irq_unmask(AFE4404_IRQ_READY_PIN);
			break;
		case kd_false:
			if (pps->locked_by != task)
				return -EINVAL;
			pps->is_measure_start = kd_false;
			pps->locked_by = NULL;
			irq_mask(AFE4404_IRQ_READY_PIN);
			break;
	}
	pps->irq_cnt = 0;

	return 0;
}

static int pps960_ioctl(struct file *fp, int cmd, unsigned long param)
{
	struct pps960_info *pps = &pps960_infos;

	switch (cmd) {
		case PPS_LOCAL_STARTUP:
			pps->irq_cnt = 0;
			pps->is_measure_start = kd_true;
			pps->locked_by = (struct task_struct *)param;
			irq_unmask(AFE4404_IRQ_READY_PIN);
			break;
		case PPS_LOCAL_POWROFF:
			pps->is_measure_start = kd_false;
			irq_mask(AFE4404_IRQ_READY_PIN);
			break;
		default:
			break;
	}

	return 0;
}

static int pps960_open(struct file *fp, struct inode *in)
{
	return 0;
}

static int pps960_close(struct file *fp)
{
	return 0;
}

static const struct file_operations pps960_fops = {
	.open = pps960_open,
	.close = pps960_close,
	.ioctl = pps960_ioctl,
};

static int pps960_probe(struct platform_device *dev)
{
	int ret;
	struct pps960_info *pps = &pps960_infos;

	pps->client = i2c_register_board_info(AFE4404_I2C_PORT, &pps960_board_info, 1);
	if (!pps->client)
		return -ENOMEM;

	ret = msg_user_register(&pps960_msg_user);
	ret = wait_queue_register(&pps960_waiter_queue);

	ret = pps960_local_init();
	ret = register_chrdev(PPS_MAJOR, &pps960_fops, "gray");
	if (ret < 0)
		goto init_err;
	ret = cdev_create(MKDEV(PPS_MAJOR, 1), "gray");
	if (ret)
		goto init_err;
	ret = pps960_local_suspend();

init_err:
	return 0;
}

static int pps960_suspend(struct platform_device *dev, pm_message_t state)
{
	pps960_local_suspend();
	return 0;
}

static int pps960_resume(struct platform_device *dev)
{
	return 0;
}

static struct platform_driver pps960_pldrv = {
	.probe = pps960_probe,
	.suspend = pps960_suspend,
	.resume = pps960_resume,
	.driver = {
		.name  = "pps960_pl",
	}
};

static struct platform_device pps960_pldev = {
	.name = "pps960_pl",
};

int __init pps960_init(void)
{
	int ret;

	ret = platform_device_register(&pps960_pldev);
	if (ret)
		goto init_err;
	ret = platform_driver_register(&pps960_pldrv);
	if (ret)
		goto init_err;

	return 0;

init_err:
	return -EINVAL;
}

module_init(pps960_init);

MODULE_DESCRIPTION("This is i2c heart rate driver on nrf platform");
MODULE_AUTHOR("xiaowen.huangg@gmail.com");
MODULE_LICENSE("GPL");
