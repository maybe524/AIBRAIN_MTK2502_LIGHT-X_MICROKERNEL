#pragma once

#define BUTTON_ACTIVE_LOW		0x00
#define BUTTON_ACTIVE_HIGH		0x01

#define BUTTON_RELEASE			0x00
#define BUTTON_PUSH				0x01

#define BUTTON_GPIO_PIN_NOPULL		0x00
#define BUTTON_GPIO_PIN_PULLDOWN	0x01
#define BUTTON_GPIO_PIN_PULLUP		0x03


struct button_info {
    __u8 pin_no;
    __u8 active_state;     /**< APP_BUTTON_ACTIVE_HIGH or APP_BUTTON_ACTIVE_LOW. */
    __u8 pull_cfg;         /**< Pull-up or -down configuration. */

    void (*button_handler)(__u8 pin_no, __u8 button_action);
};

struct button_driver {
	int (*button_register)(struct button_info *button_info, __u8 button_count, __u32 detection_delay);
	int (*button_enable)(void);
};

#ifdef CONFIG_PLATFORM_NRFS
#include "app_timer.h"
#define	BUTTON_DETECTION_DEBOUNCE         30 //ms
#define	BUTTON_DETECTION_DELAY            APP_TIMER_TICKS(BUTTON_DETECTION_DEBOUNCE, 2)
#endif
