#include <include/types.h>
#include <include/errno.h>
#include <include/stdio.h>

#include "./inc/pager_time.h"

#define LOCALTIME_CALCULATE_ALL		(1 << 0)	// recalculate

static __u32 time_stamp = 0;
static __u8 time_zone = 0;
kd_bool time_isavild = kd_false;
static struct localtime localdatetime = {0};
static __u32 time_minute_recode = 0;

int localtime_accumulator(__u8 flags)
{
	__u32 local_timestamp, local_second_total;

	local_timestamp = time_stamp + time_zone * 15 * 60;
    local_second_total = local_timestamp % 86400;

    localdatetime.Hour = local_second_total / 3600;
	localdatetime.Minute = (local_second_total / 60) % 60;
    localdatetime.Second = (local_second_total % 60);

	if (flags & LOCALTIME_CALCULATE_ALL || \
			(localdatetime.Hour == 0 && localdatetime.Minute == 0 && localdatetime.Second == 0)) {
		kasctime(local_timestamp, &localdatetime);
	}

	return 0;
}

__u8 get_timezone(void)
{
	return time_zone;
}

void set_timezone(__u8 timezone)
{
	time_zone = timezone;
	printk("Set timezone: %d\r\n", time_zone);
}

void set_timestamp(__u32 timestamp)
{
	int ret;
	struct localtime lt;

	time_stamp = timestamp;
	localtime_accumulator(LOCALTIME_CALCULATE_ALL);

	time_isavild = kd_true;
	ret = get_localtime(&lt);
	printk("Set timestamp: 0x%08x (%04d-%02d-%02d %d %02d:%02d:%02d).\r\n",
		time_stamp,
		lt.Year, lt.Month, lt.Day, lt.Week, lt.Hour, lt.Minute, lt.Second);

	ui_update_showtime();
}

time_t get_timestamp(void)
{
	return time_stamp;
}

void datetime_accumulator(void)
{
	if (!time_isavild)
		return;

	time_stamp++;

	localtime_accumulator(0);
}

int get_localtime(struct localtime *local_time)
{
	if (!time_isavild)
		return -EINVAL;
	kmemcpy(local_time, &localdatetime, sizeof(struct localtime));
	return 0;
}
