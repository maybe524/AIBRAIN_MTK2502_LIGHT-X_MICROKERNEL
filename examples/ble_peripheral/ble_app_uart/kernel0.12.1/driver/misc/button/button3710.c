#include <include/init.h>
#include <include/types.h>
#include <include/stdio.h>
#include <include/errno.h>
#include <include/sched.h>
#include <include/task.h>
#include <include/platform.h>
#include <include/irq.h>
#include <include/gpio.h>
#include <include/fs.h>
#include <include/msg.h>
#include <include/button.h>
#include <include/assert.h>

#include "button3710.h"

#define BLOG(fmt, argc...)	printk("Button3710: " fmt, ##argc)

APP_TIMER_DEF(button3710_timer);

extern kd_bool system_inited;
static __u8 button3710_push_cnt = 0, button3710_release_cnt = 0;
static __u32 button3710_time = 0;

static unsigned char __sched
		___button3710_push_isr(struct task_struct *task, void *data)
{
	printk("___button3710_push_isr\r\n");

	button3710_push_cnt++;
	return TSTOP;
}
TASK_INIT(CMD_BUTTON_PUSH, ___button3710_push_isr, kd_true, NULL, 
		EXEC_TRUE,
		SCHED_PRIORITY_MIDE);

static unsigned char __sched
		___button3710_release_isr(struct task_struct *task, void *data)
{
	printk("___button3710_release_isr\r\n");

	button3710_release_cnt++;
	return TSTOP;
}
TASK_INIT(CMD_BUTTON_RELEASE, ___button3710_release_isr, kd_true, NULL, 
		EXEC_TRUE,
		SCHED_PRIORITY_MIDE);

static unsigned char __sched
		___button3710_handle(struct task_struct *task, void *data)
{
	int ret;
	__u8 msg_buff[20] = {0};
	__u8 result_cnt;

TASK_GEN_STEP_ENTRY(0) {
		if (button3710_push_cnt || button3710_release_cnt) {
			ret = app_timer_start(button3710_timer, APP_TIMER_TICKS(1, 0), NULL);
			task_utils(task, TCFG_SET_NEXT_STEP | TCFG_CLEAR_TIMEOUT, 0);
			button3710_time = 0;
		}
		else
			return TSLEEP;
	}

TASK_GEN_STEP_ENTRY(1) {
		if (button3710_time > 550) {
			app_timer_stop(button3710_timer);
			result_cnt = button3710_push_cnt >= button3710_release_cnt ? button3710_push_cnt : button3710_release_cnt;
			if (result_cnt == 1)
				ksprintf(msg_buff, "UI%cBTN:SINGLE", 0x02);
			else if (result_cnt == 2)
				ksprintf(msg_buff, "UI%cBTN:DOUBLE", 0x02);

			BLOG("result_cnt = %d, push_cnt = %d, release_cnt = %d, msg: %s\r\n",
					result_cnt, button3710_push_cnt, button3710_release_cnt,
					msg_buff[0] ? msg_buff : "N/A");

			msg_create(CMD_BUTTON, CMD_UI, MSG_TO_RECEIVER_ONLINE, msg_buff, kstrlen(msg_buff));
			task_utils(task, TCFG_SET_STEP, 0);

			button3710_push_cnt = 0;
			button3710_release_cnt = 0;
			button3710_time = 0;
		}
		else
			return TSLEEP;
	}

	return TSLEEP;
}
TASK_INIT(CMD_BUTTON, ___button3710_handle, kd_true, NULL, 
		EXEC_TRUE,
		SCHED_PRIORITY_MIDE);

static void button3710_isr(__u32 pin, __u32 action)
{
	irq_mask(BUTTON_IRQ_READY_PIN);
	sched_task_create(TASK_INFO(___button3710_push_isr), SCHED_NRFS_TYPE, EXEC_TRUE);
}

static void button3710_time_recode(void *p_context)
{
	button3710_time++;
}

static void button3710_event_handler(__u8 pin_no, __u8 button_action)
{
	int ret;

	switch (button_action) {
		case BUTTON_PUSH:
			switch (pin_no) {
				case BUTTON_IRQ_READY_PIN:
					ret = sched_task_create(TASK_INFO(___button3710_push_isr), SCHED_NRFS_TYPE, EXEC_TRUE);
					assert(!ret);
					break;
				default:
					break;
			}
			break;
		case BUTTON_RELEASE: 
			switch (pin_no) {
				case BUTTON_IRQ_READY_PIN:
					ret = sched_task_create(TASK_INFO(___button3710_release_isr), SCHED_NRFS_TYPE, EXEC_TRUE);
					assert(!ret);
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}	
}

static int button3710_probe(struct platform_device *dev)
{
	int ret;
	struct button_info button3710[] = {
		{BUTTON_IRQ_READY_PIN, kd_false, BUTTON_GPIO_PIN_NOPULL, button3710_event_handler},
	};

	printk("button3710 probe!!!\r\n");

#if 0
	irq_register_isr(BUTTON_IRQ_TYPE,
				BUTTON_IRQ_READY_PIN,
				IRQ_CFG_TRIGGLE_FALLING | IRQ_CFG_PULLUP | IRQ_CFG_HI_ACCURACY,
				button3710_isr);
	irq_unmask(BUTTON_IRQ_READY_PIN);
#else
	ret = button_register(button3710, ARRAY_ELEM_NUM(button3710), BUTTON_DETECTION_DELAY);
	if (ret)
		printk("button_register fail\r\n");
#endif
	sched_task_create(TASK_INFO(___button3710_handle), SCHED_COMMON_TYPE, EXEC_TRUE);
	ret = app_timer_create(&button3710_timer, APP_TIMER_MODE_REPEATED, button3710_time_recode);

	return 0;
}

static int button3710_suspend(struct platform_device *dev, pm_message_t state)
{
	return 0;
}

static int button3710_resume(struct platform_device *dev)
{
	return 0;
}

static struct platform_driver button3710_pldrv = {
	.probe = button3710_probe,
	.suspend = button3710_suspend,
	.resume = button3710_resume,
	.driver = {
		.name  = "button3710_pl",
	}
};

static struct platform_device button3710_pldev = {
	.name = "button3710_pl",
};

int __init button3710_init(void)
{
	int ret;

	ret = platform_device_register(&button3710_pldev);
	if (ret)
		goto init_err;
	ret = platform_driver_register(&button3710_pldrv);
	if (ret)
		goto init_err;

	return 0;

init_err:
	return 0;
}

module_init(button3710_init);

MODULE_DESCRIPTION("This is button3710 driver on nrf platform");
MODULE_AUTHOR("xiaowen.huangg@gmail.com");
MODULE_LICENSE("GPL");
