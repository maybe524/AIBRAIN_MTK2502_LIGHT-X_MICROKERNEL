#include <include/io.h>
#include <include/irq.h>
#include <include/errno.h>
#include <include/stdio.h>
#include <include/malloc.h>
#include <include/assert.h>
#include <include/stdio.h>
#include <include/errno.h>

LIST_HEAD(__irq_driver_list);
static struct irq_driver *irq_driver = NULL;

int irq_register_isr(char *type, __u32 pin, 
					__u32 confg, IRQ_DEV_HANDLER isr)
{
	if (!irq_driver)
		return -ENOMEM;

	return irq_driver->irq_register_isr(pin, confg, isr, (void *)irq_driver);
}

int irq_mask(__u32 pin)
{
	if (!irq_driver)
		return -ENOMEM;

	return irq_driver->int_ctrl.mask(pin);
}

int irq_unmask(__u32 pin)
{
	if (!irq_driver)
		return -ENOMEM;

	return irq_driver->int_ctrl.unmask(pin);
}

int irq_register_driver(struct irq_driver *driver)
{
	irq_info("register irq driver, type: \"%s\"\r\n", driver->type);

	if (irq_driver)
		return -ENOMEM;
	irq_driver = driver;

	list_add(&driver->list, &__irq_driver_list);

	return 0;
}

EXPORT_SYMBOL(irq_register_isr);
EXPORT_SYMBOL(irq_mask);
EXPORT_SYMBOL(irq_register_driver);
