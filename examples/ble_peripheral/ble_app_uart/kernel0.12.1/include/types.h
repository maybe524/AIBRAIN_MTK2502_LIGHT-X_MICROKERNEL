#pragma once

#ifdef __arm__
#include "include/arm/types.h"
#else
#include "include/arm/types.h" // fixme
// #error "arch not supported yet"
#endif

typedef unsigned char  __u8, u8, byte, uint8_t, u_char;
typedef unsigned short __u16, u16, uint16_t;
typedef unsigned int   __u32, u32, uint32_t, umode_t, time_t;
typedef unsigned long  __size_t, u_long, blkcnt_t;	//typedef unsigned long  size_t, u_long, blkcnt_t;
typedef signed int     ssize_t;
typedef unsigned long long uint64_t, u64;
typedef unsigned long loff_t; // fixme : unsigned long long

#define __user

// fixme
typedef unsigned short __le16;
typedef unsigned int   __le32;

typedef unsigned int dev_t, uid_t, gid_t;
typedef unsigned long __kernel_time_t;

typedef enum {kd_false, kd_true} kd_bool;

#define VA(x) ((void *)(x))


#define likely(x)	(x)
#define unlikely(x)	(!(x))

#define cpu_to_le16(x)         (x)
#define le16_to_cpu(x)         (x)

#define cpu_to_le32(x)         (x)
#define le32_to_cpu(x)         (x)

#define CPU_TO_BE16(val)      (((val) >> 8) | (((val) & 0xff) << 8))
#define BE16_TO_CPU(val)      (((val) >> 8) | (((val) & 0xff) << 8))

#define BE32_TO_CPU(val)      ((((val) >> 24) & 0xff) | (((val) >> 8) & 0xff00) | ((((val) << 8) & 0xff0000)) | ((val) << 24))
#define CPU_TO_BE32(val)      ((((val) >> 24) & 0xff) | (((val) >> 8) & 0xff00) | ((((val) << 8) & 0xff0000)) | ((val) << 24))

#define typeof(type)	(struct (type))

#define ALIGN_UP(len, align) (((len) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_UP2(len, align) \
	do { \
		__size_t _________tmp; \
		if (!((align) & ((align) - 1))) { \
			(len) = (len + align - 1) & ~((align) - 1); \
		} else if (((_________tmp) = (len) % (align))) { \
			(len) += (align) - (_________tmp); \
		} \
	} while (0)
#define ALIGN_DOWN(len, align)	((len) & ~(align - 1))
#define ALIGN_CHECK(len, align)	(((len) & (align - 1)) == 0)

#define ENDING_SWAP16(n)	(((n) & 0xFF) << 8 | (n) >> 8)
#define ENDING_SWAP32(n)	(((n) & 0xFF) << 24 | ((n) & (0xFF << 8)) << 8 | ((n) & (0xFF << 16)) >> 8 | (n) >> 24)

#define __PACKED__        __attribute__((packed))
#define __WEAK__          __attribute__((weak))

#if __GNUC__ == 3 && __GNUC_MINOR__ >= 3 || __GNUC__ >= 4
#define __USED__          __attribute__((__used__))
#define __UNUSED__        __attribute__((__unused__))
#else
#define __USED__          __attribute__((__unused__))
#define __UNUSED__        // right ?
#endif

// #define PRINT_ARG_FORMAT __attribute__((format (printf, 1, 2)))

#define DECL_RESET(func) \
	void reset(void) __attribute__((alias(#func)))

#define NULL              ((void *)0)
#define KB(n)             ((n) << 10)
#define MB(n)             ((n) << 20)
#define GB(n)             ((n) << 30)

#define ARRAY_ELEM_NUM(arr) (sizeof(arr) / sizeof((arr)[0]))

#define container_of(ptr, type, member) \
	(type *)((char *)ptr - (long)(&((type *)0)->member))

#define min(x, y)         ((x) < (y) ? (x) : (y))
#define max(x, y)         ((x) > (y) ? (x) : (y))

// fixme
#define SWAP(a,b) \
	do { \
		typeof(a) _________tmp; \
		_________tmp = (a); \
		(a) = (b); \
		(b) = _________tmp; \
	} while(0)

#ifdef __GBIOS_VER__
#define GAPI
#define EXPORT_SYMBOL(func)
#else
#define __capi    /* LongCheer api */
#define __tapi  static
#define GAPI    static
#define EXPORT_ALIA_SYMBOL(sym) \
		__typeof(sym) __##sym __attribute__((alias(#sym)))
#define EXPORT_SYMBOL(func)
#endif
