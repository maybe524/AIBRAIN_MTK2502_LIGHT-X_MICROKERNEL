#include <include/gpio.h>
#include <include/init.h>

#include "nrf_gpio.h"

static int nrfs_gpio_set_dir(__u32 pin, __u8 dir, __u8 config)
{
	if (dir == GPIO_DIR_OUT) {
		nrf_gpio_cfg_output(pin);
	}
	else {
		nrf_gpio_cfg_input(pin, config);
	}

	return 0;
}

static int nrfs_gpio_set_out(__u32 pin, __u8 out)
{
	if (out == GPIO_OUT_ONE) {
		nrf_gpio_pin_set(pin);
	}
	else {
		nrf_gpio_pin_clear(pin);
	}

	return 0;
}

static int nrfs_gpio_get_val(__u32 pin)
{
	return nrf_gpio_pin_read(pin);
}

static const struct gpio_operations nrfs_gpio_operations = {
	.set_dir = nrfs_gpio_set_dir,
	.set_out = nrfs_gpio_set_out,
	.get_val = nrfs_gpio_get_val,
};

static struct gpio_board_info nrfs_gpio_board_info = {
	.name = "nrfs_gpio",
	.rang_begin = 0x00,
	.rang_end	= 0xFF - 1,
	.g_op	= &nrfs_gpio_operations,
};

int __init nrfs_gpio_init(void)
{
	int ret;

	ret = gpio_register_board_info(&nrfs_gpio_board_info, 1);

	return 0;
}

module_init(nrfs_gpio_init);