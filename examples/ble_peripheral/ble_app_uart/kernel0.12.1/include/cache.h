#pragma once

#include "types.h"

#define CACHE_MAX	3

#define CACHE_REQUEST	0x01

struct cache_info {
	unsigned short offsize; 
	unsigned char id;
	char data[KB(1)];
};

struct cache_opts {
	char co_cmd;
	char co_level;
	kd_bool co_need_store;
};


void *cache_alloc(__size_t size);