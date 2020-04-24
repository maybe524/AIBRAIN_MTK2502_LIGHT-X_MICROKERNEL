#pragma once

#include "include/stdio.h"

#define BUG() \
	do { \
		GEN_DBG("Bug @ %s() line %d of %s!\r\n", __func__, __LINE__, __FILE__); \
		while (1); \
	} while(0)

#define assert(x) \
	do { if (!(x)) BUG(); } while (0)
