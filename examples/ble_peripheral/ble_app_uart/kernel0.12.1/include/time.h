#pragma once

#include "include/types.h"

struct timespec {
	__kernel_time_t	tv_sec;			/* seconds */
	long		tv_nsec;		/* nanoseconds */
};

struct localtime {
	__u8 Year;
	__u8 Month;
	__u8 Day;
	__u8 Week;

	__u8 Zone;
	__u8 Hour;
	__u8 Minute;
	__u8 Second;
};


#define Minute(v)	(60 * (v))
#define Hour(v)		(60 * Minute((v)))
#define Second(v)	((v))

unsigned int get_systime(void);
