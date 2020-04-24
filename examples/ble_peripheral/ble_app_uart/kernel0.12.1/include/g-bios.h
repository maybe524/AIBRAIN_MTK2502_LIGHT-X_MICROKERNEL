#pragma once

// #include <include/autoconf.h>

#ifdef CONFIG_NORMAL_SPACE
#warning "depricated"
#endif

// fixme!
#define CONFIG_HEAD_SIZE 	0x2000
#define CONFIG_HEAP_SIZE		0x800

#define CONFIG_UART_INDEX	0x01
#define CONFIG_DEBUG
// #define CONFIG_IRQ_SUPPORT

// fixme
#define FILE_NAME_SIZE   10 //256

#ifndef __ASSEMBLY__
#include <include/sysconf.h>

#define VA(x) ((void *)(x))
#endif
