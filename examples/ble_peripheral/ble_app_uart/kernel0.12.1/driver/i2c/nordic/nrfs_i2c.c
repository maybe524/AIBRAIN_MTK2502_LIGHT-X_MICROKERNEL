#include <nrf_twi.h>
#include <nrf_drv_twi.h>
#include <app_util_platform.h>

#include <include/init.h>
#include <include/types.h>
#include <include/errno.h>
#include <i2c.h>
#include <include/gpio.h>

#include "nrf_drv_common.h"

#define EEPROM_SIM_SIZE				128 // Simulated EEPROM size
#define EEPROM_SIM_SEQ_WRITE_MAX	8   // Maximum number of bytes writable in one sequential access
#define EEPROM_SIM_ADDR				0x58 // Simulated EEPROM TWI address
#define TWI_SCL_M					3   // Master SCL pin
#define TWI_SDA_M					4   // Master SDA pin
#define MASTER_TWI_INST				1   // TWI interface used as a master accessing EEPROM memory

#define NRFS_I2C_SCL_HIGH()			gpio_set_out(nrfs_i2c_master_config.scl, GPIO_OUT_ONE);
#define NRFS_I2C_SCL_LOW()			gpio_set_out(nrfs_i2c_master_config.scl, GPIO_OUT_ZERO);
#define NRFS_I2C_SDA_HIGH()			gpio_set_out(nrfs_i2c_master_config.sda, GPIO_OUT_ONE);
#define NRFS_I2C_SDA_LOW()			gpio_set_out(nrfs_i2c_master_config.sda, GPIO_OUT_ZERO);

#define NRFS_I2C_LOG(fmt, argc...)		printk(fmt, ##argc)

static const nrf_drv_twi_t m_twi_master = NRF_DRV_TWI_INSTANCE(MASTER_TWI_INST);
const static struct i2c_config nrfs_i2c_master_config =
{
#ifdef CONFIG_PLATFORM_NRFS
	.bus_num = 0x01,
    .scl	= 3,
    .sda	= 2,
    .freq	= NRF_TWI_FREQ_400K,
#else
	.bus_num = 0x01,
    .scl	= TWI_SCL_M,
    .sda	= TWI_SDA_M,
    .freq	= NRF_TWI_FREQ_400K,
#endif
};

static int ___nrfs_i2c_master_write_indirectly(__u8 dev_addr,
			__u8 addr,
			uint8_t const *pdata,
			__size_t size)
{
    int ret;

    do {
        __u8 addr8 = (__u8)addr;

		ret = nrf_drv_twi_tx(&m_twi_master, dev_addr, &addr8, 1, true);
        if(NRF_SUCCESS != ret)
            break;
        ret = nrf_drv_twi_tx(&m_twi_master, dev_addr, pdata, size, false);
    } while(0);

    return ret;
}

static int ___nrfs_i2c_master_write_directly(__u8 dev_addr,
				uint8_t const *pdata,
				__size_t size)
{
	return nrf_drv_twi_tx(&m_twi_master, dev_addr, pdata, size, false);
}

static int nrfs_i2c_master_write(struct i2c_client *client,
		__u8 addr, __u32 value)
{
	int ret;
	__u8 i, data_byte, temp[5] = {0};

	if (!client)
		return -EINVAL;
	data_byte = client->dev_info.board_info.flags & 0x07;

	if (client->dev_info.board_info.flags & I2C_FLAGS_WRITE_DIRECTLY) {
		temp[0] = addr;
		for (i = 0; i < data_byte; i++)
			temp[i + 1] = (value >> ((data_byte - i - 1) * 8)) & 0xFF;
		ret = ___nrfs_i2c_master_write_directly(client->dev_info.board_info.addr, temp, data_byte + 1);
	}
	else {
		for (i = 0; i < data_byte; i++)
			temp[i] = (value >> ((data_byte - i - 1) * 8)) & 0xFF;
		ret = ___nrfs_i2c_master_write_indirectly(client->dev_info.board_info.addr, addr, temp, data_byte);
	}

	if (ret)
		NRFS_I2C_LOG("ERROR: nrfs i2c write error(%02d), Client ADDR: 0x%01x, name: %9s\r\n",
			ret, client->dev_info.board_info.addr, 
			client->dev_info.board_info.type);

	return ret;
}

static int ___twi_master_read(__u8 dev_addr, __u8 addr, uint8_t *pdata, __size_t size)
{
    int ret;

	do {
       __u8 addr8 = (__u8)addr;

		ret = nrf_drv_twi_tx(&m_twi_master, dev_addr, &addr8, 1, true);
		if (NRF_SUCCESS != ret)
			break;
		ret = nrf_drv_twi_rx(&m_twi_master, dev_addr, pdata, size);
    } while(0);

	return ret;
}

static int
nrfs_i2c_master_read(struct i2c_client *client,
		__u8 addr, __u32 *value)
{
	int ret;
	__u8 i, data_byte, temp[4] = {0};

	if (!client)
		return -EINVAL;
	data_byte = client->dev_info.board_info.flags & 0x07;
	*value = 0;

	ret = ___twi_master_read(client->dev_info.board_info.addr, addr, temp, data_byte);
	for (i = 0; i < data_byte; i++)
		*value |= temp[i] << ((data_byte - i - 1) * 8);

	if (ret)
		NRFS_I2C_LOG("ERROR: nrfs i2c read  error(%02d), Client ADDR: 0x%01x, name: %9s\r\n",
			ret, client->dev_info.board_info.addr,
			client->dev_info.board_info.type);

	return ret;
}

static int __init nrf_i2c_master_init(void)
{
    int ret;
	nrf_drv_twi_config_t config;

	config.scl = nrfs_i2c_master_config.scl;
	config.sda = nrfs_i2c_master_config.sda;
	config.frequency = nrfs_i2c_master_config.freq;
	config.interrupt_priority = APP_IRQ_PRIORITY_HIGH;

	ret = nrf_drv_twi_init(&m_twi_master, &config, NULL, NULL);
	if (ret) {
		NRFS_I2C_LOG("nrfs i2c init FAIL!\r\n");
		return ret;
	}
	nrf_drv_twi_enable(&m_twi_master);

	return 0;
}

static int nrfs_i2c_bus_reset(void)
{
	int ret;

	nrf_drv_twi_uninit(&m_twi_master);

	gpio_set_dir(nrfs_i2c_master_config.scl, GPIO_DIR_OUT, 0);
	gpio_set_dir(nrfs_i2c_master_config.sda, GPIO_DIR_OUT, 0);

	NRFS_I2C_SCL_HIGH();
	NRFS_I2C_SDA_HIGH();

	NRFS_I2C_SDA_LOW();
	mdelay(50);
	NRFS_I2C_SCL_LOW();

	mdelay(800);

	NRFS_I2C_SDA_HIGH();
	mdelay(50);
	NRFS_I2C_SCL_HIGH();

	ret = nrf_i2c_master_init();

	return ret;
}

static int
nrfs_i2c_master_transfer(struct i2c_client *client,
		unsigned int cmd,
		__u8 addr,
		__u32 *value)
{
	int ret;

	switch (cmd) {
		case I2C_CMD_WRITE:
			ret = nrfs_i2c_master_write(client, addr, *value);
			break;
		case I2C_CMD_READ:
			ret = nrfs_i2c_master_read(client, addr,   value);
			break;
		case I2C_BUS_RESET:
			ret = nrfs_i2c_bus_reset();
			break;
		default:
			break;
	}

	return ret;
}

struct i2c_bus_info nrfs_i2c_master_info =
{
	.confg = &nrfs_i2c_master_config,
	.prode = nrf_i2c_master_init,
	.i2c_transfer = nrfs_i2c_master_transfer,
};

int nrfs_i2c_master_init(void)
{
    return i2c_register_businfo(&nrfs_i2c_master_info);
}

module_init(nrfs_i2c_master_init);

MODULE_DESCRIPTION("This is i2c master init on nrf platform");
MODULE_AUTHOR("xiaowen.huangg@gmail.com");
MODULE_LICENSE("37@longcheer.net");
