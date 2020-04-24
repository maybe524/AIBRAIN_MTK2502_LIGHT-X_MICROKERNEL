#pragma once
//#ifdef ____SWAP_FUNC__
//#define ____SWAP_FUNC__

#include "include/types.h"

#define ___cswab32(x) ((__u32)(             \
	(((__u32)(x) & (__u32)0x000000ffUL) << 24) |        \
    (((__u32)(x) & (__u32)0x0000ff00UL) <<  8) |        \
    (((__u32)(x) & (__u32)0x00ff0000UL) >>  8) |        \
    (((__u32)(x) & (__u32)0xff000000UL) >> 24)))

#define ___cswab16(x) ((__u16)(		\
	(((__u16)(x) & (__u16)0x00ffUL) << 8) |			\
    (((__u16)(x) & (__u16)0xff00UL) >> 8)))

static inline __u32 
___fswab32(__u32 val)
{
	return ___cswab32(val);
}

static inline __u16
___fswab16(__u16 val)
{
	return ___cswab16(val);
}

static inline void 
fswab32(__u32 *val)
{
	*val = ___cswab32(*val);
}

static inline void 
fswab16(__u16 *val)
{
	*val = ___cswab16(*val);
}

static inline __u32
f2h_corret32(__u32 val)
{
	return (val & 0x80) ? 0 : ___fswab32(val);
}

static inline __u16
f2h_corret16(__u16 val)
{
	return (val & 0x80) ? 0 : ___fswab16(val);
}

static inline __u16
___corret16(__u16 val)
{
	return (val & 0x8000) ? 0 : val;
}


//#endif
