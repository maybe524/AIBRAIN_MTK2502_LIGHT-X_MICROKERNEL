#pragma once

#include <include/types.h>
#include <include/list.h>

#define GPIO_DIR_IN			0x00
#define GPIO_DIR_OUT		0x01

#define GPIO_OUT_ZERO		0x00
#define GPIO_OUT_ONE		0x01

#define GPIO_CFG_LEVEL_HIGH			((1 <<  8))
#define GPIO_CFG_LEVEL_LOW			((1 <<  9))
#define GPIO_CFG_TRIGGLE_RISING		((1 << 10))
#define GPIO_CFG_TRIGGLE_FALLING	((1 << 11))
#define GPIO_CFG_HI_ACCURACY		((1 << 12))
#define GPIO_CFG_NOPULL				((1 << 13))
#define GPIO_CFG_PULLDOWN			((1 << 14))
#define GPIO_CFG_PULLUP				((1 << 15))
#define GPIO_CFG_WATCHER			((1 << 16))
#define GPIO_CFG_OUT_HI_ENABLE		((1 << 17))
#define GPIO_CFG_OUT_HI_DISABLE		((1 << 18))

enum GPIO_NUM {
	GPIO_0 = 0,
	GPIO_1,  GPIO_2,  GPIO_3,  GPIO_4,
	GPIO_5,  GPIO_6,  GPIO_7,  GPIO_8,  GPIO_9,
	GPIO_10, GPIO_11, GPIO_12, GPIO_13, GPIO_14,
	GPIO_15, GPIO_16, GPIO_17, GPIO_18, GPIO_19,
	GPIO_20, GPIO_21, GPIO_22, GPIO_23, GPIO_24,
	GPIO_25, GPIO_26, GPIO_27, GPIO_28, GPIO_29,
};

enum EXGPIO_NUM {
	EXGPIO_0 = 0x0100,  EXGPIO_1,  EXGPIO_2,  EXGPIO_3,  EXGPIO_4,
	EXGPIO_5,  EXGPIO_6,  EXGPIO_7,  EXGPIO_8,  EXGPIO_9,
	EXGPIO_10, EXGPIO_11, EXGPIO_12, EXGPIO_13, EXGPIO_14,
	EXGPIO_15, EXGPIO_16, EXGPIO_17, EXGPIO_18, EXGPIO_19,
	EXGPIO_20, EXGPIO_21, EXGPIO_22, EXGPIO_23, EXGPIO_24,
	EXGPIO_25, EXGPIO_26, EXGPIO_27, EXGPIO_28, EXGPIO_29,
};

struct gpio_operations {
	int (*set_dir)(__u32 pin, __u8 dir, __u32 config);
	int (*set_out)(__u32 pin, __u8 out);
	int (*get_val)(__u32 pin);
};

struct gpio_board_info {
	char *name;
	__u16 rang_begin, rang_end;

	struct gpio_operations const *g_op;
	struct list_head list;
};

#define GPIO_PIN_MASK		0xFF
#define GPIO_PIN(pin)		((pin) & GPIO_PIN_MASK)

int gpio_set_mode(__u32 pin, __u8 mode);
int gpio_set_dir(__u32 pin, __u8 dir, __u32 config);
int gpio_set_out(__u32 pin, __u8 out);