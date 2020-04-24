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

#define MISC_TAG		"[MISC]"
#define MISC_LOG(fmt, arg...)	printk(MISC_TAG fmt, ##arg)

module_extern(bq_init);
module_extern(pps_init);

int __init misc_init(void)
{
	MISC_LOG("misc driver init\r\n");

	button3710_init();
	bq25120_init();
	tca7408_init();
	pps960_init();
	drv2605_init();
	mt2502_init();
	lcm_drv_init();

	return 0;
}

module_init(misc_init);

MODULE_DESCRIPTION("This is i2c misc driver on nrf platform");
MODULE_AUTHOR("xiaowen.huangg@gmail.com");
MODULE_LICENSE("GPL");
