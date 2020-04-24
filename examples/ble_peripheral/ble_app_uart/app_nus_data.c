#include "bsp.h"
#include "nordic_common.h"
#include "nrf.h"
#include "ble_nus.h"
#include "nrf_delay.h"
#include "softdevice_handler.h"
#include "nrf_gpio.h"
#include "app_nus_data.h"
#include "ble_hci.h"

#ifdef CONFIG_KERNEL_SYSTEM
#include <include/types.h>
#endif

static struct app_nus_datfmt {
	uint8_t idx;
	uint8_t key;
	uint8_t len;
	uint8_t val[17];
};

kd_bool ble_isconnected = kd_false;

kd_bool app_nus_ble_isconnected(void)
{
	return ble_isconnected;
}
#define GPREGRET_ENTER_DFU      0x80

static const uint8_t  ble_version[] = "PG160616001";
static uint8_t ble_nus_data[20] = {0};

extern void gsensor_start_upload_rawdata(void);
extern ble_nus_t * get_nus_conn_handle(void);
extern uint8_t sn[11];
extern bool	m_gsensor_factory_test;
extern uint8_t boot_ver_num[4];

static void app_enter_dfu(void)
{    
    uint32_t err_code;
    
    err_code = sd_power_gpregret_set(GPREGRET_ENTER_DFU);	
    APP_ERROR_CHECK(err_code);   
	NVIC_SystemReset();
}

static uint32_t uint64_array_to_uint32(uint8_t *buf) 
{
	uint64_t data64;
	data64 = ((((uint64_t) buf[0] & 0xff) << 56)
						| (((uint64_t) buf[1] & 0xff) << 48)
						| (((uint64_t) buf[2] & 0xff) << 40)
						| (((uint64_t) buf[3] & 0xff) << 32)
						| (((uint64_t) buf[4] & 0xff) << 24)
						| (((uint64_t) buf[5] & 0xff) << 16)
						| (((uint64_t) buf[6] & 0xff) << 8)
						| (((uint64_t) buf[7] & 0xff) << 0));
	uint32_t data = (uint32_t) (data64 & 0xffffffff);
	return data;
}

static void set_packet_time_info(uint8_t * buf) 
{
	uint32_t timestamp = get_timestamp();
	
	*buf = get_timezone();	
	
	*(buf+1) = (uint8_t)((timestamp >> 0)  & 0xff);
	*(buf+2) = (uint8_t)((timestamp >> 8)  & 0xff); 
	*(buf+3) = (uint8_t)((timestamp >> 16) & 0xff); 
	*(buf+4) = (uint8_t)((timestamp >> 24) & 0xff); 		
}

void app_nus_data_handler(ble_nus_t * p_nus, uint8_t * p_data, uint16_t length)
{
	int ret;
	struct app_nus_datfmt *dfmt = (struct app_nus_datfmt *)p_data;

	switch (p_data[0]) {
		case BLE_ID_GET_SENSOR_RAWDATA:
			if (BLE_OP_KEY_GET == p_data[1]) {
				if (0x01 == p_data[3]) {
					gsensor_start_upload_rawdata();
				} 
			} else if (BLE_OP_KEY_SET == p_data[1]) {
				if (0x01 == p_data[3]) {
					memset(ble_nus_data, 0, sizeof(ble_nus_data));
					ble_nus_data[BLE_DATA_HEAD_ID] = BLE_ID_GET_SENSOR_RAWDATA;
					ble_nus_data[BLE_DATA_HEAD_OP_KEY] = BLE_OP_KEY_ACK;
					ble_nus_data[BLE_DATA_HEAD_LENGTH] = 0x01;
					ble_nus_data[BLE_DATA_HEAD_LENGTH+1] = 0x0;
					ble_send_packet(ble_nus_data,
									ble_nus_data[BLE_DATA_HEAD_LENGTH]+3,
					                10);
					gsensor_stop_upload_rawdata();
				}
			}
			break;
		case BLE_ID_SET_SYSTEM_TIME:
			set_timezone(p_data[3]);
			set_timestamp(uint64_array_to_uint32(&p_data[4]));
			memset(ble_nus_data, 0, sizeof(ble_nus_data));
			ble_nus_data[BLE_DATA_HEAD_ID] = BLE_ID_SET_SYSTEM_TIME;
			ble_nus_data[BLE_DATA_HEAD_OP_KEY] = BLE_OP_KEY_ACK;
		    ble_nus_data[BLE_DATA_HEAD_LENGTH] = 0x09;
			ble_nus_data[BLE_DATA_HEAD_LENGTH+1] = p_data[3];
		    memcpy(&(ble_nus_data[BLE_DATA_HEAD_LENGTH+2]), &p_data[4], 0x08);
			ble_send_packet(ble_nus_data, 
							ble_nus_data[BLE_DATA_HEAD_LENGTH]+3,
							10);		
			break;
		case BLE_ID_DUF_MODE:
			app_enter_dfu();
			break;
		case BLE_ID_GET_VERSION_NUMER:
			mesure_start_up("gver");
			break;
		case BLE_ID_GET_TEMPERATURE:
			float temperature1 = 0.0f;
            uint8_t	temp1 = 0;
			uint8_t temp2 = 0;
			get_temperature_value(&temperature1);
			uint16_t temperature = (uint16_t)(temperature1*100);
			temp2 = (uint8_t)(temperature % 100);
			temp1 = (uint8_t)((temperature - temp2)/100);
			memset(ble_nus_data, 0, sizeof(ble_nus_data));
			ble_nus_data[BLE_DATA_HEAD_ID] = BLE_ID_GET_TEMPERATURE;
			ble_nus_data[BLE_DATA_HEAD_OP_KEY] = BLE_OP_KEY_RESULT;
		    ble_nus_data[BLE_DATA_HEAD_LENGTH] = 0x07;
		    set_packet_time_info(&ble_nus_data[BLE_DATA_HEAD_LENGTH + 1]);
			ble_nus_data[BLE_DATA_HEAD_LENGTH + 6] = temp1;
			ble_nus_data[BLE_DATA_HEAD_LENGTH + 7] = temp2;
			ble_send_packet(ble_nus_data, 
							ble_nus_data[BLE_DATA_HEAD_LENGTH]+3,
		                    10);			
			break;
		case BLE_ID_GET_TEMPERATURE_ADC:
			uint16_t adc = get_temperature_adc();
			memset(ble_nus_data, 0, sizeof(ble_nus_data));
			ble_nus_data[BLE_DATA_HEAD_ID] = BLE_ID_GET_TEMPERATURE_ADC;
			ble_nus_data[BLE_DATA_HEAD_OP_KEY] = BLE_OP_KEY_RESULT;
		    ble_nus_data[BLE_DATA_HEAD_LENGTH] = sizeof(uint16_t);
			ble_nus_data[BLE_DATA_HEAD_LENGTH + 1] = (adc >> 8) & 0x00FF;
			ble_nus_data[BLE_DATA_HEAD_LENGTH + 2] = adc & 0x00FF;
			ble_send_packet(ble_nus_data, 
							ble_nus_data[BLE_DATA_HEAD_LENGTH]+3,
							10);
			break;
		case BLE_ID_GET_SERIAL_NUMER:
			memset(ble_nus_data, 0, sizeof(ble_nus_data));
			ble_nus_data[BLE_DATA_HEAD_ID] = BLE_ID_GET_SERIAL_NUMER;
			ble_nus_data[BLE_DATA_HEAD_OP_KEY] = BLE_OP_KEY_RESULT;
		    ble_nus_data[BLE_DATA_HEAD_LENGTH] = sizeof(sn);
			memcpy(&(ble_nus_data[BLE_DATA_HEAD_LENGTH+1]), sn, sizeof(sn));
			ble_send_packet(ble_nus_data, 
							ble_nus_data[BLE_DATA_HEAD_LENGTH]+3,
							10);
			break;
		case BLE_ID_GET_BP:
			mesure_start_up("bhrr");
			break;
		case BLE_ID_SET_VIBTATION:
			memset(ble_nus_data, 0, sizeof(ble_nus_data));
			ble_nus_data[BLE_DATA_HEAD_ID] = BLE_ID_SET_VIBTATION;
			ble_nus_data[BLE_DATA_HEAD_OP_KEY] = BLE_OP_KEY_ACK;
		    ble_nus_data[BLE_DATA_HEAD_LENGTH] = 0x01;
			ble_nus_data[BLE_DATA_HEAD_LENGTH+1] = p_data[3];
			ble_send_packet(ble_nus_data, 
							ble_nus_data[BLE_DATA_HEAD_LENGTH]+3,
							10);
			if (0x01 == p_data[3]) {
				shell("factory -vibr");
			}
			break;
		case BLE_ID_FACTORY_G_TEST:
			memset(ble_nus_data, 0, sizeof(ble_nus_data));
			ble_nus_data[BLE_DATA_HEAD_ID] = BLE_ID_FACTORY_G_TEST;
			ble_nus_data[BLE_DATA_HEAD_OP_KEY] = BLE_OP_KEY_ACK;
			ble_nus_data[BLE_DATA_HEAD_LENGTH] = 0x01;
			ble_nus_data[BLE_DATA_HEAD_LENGTH+1] = p_data[3];
			ble_send_packet(ble_nus_data, 
						ble_nus_data[BLE_DATA_HEAD_LENGTH]+3,
						10);	
			if ((0x01 == p_data[1]) && (0x01 == p_data[3])) {
				m_gsensor_factory_test = true;
			} else if ((0x01 == p_data[1]) && (0x0 == p_data[3])) {
				m_gsensor_factory_test = false;
			}
			break;
		case BLE_ID_SET_USER_INFO:
			mt2502_set_userinfo(&p_data[3]);
			memset(ble_nus_data, 0, sizeof(ble_nus_data));
			ble_nus_data[BLE_DATA_HEAD_ID] = BLE_ID_SET_USER_INFO;
			ble_nus_data[BLE_DATA_HEAD_OP_KEY] = BLE_OP_KEY_ACK;
			ble_nus_data[BLE_DATA_HEAD_LENGTH] = 0x0C;
			memcpy(&(ble_nus_data[BLE_DATA_HEAD_LENGTH+1]), &p_data[3], ble_nus_data[BLE_DATA_HEAD_LENGTH]);
			ble_send_packet(ble_nus_data, 
						ble_nus_data[BLE_DATA_HEAD_LENGTH]+3,
						10);
			break;
		case BLE_ID_GET_STEP_INFO:
			break;
		case BLE_ID_GET_BLE_FLASH_DATA:
			break;
		case BLE_ID_GET_BOOT_VER_NUM:
			memset(ble_nus_data, 0, sizeof(ble_nus_data));
			ble_nus_data[BLE_DATA_HEAD_ID] = BLE_ID_GET_BOOT_VER_NUM;
			ble_nus_data[BLE_DATA_HEAD_OP_KEY] = BLE_OP_KEY_RESULT;
		    ble_nus_data[BLE_DATA_HEAD_LENGTH] = sizeof(uint32_t);
			memcpy(&(ble_nus_data[BLE_DATA_HEAD_LENGTH+1]), boot_ver_num, sizeof(uint32_t));
			ble_send_packet(ble_nus_data, 
							ble_nus_data[BLE_DATA_HEAD_LENGTH]+3,
							10);
			break;
		default:
			break;
	}
}

int app_getversion(char *buff)
{
	kmemcpy(buff, ble_version, kstrlen(ble_version));
	return 0;
}
