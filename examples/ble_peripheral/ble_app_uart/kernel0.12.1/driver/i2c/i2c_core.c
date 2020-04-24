#include <include/init.h>
#include <include/types.h>
#include <include/list.h>
#include <i2c.h>
#include <include/errno.h>

LIST_HEAD(__i2c_bus_list);
LIST_HEAD(__i2c_board_list);

struct i2c_bus_info *
i2c_match_businfo(struct i2c_client *client)
{
	struct list_head *iter;

	list_for_each(iter, &__i2c_bus_list) {
		struct i2c_bus_info *i2c_bus = container_of(iter, struct i2c_bus_info, list);
		if (client->dev_info.busnum == \
					i2c_bus->confg->bus_num)
			return i2c_bus;
	}

	return NULL;
}

int i2c_register_businfo(struct i2c_bus_info *info)
{
	int ret;
	struct list_head *iter;

	if (!info)
		return -EINVAL;

	list_for_each(iter, &__i2c_bus_list) {
		struct i2c_bus_info *i2c_bus = container_of(iter, struct i2c_bus_info, list);
		if (i2c_bus == info || \
				i2c_bus->confg->bus_num == info->confg->bus_num)
			return -EBUSY;
	}

	if (info->prode) {
		if (ret = info->prode())
			return ret;
		list_add(&info->list, &__i2c_bus_list);
	}

	return 0;
}

int i2c_write_bytes(struct i2c_client *client, __u8 addr, __u32 value)
{	
	if (!client || \
			!client->bus_info->i2c_transfer)
		return -EINVAL;
	return client->bus_info->i2c_transfer(client,
				I2C_CMD_WRITE,
				addr, 
				&value);
}

int i2c_read_bytes(struct i2c_client *client, __u8 addr, __u32 *value)
{
	if (!client || \
			!client->bus_info->i2c_transfer)
		return -EINVAL;
	return client->bus_info->i2c_transfer(client,
				I2C_CMD_READ,
				addr, 
				value);
}

int i2c_busp_reset(struct i2c_client *client)
{
	if (!client || \
			!client->bus_info->i2c_transfer)
		return -EINVAL;
	return client->bus_info->i2c_transfer(client,
				I2C_BUS_RESET,
				0, 
				0);
}

/*
 * An i2c_driver is used with one or more i2c_client (device) nodes to access
 * i2c slave chips, on a bus instance associated with some i2c_adapter.
 */
int i2c_register_driver(struct i2c_driver *driver)
{
	int res;

	/* When registration returns, the driver core
	 * will have called probe() for all matching-but-unbound devices.
	 */
	res = driver_register(&driver->driver);
	if (res)
		return res;

	return 0;
}

EXPORT_SYMBOL(i2c_register_businfo);
EXPORT_SYMBOL(i2c_write_bytes);
EXPORT_SYMBOL(i2c_read_bytes);
EXPORT_SYMBOL(i2c_register_driver);
