#include <include/init.h>
#include <include/types.h>
#include <include/stdio.h>
#include <include/errno.h>
#include <include/sched.h>
#include <include/task.h>
#include <include/platform.h>
#include <include/gpio.h>
#include <include/fs.h>
#include <include/spi.h>
#include <include/lock.h>

#include "nrf_drv_spi.h"
#include "app_util_platform.h"
#include "nrf_drv_config.h"
#include "nrf_drv_common.h"
#include "nrf_gpio.h"

#define SPIM0_SCK_PIN   	27  // SPI clock GPIO pin number.
#define SPIM0_MOSI_PIN  	25  // SPI Master Out Slave In GPIO pin number.
#define SPIM0_MISO_PIN  	26  // SPI Master In Slave Out GPIO pin number.
#define SPIM0_SS_PIN    	24   // SPI Slave Select GPIO pin number.

static int nrfs_spi1_master_transfer(struct spi_slave *slave);

static const nrf_drv_spi_t nrfs_drv_spi1_master = NRF_DRV_SPI_INSTANCE(2);
static const struct spi_config nrfs_spi1_config = {
	.bus_num	= 0x01,
	.sck_pin  	= SPIM0_SCK_PIN,
	.mosi_pin 	= SPIM0_MOSI_PIN,
	.miso_pin 	= NRF_DRV_SPI_PIN_NOT_USED,
	.ss_pin   	= NRF_DRV_SPI_PIN_NOT_USED,
	.irq_priority = APP_IRQ_PRIORITY_LOW,
	.orc		  = 0xFF,
	.frequency	  = NRF_DRV_SPI_FREQ_8M,
	.mode		  = NRF_DRV_SPI_MODE_0,
	.bit_order	  = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST,
};
static struct spi_master nrfs_spi1_master = {
	.name = "nrfs_spi1_master",
	.bus_num = 0x01,
	.spi_transfer = nrfs_spi1_master_transfer,
	.platform_data = &nrfs_drv_spi1_master,
};

static void nrfs_spi1_master_event_handler(nrf_drv_spi_evt_t const *p_event)
{
#if 0
    spi_xfer_done = true;
    NRF_LOG_PRINTF(" Transfer completed.\r\n");
    if (m_rx_buf[0] != 0)
    {
        NRF_LOG_PRINTF(" Received: %s\r\n",m_rx_buf);
    }
#endif
}

static int nrfs_spi1_master_transfer(struct spi_slave *slave)
{
	int ret;
	__u32 n_tx = 0, n_rx = 0, len;
	const __u8 *tx_buf = NULL;
	__u8 *rx_buf = NULL;
	struct list_head *msg_qu = &slave->msg_qu, *iter;
	struct spi_trans_msg *trans;
	__u32 timeout = 15000;

	if (!slave)
		return -EIO;

	list_for_each(iter, msg_qu) {
		trans = container_of(iter, struct spi_trans_msg, msg_node);
		if (trans->tx_buf && !tx_buf) {
			tx_buf = trans->tx_buf;
			n_tx = trans->len;
		}
		if (trans->rx_buf && !rx_buf) {
			rx_buf = trans->rx_buf;
			n_rx = trans->len;
		}
	}

	while (check_lock(&slave->master->lock)) {
		timeout--;
		if (!timeout)
			return -ETIMEDOUT;
	}
	lock(&slave->master->lock);

	if (slave->slave_info->mode == SPI_MODE_4WIRE_1DAT_1DC && \
			slave->slave_info->status == SPI_READ_BYTE) {
		// Change the SDA pin to MISO pin.
		nrf_spim_pins_set(nrfs_drv_spi1_master.p_registers,
								  nrfs_spi1_config.sck_pin,
								  NRF_DRV_SPI_PIN_NOT_USED,
								  nrfs_spi1_config.mosi_pin);
		nrf_gpio_cfg_input(nrfs_spi1_config.mosi_pin, NRF_GPIO_PIN_NOPULL);
	}
	ret = nrf_drv_spi_transfer((nrf_drv_spi_t *)slave->master->platform_data,
					tx_buf, n_tx,
					rx_buf, n_rx);
	if (ret)
		unlock(&slave->master->lock);
	if (slave->slave_info->mode == SPI_MODE_4WIRE_1DAT_1DC && \
			slave->slave_info->status == SPI_READ_BYTE) {
		// Change back SDA pin to MOSI pin.
		nrf_spim_pins_set(nrfs_drv_spi1_master.p_registers,
								  nrfs_spi1_config.sck_pin,
								  nrfs_spi1_config.mosi_pin,
								  NRF_DRV_SPI_PIN_NOT_USED);
		nrf_gpio_pin_clear(nrfs_spi1_config.mosi_pin);
		nrf_gpio_cfg_output(nrfs_spi1_config.mosi_pin);
	}

	return ret;
}

int nrfs_spi1_master_init(void)
{
	int ret;
	const nrf_drv_spi_config_t nrfs_drv_spi1_config = {
			.sck_pin	= nrfs_spi1_config.sck_pin,
			.mosi_pin	= nrfs_spi1_config.mosi_pin,
			.miso_pin	= nrfs_spi1_config.miso_pin,
			.ss_pin		= nrfs_spi1_config.ss_pin,
			.irq_priority = nrfs_spi1_config.irq_priority,
			.orc		= nrfs_spi1_config.orc,
			.frequency	= nrfs_spi1_config.frequency,
			.mode		= nrfs_spi1_config.mode,
			.bit_order	= nrfs_spi1_config.bit_order,
	};

	ret = nrf_drv_spi_init(&nrfs_drv_spi1_master, &nrfs_drv_spi1_config, nrfs_spi1_master_event_handler);
	if (ret)
		spi_err("nrfs init error(%d)\r\n", ret);

	ret = spi_master_register(&nrfs_spi1_master);
	if (ret)
		spi_err("register error\r\n");
	ret = nrf_drv_spi_attach_master(&nrfs_drv_spi1_master, &nrfs_spi1_master);

	return ret;
}

struct spi_bus_info nrfs_spi1_bus_info =
{
	.name = "nrfs_spi1_bus",
	.confg = &nrfs_spi1_config,
	.prode = nrfs_spi1_master_init,
};

int __init nrfs_spi_init(void)
{
	return spi_register_businfo(&nrfs_spi1_bus_info);
}

module_init(nrfs_spi_init);

MODULE_DESCRIPTION("This is spi driver on nrf platform");
MODULE_AUTHOR("xiaowen.huangg@gmail.com");
MODULE_LICENSE("GPL");
