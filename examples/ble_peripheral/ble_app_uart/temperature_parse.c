#include "temperature_parse.h"
#include "bsp.h"
#include "nrf_drv_saadc.h"
#include <include/gpio.h>

#define	TEMP_VOLTAGE(x)			(((float)x/1023)*3.6)
#define LDO_OUT_VOLTAGE			(3.3f)

#define PRECISION 				(0.0001f)
#define INVALID_TEMPERATURE		(0.0f)
//#define TEMPERATURE_DEBUG	

void get_temperature_value(float * ptemperature)
{
	ret_code_t err_code;
	nrf_saadc_value_t adc = 0;
	float  resistance = 0;	
	uint16_t m = 0,n = ((sizeof(temperature_array)/(sizeof(float)*4)) - 1);
	uint16_t k = (n + m)/2;
	float  diff  = 0.0f;
	float  diff1 = 0.0f;
	float  diff2 = 0.0f;
	float  diff3 = 0.0f;
	
	gpio_set_dir(EXGPIO_3, GPIO_DIR_OUT, GPIO_CFG_OUT_HI_DISABLE);
	gpio_set_out(EXGPIO_3, GPIO_OUT_ONE);
	
	err_code = nrf_drv_saadc_sample_convert(7, &adc);
	APP_ERROR_CHECK(err_code);
	
#ifndef TEMPERATURE_DEBUG	
	resistance = (TEMP_VOLTAGE(adc) * 100) / (LDO_OUT_VOLTAGE - TEMP_VOLTAGE(adc));
#else
	resistance = 63.7343;
#endif
		
	if ((resistance < temperature_array[(sizeof(temperature_array)/(sizeof(float)*4)) - 1][2]) ||
	    (resistance > temperature_array[0][2])) {
		*ptemperature = INVALID_TEMPERATURE;
		return;
	}
		
	while (m < (n-1)) {
		
		diff = resistance - temperature_array[k][2];
		diff = (diff >= 0)?diff:(-diff);
		
		if ((resistance < temperature_array[k][2]) && (diff >= PRECISION)) {
			m = k;
			k = (n + m)/2;
		} else if ((resistance > temperature_array[k][2]) && (diff >= PRECISION)) {
			n = k;
			k = (n + m)/2;
		} else if (diff < PRECISION) {
			*ptemperature = temperature_array[k][0];
			return;
		}
	}
	
	if ((k-1) >= 0) {
		diff1 = resistance - temperature_array[k-1][2];
		diff1 = (diff1 >= 0)?diff1:(-diff1);
	} else {
		diff1 = 255.0;
	}
	
	if ((k+1) < (sizeof(temperature_array)/(sizeof(float)*4))) {
		diff2 = resistance - temperature_array[k+1][2];
		diff2 = (diff2 >= 0)?diff2:(-diff2);
	} else {
		diff2 = 255.0;
	}
	
	diff3 = resistance - temperature_array[k][2];
	diff3 = (diff3 >= 0)?diff3:(-diff3);
	
	if ((diff1 <= diff2) && (diff1 <= diff3)) {
		*ptemperature = temperature_array[k-1][0];
		return;
	}
	
	if ((diff2 <= diff1) && (diff2 <= diff3)) {
		*ptemperature = temperature_array[k+1][0];
		return;
	}
	
	if ((diff3 <= diff1) && (diff3 <= diff2)) {
		*ptemperature = temperature_array[k][0];
		return;
	}
}

nrf_saadc_value_t  get_temperature_adc(void)
{
	ret_code_t err_code;
	nrf_saadc_value_t adc = 0;
	
	gpio_set_dir(EXGPIO_3, GPIO_DIR_OUT, GPIO_CFG_OUT_HI_DISABLE);
	gpio_set_out(EXGPIO_3, GPIO_OUT_ONE);
	
	err_code = nrf_drv_saadc_sample_convert(7, &adc);
	APP_ERROR_CHECK(err_code);
	
	return adc;
}
