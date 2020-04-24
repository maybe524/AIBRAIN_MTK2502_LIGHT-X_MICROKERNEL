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

#ifdef CONFIG_PLATFORM_NRFS
#include "nrf_drv_saadc.h"
#endif

int pnm723t_read_adc(void)
{
#ifdef CONFIG_PLATFORM_NRFS
	nrf_saadc_value_t val = 0;
#endif

	gpio_set_dir(EXGPIO_3, GPIO_DIR_OUT, GPIO_CFG_OUT_HI_DISABLE);
	gpio_set_out(EXGPIO_3, GPIO_OUT_ONE);

#ifdef CONFIG_PLATFORM_NRFS
	nrf_drv_saadc_sample_convert(7, &val);
#endif
	printk("temp = 0x%04x\r\n", val);

	return 0;
}
