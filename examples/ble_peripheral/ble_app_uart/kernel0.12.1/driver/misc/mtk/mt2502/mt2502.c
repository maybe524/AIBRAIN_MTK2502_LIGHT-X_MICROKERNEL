#include <include/stdio.h>
#include <include/errno.h>
#include <include/gpio.h>
#include <include/delay.h>
#include <include/errno.h>
#include <include/lock.h>
#include <include/time.h>
#include <include/init.h>
#include <include/sched.h>
#include <include/task.h>
#include <include/msg.h>
#include <include/wait.h>
#include <include/platform.h>

#include "mt2502.h"
#include "bq25120.h"

#define MT2502_LOAD_SWITCH_EN		EXGPIO_1
#define MT2502_POWER_KEY			GPIO_24

#define MT2502_POWERKEY_RELEASE()	gpio_set_out(MT2502_POWER_KEY, GPIO_OUT_ZERO);
#define MT2502_POWERKEY_PRESS()		gpio_set_out(MT2502_POWER_KEY, GPIO_OUT_ONE);

#define MT2502_LOAD_SWITCH_ON()		gpio_set_out(MT2502_LOAD_SWITCH_EN, GPIO_OUT_ONE);
#define MT2502_LOAD_SWITCH_OFF()	gpio_set_out(MT2502_LOAD_SWITCH_EN, GPIO_OUT_ZERO);

#define mt2502_atcmd(fmt, argc...)	do {	\
				__u8 at_buf[50] = {0};	\
				ksprintf(at_buf, "\r\n"fmt"\r\n", ##argc);	\
				kprintf(at_buf);\
			} while (0);

#define MT2502_TAG		"[MT2502] "
#define MT2502_LOG(fmt, arg...)		printk(MT2502_TAG fmt, ##arg)

struct mt2502_status mt2502_kd_status = {0};
static kd_bool mt2502_poweron_abnormal = kd_false;
static __u8 user_infos[20] = {0};
static kd_bool is_lock_ldo = kd_false, is_unlock_ldo = kd_false;
static __u8 msg_array[20] = {0};
static __u8 mt2502_version[4] = {0};
static kd_bool mt2502_ver_isvalid = kd_false;

int mt2502_set_userinfo(__u8 *buff)
{
	kmemset(user_infos, 0, sizeof(user_infos));
	kmemcpy(user_infos, buff, kstrlen(buff));

	return 0;
}

int mt2502_get_version(__u8 *buff)
{
	if (!mt2502_ver_isvalid)
		return -EINVAL;
	kmemcpy(buff, mt2502_version, sizeof(mt2502_version) - 1);

	return 0;
}

int mt2502_power_set(__u8 turn_to)
{
    if (turn_to == POWER_ON)
        mt2502_kd_status.curr_use_count++;
    else if (turn_to == POWER_OFF && mt2502_kd_status.curr_use_count > 0)
        mt2502_kd_status.curr_use_count--;

	if (mt2502_kd_status.curr_use_count && !is_lock_ldo) {
		is_lock_ldo = kd_true;
		is_unlock_ldo = kd_false;
		bq25120_lock_ldout();
	}
	else if (!mt2502_kd_status.curr_use_count && !is_unlock_ldo) {
		is_unlock_ldo = kd_true;
		is_lock_ldo = kd_false;
		bq25120_unlock_ldout();
	}

	return mt2502_kd_status.curr_use_count;
}

uint8_t mt2502_power_get(void)
{
    return mt2502_kd_status.curr_status;
}

static unsigned char mt2502_poweron_thread(struct task_struct *task, void *argc)
{
	int ret;
	__u8 atcmd[40], temp_step;
	struct msg_msg *msg;
	__u32 temp_cnfg = 0;
	struct mt2502_status *status = &mt2502_kd_status;

TASK_GEN_STEP_ENTRY(0) {
		if (status->curr_use_count > 0 && \
				(status->curr_status == TSTOP || status->curr_status == TDEAD))
		{
			if (mt2502_poweron_abnormal) {
				if (!task->t_stime_temp)
					task->t_stime_temp = get_systime();
				if (get_systime() - task->t_stime_temp <= Second(2))
					return TAGAIN;
			}
			mt2502_poweron_abnormal = kd_false;
			lock(&status->lock);

			MT2502_LOAD_SWITCH_ON();
			mdelay(1);
			MT2502_POWERKEY_PRESS();

			task_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP, 0);

			mt2502_task_msg_clear(task);
			mt2502_task_msg_register(task);
		}
		else
			return TSLEEP;
	}

TASK_GEN_STEP_ENTRY(1) {
		ret = task_wait_timeout(task, Second(8));
		msg = mt2502_task_msg_check(task);
		if (ret)
			temp_step = task->t_step + 1;
		else if (msg)
			temp_step = TASK_STEP(3);
		else
			return TSLEEP;
		task_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_STEP, temp_step);

		MT2502_POWERKEY_RELEASE();
	}

TASK_GEN_STEP_ENTRY(2) {
		ret = task_wait_timeout(task, Second(6));
		msg = mt2502_task_msg_check(task);
		if (ret) {
			temp_cnfg = TCFG_CLEAR_MSG;
			mt2502_atcmd("AT+CMD_MT2502_POWERON");
		}
		else if (msg) {}
		else
			return TSLEEP;
		task_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP | temp_cnfg, 0);
	}

TASK_GEN_STEP_ENTRY(3) {
		msg = mt2502_task_msg_check(task);
		if (msg) {
			if (!kstrncmp("PWO:OK", &msg->msg_comment[3], 6)) {
				temp_cnfg = TCFG_CLEAR_TRYCNT;
				temp_step = task->t_step + 1;
				kmemcpy(mt2502_version, &msg->msg_comment[10], 3);
				mt2502_ver_isvalid = kd_true;
				MT2502_LOG("VER: %s\r\n", mt2502_version);
			}
		}
		else if (task->t_trycnt < Trycnt(3)) {
			ret = task_wait_timeout(task, Second(3));
			if (!ret)
				return TAGAIN;
			MT2502_LOAD_SWITCH_OFF();
			mt2502_poweron_abnormal = kd_true;
			task_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_STEP, TASK_STEP(0));
			return TAGAIN;
		}
		else {
			ret = task_wait_timeout(task, Second(3));
			if (!ret)
				return TSLEEP;
			temp_cnfg = 0;
			temp_step = TASK_STEP(88);
		}

		task_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_STEP, temp_step);
	}

TASK_GEN_STEP_ENTRY(4) {
		mt2502_atcmd("AT+SYNCTIME=%08X%02X", get_timestamp(), get_timezone());
		task_utils(task, TCFG_CLEAR_TIMEOUT | 
						TCFG_SET_NEXT_STEP | TCFG_CLEAR_MSG,
					0);
	}

TASK_GEN_STEP_ENTRY(5) {
		ret = task_wait_timeout(task, Second(2));
		msg = mt2502_task_msg_check(task);
		if (msg && !kstrncmp("TIM:OK", &msg->msg_comment[3], 6))
			temp_cnfg = TCFG_SET_NEXT_STEP | TCFG_CLEAR_MSG;
		else if (ret) {
			temp_cnfg = TCFG_SET_STEP;
			temp_step = TASK_STEP(88);
		}
		else
			return TSLEEP;
		task_utils(task, TCFG_CLEAR_TIMEOUT | temp_cnfg, temp_step);
	}

TASK_GEN_STEP_ENTRY(6) {
		if (user_infos[0])
			mt2502_atcmd("AT+SETUSRINFO=%s", user_infos);
		mdelay(20);

		task_utils(task, TCFG_SET_NEXT_STEP, 0);
	}

TASK_GEN_STEP_ENTRY(7) {
		ret = task_wait_timeout(task, Second(1));
		if (ret) {
			task_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_STEP, TASK_STEP(0));
			status->curr_status = TRUN;
			MT2502_LOG("Mt2502 power on OK!!!\r\n");
		}
		return TSLEEP;
	}

TASK_GEN_STEP_ENTRY(88) {
		task_utils(task, TCFG_SET_STEP, TASK_STEP(0));
		unlock(&status->lock);
		return TSTOP;
	}

	return TSLEEP;
}
TASK_INIT(CMD_MT2502_POWERON, mt2502_poweron_thread, kd_false, NULL, 
		EXEC_FALSE,
		SCHED_PRIORITY_MIDE);

static unsigned char mt2502_poweroff_thread(struct task_struct *task, void *argc)
{
	int ret;
	struct msg_msg *msg;
	struct mt2502_status *status = &mt2502_kd_status;

TASK_GEN_STEP_ENTRY(0) {
		if (!status->curr_use_count && status->curr_status == TRUN) {
			ret = task_wait_timeout(task, Second(5));
			if (!ret)
				return TSLEEP;
			task_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP, 0);
		}
		else {
			task_utils(task, TCFG_CLEAR_TIMEOUT, 0);
			return TSLEEP;
		}
	}

TASK_GEN_STEP_ENTRY(1) {
		mt2502_task_msg_clear(task);
		mt2502_task_msg_register(task);

		lock(&status->lock);
		status->curr_status = TSTOP;

		mt2502_atcmd("AT+PWROFF");
		task_utils(task, TCFG_SET_NEXT_STEP, 0);
	}
	
TASK_GEN_STEP_ENTRY(2) {
		msg = mt2502_task_msg_check(task);
		ret = task_wait_timeout(task, Second(8));
		if (msg) {
			if (!kstrncmp("PWF:OK", &msg->msg_comment[3], 6))
				task_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP | TCFG_CLEAR_MSG, 0);
			else {
				// err msg
			}
		}
		else if (ret) {}
		else
			return TSLEEP;
	}
	
TASK_GEN_STEP_ENTRY(3) {
		ret = task_wait_timeout(task, Second(8));
		if (!ret)
			return TAGAIN;
		MT2502_LOAD_SWITCH_OFF();
		task_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP, 0);
	}
	
TASK_GEN_STEP_ENTRY(4) {		/* this t_step just handle 2502 power off abnormal */
		ret = task_wait_timeout(task, Second(1));
		if (!ret)
			return TAGAIN;
		MT2502_LOAD_SWITCH_ON();
		task_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_NEXT_STEP, 0);
	}
	
TASK_GEN_STEP_ENTRY(5) {
		ret = task_wait_timeout(task, Second(2));
		if (!ret)
			return TAGAIN;
		task_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_STEP, TASK_STEP(88));
	}

TASK_GEN_STEP_ENTRY(88) {
		task_utils(task, TCFG_SET_STEP, TASK_STEP(0));
		unlock(&status->lock);
	}

	return TSLEEP;
}
TASK_INIT(CMD_MT2502_POWEROFF, mt2502_poweroff_thread, kd_false, NULL, 
		EXEC_FALSE,
		SCHED_PRIORITY_MIDE);

static unsigned char mt2502_loadsw_thread(struct task_struct *task, void *argc)
{
	int ret, cto;
	struct mt2502_status *status = &mt2502_kd_status;
	struct msg_msg *msg;
	__u32 temp_cnfg = 0;
	__u8 temp_step = 0;

TASK_GEN_STEP_ENTRY(0) {
		if (status->is_usefirst || 
				(status->loadsw_use_cnt && !check_lock(&status->lock))) {
			status->is_usefirst = kd_false;
			MT2502_LOAD_SWITCH_ON();
			task_utils(task, TCFG_SET_STEP, TASK_STEP(1));
		}
		else if (0) { //(msg = mt2502_task_msg_check(task)) {
			// Uart need open load switch(ULS)
			if (!kstrncmp("ULS:ON", &msg->msg_comment[3], 6)) {
				MT2502_LOAD_SWITCH_ON();
				mdelay(10);
				temp_cnfg = TCFG_SET_NEXT_STEP;
				temp_step = TASK_STEP(1);
			}
			else {
				temp_cnfg = 0;
				temp_step = 0;
			}
			task_utils(task, TCFG_CLEAR_MSG | temp_cnfg, temp_step);
		}
		else
			return TSLEEP;
	}

TASK_GEN_STEP_ENTRY(1) {
		cto = task_wait_timeout(task, Second(50));
		if (!cto)
			return TSLEEP;
		if (check_lock(&status->lock))
			return TSLEEP;
		MT2502_LOAD_SWITCH_OFF();
		task_utils(task, TCFG_CLEAR_TIMEOUT | TCFG_SET_STEP, TASK_STEP(0));
	}

	return TSLEEP;
}
TASK_INIT(CMD_MT2502_LSW, mt2502_loadsw_thread, kd_false, NULL, 
		EXEC_FALSE,
		SCHED_PRIORITY_MIDE);

static int mt2502_gpio_init(void)
{
	struct mt2502_status *status = &mt2502_kd_status;

	// Mt2502 load switch enable
	gpio_set_dir(MT2502_LOAD_SWITCH_EN, GPIO_DIR_OUT, GPIO_CFG_OUT_HI_DISABLE | GPIO_CFG_NOPULL);
	gpio_set_out(MT2502_LOAD_SWITCH_EN, GPIO_OUT_ONE);

	// Mt2502 Power Key
	gpio_set_dir(MT2502_POWER_KEY, GPIO_DIR_OUT, GPIO_CFG_NOPULL);
	gpio_set_out(MT2502_POWER_KEY, GPIO_OUT_ZERO);

	status->is_usefirst = kd_true;

	return 0;
}

static int mt2502_probe(struct platform_device *dev)
{
	int ret;

	MT2502_LOG("mt2502 drv init\r\n");

	mt2502_gpio_init();
	ret = sched_task_create(TASK_INFO(mt2502_poweron_thread),
					SCHED_COMMON_TYPE,
					EXEC_TRUE);
	ret = sched_task_create(TASK_INFO(mt2502_poweroff_thread),
					SCHED_COMMON_TYPE,
					EXEC_TRUE);

	ret = sched_task_create(TASK_INFO(mt2502_loadsw_thread),
					SCHED_COMMON_TYPE,
					EXEC_TRUE);
	mt2502_task_msg_register(TASK_INFO(mt2502_loadsw_thread));

	mt2502_msg_init();

	//mt2502_power_set(POWER_ON);

	return 0;
}

static int mt2502_suspend(struct platform_device *dev, pm_message_t state)
{
	if (!mt2502_kd_status.curr_use_count)
		gpio_set_out(MT2502_LOAD_SWITCH_EN, GPIO_OUT_ZERO);

	return 0;
}

static int mt2502_resume(struct platform_device *dev)
{
	return 0;
}

static struct platform_driver mt2502_pldrv = {
	.probe = mt2502_probe,
	.suspend = mt2502_suspend,
	.resume = mt2502_resume,
	.driver = {
		.name  = "mt2502_pl",
	}
};

static struct platform_device mt2502_pldev = {
	.name = "mt2502_pl",
};


int __init mt2502_init(void)
{
	int ret;
	
	ret = platform_device_register(&mt2502_pldev);
	if (ret)
		goto init_err;
	ret = platform_driver_register(&mt2502_pldrv);
	if (ret)
		goto init_err;
	
init_err:
	return 0;
}

MODULE_DESCRIPTION("This is Charger mtk2502 driver on nrf platform");
MODULE_AUTHOR("xiaowen.huangg@gmail.com");
MODULE_LICENSE("GPL");
