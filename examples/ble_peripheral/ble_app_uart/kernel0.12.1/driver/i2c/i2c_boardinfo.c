#include <include/init.h>
#include <i2c.h>
#include <include/malloc.h>
#include <include/errno.h>

extern struct list_head __i2c_board_list;

/**
 * i2c_register_board_info - statically declare I2C devices
 * @busnum: identifies the bus to which these devices belong
 * @info: vector of i2c device descriptors
 * @len: how many descriptors in the vector; may be zero to reserve
 *	the specified bus number.
 *
 * Systems using the Linux I2C driver stack can declare tables of board info
 * while they initialize.  This should be done in board-specific init code
 * near arch_initcall() time, or equivalent, before any I2C adapter driver is
 * registered.  For example, mainboard init code could define several devices,
 * as could the init code for each daughtercard in a board stack.
 *
 * The I2C devices will be created later, after the adapter for the relevant
 * bus has been registered.  After that moment, standard driver model tools
 * are used to bind "new style" I2C drivers to the devices.  The bus number
 * for any device declared using this routine is not available for dynamic
 * allocation.
 *
 * The board info passed can safely be __initdata, but be careful of embedded
 * pointers (for platform_data, functions, etc) since that won't be copied.
 */
struct i2c_client *
i2c_register_board_info(int busnum,
			struct i2c_board_info const *info, unsigned len)
{
	struct i2c_client *client;

	client = kzalloc(sizeof(*client), GFP_NORMAL);
	if (!client)
		goto mem_err;

	client->dev_info.busnum = busnum;
	client->dev_info.board_info = *info;
	client->bus_info = i2c_match_businfo(client);
	if (!client->bus_info)
		goto bus_err;

	list_add_tail(&client->dev_info.list, &__i2c_board_list);
	return client;

bus_err:
	kfree(client);
mem_err:
	pr_debug("i2c-core: can't register boardinfo!\r\n");

	return NULL;
}
