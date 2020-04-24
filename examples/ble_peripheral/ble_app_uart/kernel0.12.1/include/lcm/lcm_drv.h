#pragma once

#include <include/graphic/display.h>

#define DECLARE_LCM_DRV_INIT(func)		int lcm_drv_init(void) 		__attribute__((alias(#func)))
#define DECLARE_LCM_DRV_SUSPEND(func)	int lcm_drv_suspend(void) 	__attribute__((alias(#func)))
#define DECLARE_LCM_DRV_RESUME(func)	int lcm_drv_resume(void) 	__attribute__((alias(#func)))
#define DECLARE_LCM_DRV_UPDATE(func)	int lcm_drv_update(void *buff, __u32 x, __u32 y, __u32 width, __u32 height, __u32 flags) 	__attribute__((alias(#func)))
#define DECLARE_LCM_DRV_GETINFO(func)	int lcm_drv_get_info(struct lcd_info *info) 	__attribute__((alias(#func)))
