#include <include/wait.h>
#include <include/errno.h>

int ble_send_packet(__u8 *buf, __u8 len, __u32 timeout)
{
	int ret;
	wait_queue_head_t wq;

	if (!app_nus_ble_isconnected())
		return EEXIST;

	wait_event_timeout(wq, NULL, \
		!(ret = ble_nus_string_send(get_nus_conn_handle(), buf, len)), \
		timeout);

	return ret;
}

