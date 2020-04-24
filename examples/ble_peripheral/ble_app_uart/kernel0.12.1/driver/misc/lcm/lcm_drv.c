#include <include/stdio.h>
#include <include/errno.h>
#include <include/lcm/lcm_drv.h>
#include <include/graphic/display.h>

#define lcm_err(fmt, argc...)		printk("LCM_DRV ERROR: " fmt, ##argc)

int __weak lcm_drv_suspend(void)
{	
	return -ENOTSUPP;
}

int __weak lcm_drv_resume(void)
{
	return -ENOTSUPP;
}

int __weak lcm_drv_init(void)
{
	return -ENOTSUPP;
}

int __weak lcm_drv_update(__u8 *buff, __u32 x, __u32 y, __u32 width, __u32 height, __u32 flags)
{
	return -ENOTSUPP;
}

int __weak lcm_drv_get_info(struct lcd_info *info)
{
	return -ENOTSUPP;
}

struct lcd_drv plat_lcd_drv = {
	.name = "lcm_drv",
	.init = lcm_drv_init,
	.suspend = lcm_drv_suspend,
	.resume = lcm_drv_resume,
	.update = lcm_drv_update,
	.get_info = lcm_drv_get_info,
};
