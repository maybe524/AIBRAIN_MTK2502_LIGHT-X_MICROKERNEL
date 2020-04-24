#pragma once

#include "include/types.h"
#include "include/list.h"
#include "include/gpio.h"

#define IRQ_NONE       0
#define IRQ_HANDLED    1

#define IRQ_TYPE_NONE			0x00000000
#define IRQ_TYPE_RISING			0x00000001
#define IRQ_TYPE_FALLING		0x00000002
#define IRQ_TYPE_BOTH			(IRQ_TYPE_FALLING | IRQ_TYPE_RISING)
#define IRQ_TYPE_HIGH			0x00000004
#define IRQ_TYPE_LOW			0x00000008

#define IRQ_CFG_LEVEL_HIGH		GPIO_CFG_LEVEL_HIGH
#define IRQ_CFG_LEVEL_LOW		GPIO_CFG_LEVEL_LOW
#define IRQ_CFG_TRIGGLE_RISING	GPIO_CFG_TRIGGLE_RISING
#define IRQ_CFG_TRIGGLE_FALLING	GPIO_CFG_TRIGGLE_FALLING
#define IRQ_CFG_HI_ACCURACY		GPIO_CFG_HI_ACCURACY
#define IRQ_CFG_NOPULL			GPIO_CFG_NOPULL
#define IRQ_CFG_PULLDOWN		GPIO_CFG_PULLDOWN
#define IRQ_CFG_PULLUP			GPIO_CFG_PULLUP
#define IRQ_CFG_WATCHER			GPIO_CFG_WATCHER

struct int_pin;
struct irq_dev;

typedef	void (*IRQ_PIN_HANDLER)(struct int_pin *, __u32);
typedef int  (*IRQ_DEV_HANDLER)(__u32 pin, __u32 action);

struct int_ctrl {
	//void (*ack)(__u32);
	int (*mask)(__u32 pin);
	//void (*mack)(__u32);
	int (*unmask)(__u32 pin);
	//int	 (*set_trigger)(__u32, __u32);
};

struct int_pin {
	IRQ_PIN_HANDLER	 irq_handle;
	struct int_ctrl	 *intctrl;
	struct irq_dev	 *dev_list;
};

struct irq_dev {
	void             *device;
	IRQ_DEV_HANDLER  dev_isr;
	struct irq_dev   *next;
};

struct irq_driver {
	char	*type;
	__u32	pin;
	__u32	confg;

	/*
	 *	confg:
	 *	low char is using config IRQ_TYPE,
	 *	include IRQ_TYPE_RISING, IRQ_TYPE_FALLING, IRQ_TYPE_HIGH, IRQ_TYPE_LOW
	 */
	int (*irq_register_isr)(__u32 pin, __u32 confg, IRQ_DEV_HANDLER isr, void *dev);
	struct int_ctrl int_ctrl;

	struct list_head list;
};

#define irq_info(fmt, arg...)	printk("IRQ: "fmt, ##arg)

int irq_register_isr(char *type, __u32 pin, __u32 confg, IRQ_DEV_HANDLER isr);
int irq_register_driver(struct irq_driver *driver);
int irq_mask(__u32 pin);
int irq_unmask(__u32 pin);