#include <include/time.h>
#include <include/init.h>
#include <include/stdio.h>

static __u32 systime = 0;

#ifdef CONFIG_PLATFORM_NRFS 
#include "nrf_drv_timer.h"
#include "app_timer.h"

#define APP_TIMER_PRESCALER		0
APP_TIMER_DEF(timestamp_id);

static void nrfs_time_accumulator(void * p_context)
{
	systime++;
	datetime_accumulator();
}

static int nrfs_systime_init(void)
{
	__u32 ret;

	ret = app_timer_create(&timestamp_id, APP_TIMER_MODE_REPEATED, nrfs_time_accumulator);
	ret = app_timer_start(timestamp_id, APP_TIMER_TICKS(1000, APP_TIMER_PRESCALER), NULL);

	return ret;
}
#else
#error "Check Timestamp init"
#endif

time_t get_systime(void)
{
	return systime;
}

int __init systime_init(void)
{
#ifdef CONFIG_PLATFORM_NRFS 
	nrfs_systime_init();
#endif

	return 0;
}
