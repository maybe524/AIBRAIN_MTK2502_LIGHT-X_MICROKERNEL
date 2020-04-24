#pragma once

#include "include/types.h"

#define LCT_OPT_ALL_CNT	7

struct opt_item {
	kd_bool		opt_result;
    kd_bool   	opt_lock;
    void	*opt_cb;
};

typedef struct gen_opt_status_t {
    uint8_t         curr_status;
    struct opt_item     *opt_element[LCT_OPT_ALL_CNT];
} gen_opt_status_t;

static inline void lock(kd_bool *lock_obj)
{
    *lock_obj = kd_true;
}

static inline void unlock(kd_bool *lock_obj)
{
    *lock_obj = kd_false;
}

static inline kd_bool check_lock(kd_bool *lock_obj)
{
    return *lock_obj;
}
