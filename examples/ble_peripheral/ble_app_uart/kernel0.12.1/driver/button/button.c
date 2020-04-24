#include <include/init.h>
#include <include/types.h>
#include <include/list.h>
#include <include/errno.h>
#include <include/button.h>

static struct button_driver *___button_driver = NULL;

int button_register(struct button_info *button_info, __u8 count, __u32 deboundce)
{
	if (!___button_driver)
		return -EINVAL;
	return ___button_driver->button_register(button_info, count, deboundce);
}

int button_driver_register(struct button_driver *button_drv)
{
	if (!button_drv || ___button_driver)
		return -EINVAL;

	___button_driver = button_drv;
	return 0;
}