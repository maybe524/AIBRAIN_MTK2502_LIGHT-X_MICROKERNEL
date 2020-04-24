#include <init.h>
#include <list.h>
#include <spi.h>
#include <malloc.h>
#include <include/string.h>
#include <include/errno.h>

#define MAX_SPI 2

static struct spi_master *g_spi_masters[MAX_SPI];
LIST_HEAD(__spi_bus_list);

int spi_transfer(struct spi_slave *slave)
{
	struct spi_master *master = slave->master;

	master->active_slave = slave;
	if (master->spi_transfer)
		return master->spi_transfer(slave);

	return -ENOTSUPP;
}

int spi_write_then_read(struct spi_slave *slave,
			const __u8 *txbuf, __u32 n_tx, __u8 *rxbuf, __u32 n_rx)
{
	enum TX_TYPE {TX, RX};
	struct spi_trans_msg tx[2];

	tx[TX].tx_buf = txbuf;
	tx[TX].rx_buf = NULL;
	tx[TX].len = n_tx;

	tx[RX].tx_buf = NULL;
	tx[RX].rx_buf = rxbuf;
	tx[RX].len = n_rx;

	INIT_LIST_HEAD(&slave->msg_qu);
	list_add_tail(&tx[TX].msg_node, &slave->msg_qu);
	list_add_tail(&tx[RX].msg_node, &slave->msg_qu);

	return spi_transfer(slave);
}

struct spi_master *spi_master_alloc(void)
{
	struct spi_master *master;

	master = (struct spi_master *)kmalloc(sizeof(*master), GFP_NORMAL);
	if (NULL == master)
		return NULL;

	master->bus_num = -1;
	INIT_LIST_HEAD(&master->slave_list);
	master->spi_transfer = NULL;

	return master;
}

struct spi_slave *spi_slave_alloc(void)
{
	struct spi_slave *slave;

	slave = (struct spi_slave *)kmalloc(sizeof(*slave), GFP_NORMAL);
	if (NULL == slave)
		return NULL;

	slave->master = NULL;
	INIT_LIST_HEAD(&slave->msg_qu);

	return slave;
}

int spi_slave_attach(struct spi_master *master, struct spi_slave *slave)
{
	list_add_tail(&slave->slave_node, &master->slave_list);
	slave->master = master;

	return 0;
}

int spi_master_register(struct spi_master *master)
{
	int i;

	for (i = 0; i < MAX_SPI; i++) {
		if (!g_spi_masters[i]) {
			g_spi_masters[i] = master;
			return 0;
		}
	}

	return -EBUSY;
}

static struct spi_master *g_spi_master;
static int g_spi_master_num;

static struct spi_slave  *g_spi_slave;
static int  g_spi_slave_num;

int set_master_tab(struct spi_master spi_master[], int len)
{
	g_spi_master_num = len;
	g_spi_master = spi_master;

	return 0;
}

int set_slave_tab(struct spi_slave spi_slave[], int len)
{
	g_spi_slave_num = len;
	g_spi_slave = spi_slave;

	return 0;
}

struct spi_master *get_spi_master(char *name)
{
	int i;

	for (i = 0; i < MAX_SPI; i++) {
		if (!kstrcmp(g_spi_masters[i]->name, name))
			return g_spi_masters[i];
	}

	return NULL;
}

struct spi_slave *get_spi_slave(char *name)
{
	int i;

	for (i = 0; i < g_spi_slave_num; i++) {
		if (!kstrcmp(g_spi_slave[i].name, name))
			return g_spi_slave + i;
	}

	return NULL;
}

/**
 * spi_register_board_info - register SPI devices for a given board
 * @info: array of chip descriptors
 * @n: how many descriptors are provided
 * Context: can sleep
 *
 * Board-specific early init code calls this (probably during arch_initcall)
 * with segments of the SPI device table.  Any device nodes are created later,
 * after the relevant parent SPI controller (bus_num) is defined.  We keep
 * this table of devices forever, so that reloading a controller driver will
 * not make Linux forget about these hard-wired devices.
 *
 * Other code can also call this, e.g. a particular add-on board might provide
 * SPI devices through its expansion connector, so code initializing that board
 * would naturally declare its SPI devices.
 *
 * The board info passed can safely be __initdata ... but be careful of
 * any embedded pointers (platform_data, etc), they're copied as-is.
 */
struct spi_slave *spi_register_slave_info(struct spi_slave_info *info, unsigned n)
{
	struct spi_slave *slave = NULL;
	struct spi_master *master = NULL;
	int ret;

	slave = spi_slave_alloc();
	if (!slave)
		return NULL;

	master = get_spi_master(info->matchto_master_name);
	if (!master)
		goto register_err;

	ret = spi_slave_attach(master, slave);
	if (ret)
		goto register_err;

	slave->slave_info= info;

	return slave;

register_err:
	kfree(slave);
	return 0;
}

int spi_register_businfo(struct spi_bus_info *info)
{
	int ret;
	struct list_head *iter;

	if (!info)
		return -EINVAL;

	list_for_each(iter, &__spi_bus_list) {
		struct spi_bus_info *spi_bus = container_of(iter, struct spi_bus_info, list);
		if (spi_bus == info || \
				spi_bus->confg->bus_num == info->confg->bus_num || \
				!kstrcmp(spi_bus->name, info->name))
			return -EBUSY;
	}

	if (info->prode) {
		if (ret = info->prode())
			return ret;
		list_add(&info->list, &__spi_bus_list);
	}

	return 0;
}


static int __init spi_init(void)
{
	int i;

	kprintf("%s:spi subsys init\n", __func__);

	for (i = 0; i < MAX_SPI; i++)
		g_spi_masters[i] = NULL;

	return 0;
}

subsys_init(spi_init);
