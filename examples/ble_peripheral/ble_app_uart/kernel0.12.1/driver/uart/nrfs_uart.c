#include "include/io.h"
#include "include/delay.h"
#include "include/uart/uart.h"
#include "include/stdio.h"
#include "include/errno.h"
#include "app_uart.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "nrf.h"
#include "bsp.h"
#include "nrf_drv_config.h"

#define MAX_TEST_DATA_BYTES     (15U)                /**< max number of test bytes to be used for tx and rx. */
#define UART_TX_BUF_SIZE 256                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 16                           /**< UART RX buffer size. */


char uart_rxdat[100] = {0};
unsigned char uart_rxlen = 0;
bool uart_data_ready = false;
bool uart_buff_release = false;


void unlock_uart_buf(void)
{
	kmemset(uart_rxdat, 0, sizeof(uart_rxdat));
	uart_buff_release = true;
	uart_data_ready = false;
}

void lock_uart_buf(void)
{
    uart_data_ready = true;
}

static const app_uart_comm_params_t comm_params =
{
#ifdef CONFIG_PLATFORM_NRFS
	4,
	5,
	7,
	6,
	APP_UART_FLOW_CONTROL_ENABLED,
	false,
	UART_BAUDRATE_BAUDRATE_Baud115200,
#else
	RX_PIN_NUMBER,
	TX_PIN_NUMBER,
	RTS_PIN_NUMBER,
	CTS_PIN_NUMBER,
	APP_UART_FLOW_CONTROL_DISABLED,
	false,
	UART_BAUDRATE_BAUDRATE_Baud115200
#endif
};

static void nrfs_uart_error_handle(app_uart_evt_t * p_event)
{
#if 0
	if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR)
		APP_ERROR_HANDLER(p_event->data.error_communication);
	else if (p_event->evt_type == APP_UART_FIFO_ERROR)
		APP_ERROR_HANDLER(p_event->data.error_code);
#endif
	return;
}

int nrfs_uart_init(void)
{
	uint32_t err_code;

	APP_UART_FIFO_INIT(&comm_params,
			UART_RX_BUF_SIZE,
			UART_TX_BUF_SIZE,
			nrfs_uart_error_handle,
			APP_IRQ_PRIORITY_LOW,
			err_code);

	APP_ERROR_CHECK(err_code);
}

static __u32 nrfs_uart_rxbuf_count(void)
{
	return 0;
}

static __u8 nrfs_uart_recv_byte(void)
{
	return 0;
}

int nrfs_uart_send_byte(uint8_t cr)
{
	unsigned int timeout = 15000;

	while (app_uart_put(cr) && timeout--);

	return !timeout ? -EBUSY : 0;
}

DECLARE_UART_INIT(nrfs_uart_init);
DECLARE_UART_RECV(nrfs_uart_recv_byte);
DECLARE_UART_SEND(nrfs_uart_send_byte);
DECLARE_UART_RXBF_CNT(nrfs_uart_rxbuf_count);
