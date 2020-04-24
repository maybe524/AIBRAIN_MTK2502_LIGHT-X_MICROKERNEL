#pragma once

#include <stdint.h>

#if 0
#include "app_util_platform.h"
#include "nrf.h"
#include "simple_uart.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#endif

#define RX_PIN_NUMBER  16   // UART RX pin number.
#define TX_PIN_NUMBER  17   // UART TX pin number.
#define CTS_PIN_NUMBER 18   // UART Clear To Send pin number. Not used if HWFC is set to false. 
#define RTS_PIN_NUMBER 19   // UART Request To Send pin number. Not used if HWFC is set to false. 
#define HWFC	true		// UART hardware flow control.

