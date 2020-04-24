#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "drv_kxo22.h"
#include "nordic_common.h"
#include "nrf_error.h"
#include "nrf52.h"
#include "nrf_delay.h"
#include "nrf_drv_spi.h"
#include "app_util_platform.h"
#include "app_timer.h"

/**
 * @brief SPI master instance configuration.
 */
#define NRF_DRV_SPI_GSENSOR_CONFIG(id)                       \
{                                                            \
    .sck_pin      = CONCAT_3(SPI, id, _CONFIG_SCK_PIN),      \
    .mosi_pin     = CONCAT_3(SPI, id, _CONFIG_MOSI_PIN),     \
    .miso_pin     = CONCAT_3(SPI, id, _CONFIG_MISO_PIN),     \
    .ss_pin       = 11,                \
    .irq_priority = CONCAT_3(SPI, id, _CONFIG_IRQ_PRIORITY), \
    .orc          = 0xFF,                                    \
    .frequency    = NRF_DRV_SPI_FREQ_4M,                     \
    .mode         = NRF_DRV_SPI_MODE_0,                      \
    .bit_order    = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST,         \
}
#define SPI_GSENSOR_INSTANCE  0
#define DONT_TRACE_UART

#define IO_KXO22_INT_1_PIN	(13)
#define IO_KXO22_INT_2_PIN	(12)

#define GSENSOR_INTERVAL			APP_TIMER_TICKS(1000, 0)
#define GSENSOR_START_INTERVAL		APP_TIMER_TICKS(500, 0)
#define GSENSOR_UPLOAD_INTERVAL		APP_TIMER_TICKS(125, 0)

APP_TIMER_DEF(m_gsensor_timer_id);
APP_TIMER_DEF(m_gsensor_upload_timer_id);

uint16_t gsensor_x = 0;
uint16_t gsensor_y = 0;
uint16_t gsensor_raw_array[75] = {0};

uint8_t gsensor_test_array[150] = {0};
uint8_t gsensor_raw_data[75] = {0};
uint8_t upload_data[126] = {0};

uint8_t kxo22_chip_id;
uint8_t kxo22_chip_cotr;
bool	m_gsensor_factory_test = false;

static const nrf_drv_spi_t spi = NRF_DRV_SPI_INSTANCE(SPI_GSENSOR_INSTANCE);  /**< SPI instance. */
static bool m_gsensor_start = false;
static bool m_upload_timer = false;
static uint8_t ble_nus_gsensor_rawdata[20] = {0};
static uint8_t m_upload_index = 0;
static uint8_t m_factory_test_id = 0;

ret_code_t result;
static uint8_t sdio_read_byte(uint8_t address)
{
	uint8_t tx_data[] = {0,0};
	uint8_t rx_data[] = {0,0};

	tx_data[0] = address | BIT_7;

	result = nrf_drv_spi_transfer(&spi, tx_data, 2, rx_data, 2);
	return rx_data[1];
}

static uint8_t sdio_write_byte(uint8_t address, uint8_t data_byte)
{
    uint8_t tx_data[] = {0, 0};
    uint8_t rx_data[] = {0, 0};

	tx_data[0] = (address & ~(BIT_7));
    tx_data[1] = data_byte;

	result = nrf_drv_spi_transfer(&spi, tx_data, 2, rx_data, 2);
    return rx_data[1];
}

static void arithmetic_spi_write(uint8_t regaddr, uint8_t *p_data, uint8_t data_len)
{
	uint8_t num = 0;
	
	while(num < data_len)
		sdio_write_byte(regaddr++, p_data[num++]);
}

static void arithmetic_spi_read(uint8_t regaddr, uint8_t *p_data, uint8_t data_len)
{
	uint8_t num = 0;
	
	while(num < data_len)
		p_data[num++] = sdio_read_byte(regaddr++);
}

uint8_t drv_kxo22_read_register(uint8_t reg_addr)
{
	return sdio_read_byte(reg_addr);
}

static void get_gsensor_rawdata(void)
{
	uint8_t sum = 0;
	uint8_t num = 0;
	uint8_t i = 0;
	uint8_t array[150] = {0};
	uint8_t gsensor_xy[2] = {0};
	
	arithmetic_spi_read(0X3C, &sum, 1);
	
	if (0x95 < sum) {
		sum = 0x96;
		
		for (num=0;num<sum;num++) {
			arithmetic_spi_read(0X3F, &(array[num]), 1);
			gsensor_test_array[num] = array[num];
		}
		for (num=0; num<75; num++) {
			gsensor_raw_array[num] = (array[num*2]&0x00FF)|((array[num*2+1]<<8)&0xFF00);  
			if (0X0 == ((array[num*2+1]>>7)&0X01)) {
				gsensor_raw_data[num] = ((0 == ((array[num*2+1]>>5)&0X03))? \
												((array[num*2+1]<<2)&0XFC)|((array[num*2]>>6)&0X03):\
												0XFF) & 0X7F;
			} else {
				gsensor_raw_data[num] = ((0X03 == ((array[num*2+1]>>5)&0X03))? \
												((array[num*2+1]<<2)&0XFC)|((array[num*2]>>6)&0X03):\
												0X80) | 0X80;
			}
		}				
	}
	
	for (i=0; i<25; i++) {					
		gsensor_x = gsensor_test_array[i*6+1];
		gsensor_y = gsensor_test_array[i*6+3];
		gsensor_x = ((gsensor_x << 8) | gsensor_test_array[i*6+0]);
		gsensor_y = ((gsensor_y << 8) | gsensor_test_array[i*6+2]);
		gsensor_x = ((gsensor_x & 0x8000) == 0)? gsensor_x : (~gsensor_x + 1);
		gsensor_y = ((gsensor_y & 0x8000) == 0)? gsensor_y : (~gsensor_y + 1);
		gsensor_xy[0] = ((gsensor_x + gsensor_y) & 0x00FF);
		gsensor_xy[1] = ((gsensor_x + gsensor_y) & 0xFF00) >> 8;
		
		upload_data[i*5+0] = gsensor_xy[0];
		upload_data[i*5+1] = gsensor_xy[1];
		upload_data[i*5+2] = gsensor_test_array[i*6+4];
		upload_data[i*5+3] = gsensor_test_array[i*6+5];
		upload_data[i*5+4] = 0;
	}
	upload_data[125] = 0xAA;
}

static void gsensor_timeout_handler(void)
{
	uint32_t err_code;
	
	if (!m_gsensor_start) {
		uint8_t value = 0x90;
		err_code = app_timer_stop(m_gsensor_timer_id);
		APP_ERROR_CHECK(err_code);
		nrf_delay_us(10000);
		err_code = app_timer_start(m_gsensor_timer_id, GSENSOR_INTERVAL, NULL);
		APP_ERROR_CHECK(err_code);
		m_gsensor_start = true;
		// enable gsensor				
		arithmetic_spi_write(ACC_REG_ADDR_CTRL_REG1, &value, 1);
	} else {
		get_gsensor_rawdata();
	}
}

void gsensor_start_upload_rawdata(void)
{
	uint32_t err_code;

	if (true == m_gsensor_factory_test) {
		m_factory_test_id = 0;
	}
	
	if (!m_upload_timer) {
		m_upload_timer = true;
		err_code = app_timer_start(m_gsensor_upload_timer_id, GSENSOR_UPLOAD_INTERVAL, NULL);
		APP_ERROR_CHECK(err_code);
	}
}

void gsensor_stop_upload_rawdata(void)
{
	uint32_t err_code;
	
	if (m_upload_timer) {
		m_upload_timer = false;
		err_code = app_timer_stop(m_gsensor_upload_timer_id);
		APP_ERROR_CHECK(err_code);
	}
}

static void gsensor_upload_timeout_handler(void)
{
	uint8_t loop;
	
	ble_nus_gsensor_rawdata[0] = 0x41;
	ble_nus_gsensor_rawdata[1] = 0x04;
	ble_nus_gsensor_rawdata[3] = 0x01;

	if (false == m_gsensor_factory_test) {
		if (m_upload_index < 7) {
			for (loop = 0; loop < 16; loop++) 
				ble_nus_gsensor_rawdata[loop+4] = upload_data[m_upload_index*16+loop];
			ble_nus_gsensor_rawdata[2] = 0x10;
			ble_send_packet(ble_nus_gsensor_rawdata, 20, 10);
			m_upload_index++;
		} else {
			for (loop = 0; loop < 14; loop++) 
				ble_nus_gsensor_rawdata[loop+4] = upload_data[m_upload_index*16+loop];
			ble_nus_gsensor_rawdata[2] = 0x0E;
			ble_send_packet(ble_nus_gsensor_rawdata, 18, 10);
			m_upload_index = 0;
		}
	} else if (true == m_gsensor_factory_test) {
		ble_nus_gsensor_rawdata[2] = 0x0E;
		ble_nus_gsensor_rawdata[4] = m_factory_test_id;
		if (m_factory_test_id < 0x0C) {
			for (loop = 0; loop < 12; loop++) {
				ble_nus_gsensor_rawdata [loop+5] = gsensor_test_array[m_factory_test_id*12+loop];
			}
		} else {
			for (loop = 0; loop < 12; loop++) {
				ble_nus_gsensor_rawdata [loop+5] = gsensor_test_array[(m_factory_test_id-0x0C)*12+loop];
			}			
		}
		ble_send_packet(ble_nus_gsensor_rawdata, 17, 10);
		if (m_factory_test_id < 0x0D) {
			m_factory_test_id++;
		} else {
			gsensor_stop_upload_rawdata();
		}
	}
}

static void gsensor_timers_init(void)
{
	uint32_t err_code = app_timer_create(&m_gsensor_timer_id,
							APP_TIMER_MODE_REPEATED,
							gsensor_timeout_handler);
    APP_ERROR_CHECK(err_code);
	
	err_code = app_timer_create(&m_gsensor_upload_timer_id,
							APP_TIMER_MODE_REPEATED,
							gsensor_upload_timeout_handler);
    APP_ERROR_CHECK(err_code);
	
	err_code = app_timer_start(m_gsensor_timer_id, GSENSOR_START_INTERVAL, NULL);
	APP_ERROR_CHECK(err_code);
}

uint32_t drv_kxo22_init(void)
{
	nrf_drv_spi_config_t spi_config = NRF_DRV_SPI_GSENSOR_CONFIG(SPI_GSENSOR_INSTANCE);  
	APP_ERROR_CHECK(nrf_drv_spi_init(&spi, &spi_config, 0));

	// by default, both INT_1 and INT_2 are configured to push-pull active high level in INT_OUT_CTRL register
	NRF_GPIO->PIN_CNF[IO_KXO22_INT_2_PIN] = (GPIO_PIN_CNF_SENSE_Disabled     << GPIO_PIN_CNF_SENSE_Pos) |
																							(GPIO_PIN_CNF_DRIVE_S0S1       << GPIO_PIN_CNF_DRIVE_Pos) |
																							(GPIO_PIN_CNF_PULL_Disabled    << GPIO_PIN_CNF_PULL_Pos)  |
																							(GPIO_PIN_CNF_INPUT_Connect    << GPIO_PIN_CNF_INPUT_Pos) |
																							(GPIO_PIN_CNF_DIR_Input        << GPIO_PIN_CNF_DIR_Pos); 

	nrf_delay_us(30000);
	sdio_write_byte(0x7F, 0x00);
	sdio_write_byte(0x19, 0x00);
	sdio_write_byte(0x19, 0x80);
	nrf_delay_us(10000);
	// verify interface by checking chip id
	kxo22_chip_id = sdio_read_byte(ACC_REG_ADDR_WHO_AM_I);
	if (kxo22_chip_id != REG_VAL_WHO_AM_I) {
        return NRF_ERROR_TIMEOUT;
	}
	// verify interface by checking chip content of COTR register
	kxo22_chip_cotr = sdio_read_byte(ACC_REG_ADDR_DCST_RESP);
	if (kxo22_chip_cotr != REG_VAL_START_OK) {
        return NRF_ERROR_TIMEOUT;
	}

#ifndef DONT_TRACE_UART  //TODO: remove later; this is just to test write/read of register
	// REG_PMU_RANGE reset value is 0x03 (+/- 2g range)
	sdio_write_byte(REG_PMU_RANGE, 0x05); // +/- 4g range
	if (sdio_read_byte(REG_PMU_RANGE) != 0x05) {
			return NRF_ERROR_INTERNAL;
	}
#endif
	
	gsensor_timers_init();

	sdio_write_byte(ACC_REG_ADDR_CTRL_REG1, 0x10);//配置量程为8G
	sdio_write_byte(ACC_REG_ADDR_BUF_CTRL1, 0x19);//设置FIFO模式的数量（25）
	sdio_write_byte(ACC_REG_ADDR_BUF_CTRL2, 0xE0);
	sdio_write_byte(ACC_REG_ADDR_ODR_CNTL, 0x01);//配置采样率为25HZ
	sdio_write_byte(ACC_REG_ADDR_LP_CNTL, 0x0B);//配置平均组数 目前为0 
	sdio_write_byte(ACC_REG_ADDR_INT_CTRL_REG5, 0x30);//配置INT2的模式 HIGH 边沿触发
	sdio_write_byte(ACC_REG_ADDR_INT_CTRL_REG6, 0x04);//配置INT2的中断源是TAP
	sdio_write_byte(ACC_REG_ADDR_CTRL_REG3, 0x98);//采样率 1600 不配置是400(0X98)
	sdio_write_byte(ACC_REG_ADDR_TDT_EN, 0x01);
	nrf_delay_us(50000);
	sdio_read_byte(ACC_REG_ADDR_INT_REL);//清除中断源
	sdio_write_byte(ACC_REG_ADDR_BUF_CLEAR, 0xFF);

	return NRF_SUCCESS;
}
