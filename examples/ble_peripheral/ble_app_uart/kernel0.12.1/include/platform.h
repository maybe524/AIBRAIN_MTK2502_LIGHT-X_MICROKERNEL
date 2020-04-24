#include <include/errno.h>
#include <include/device.h>
#include <include/pm.h>

struct platform_id {
	const char *name;
};

struct platform_device {
	struct device dev;
	const char *name;
	void *platform_data;

	int (*init)(struct platform_device *); // to be removed!
};

struct platform_driver {
	int (*probe)(struct platform_device *);
	//int (*remove)(struct platform_device *);
	void (*shutdown)(struct platform_device *);
	int (*suspend)(struct platform_device *, pm_message_t state);
	int (*resume)(struct platform_device *);
	struct driver driver;
	//const struct platform_device_id *id_table;
};

static inline struct resource *platform_get_mem(struct platform_device *pdev, int n)
{
	int i, j;

	for (i = 0, j = 0; i < pdev->dev.res_num; i++)
		if (pdev->dev.resources[i].flag & IORESOURCE_MEM) {
			if (j == n)
				return pdev->dev.resources + i;
			j++;
		}

	return NULL;
}

static inline int platform_get_irq(struct platform_device *pdev, int n)
{
	int i, j;

	for (i = 0, j = 0; i < pdev->dev.res_num; i++)
		if (pdev->dev.resources[i].flag & IORESOURCE_IRQ) {
			if (j == n)
				return pdev->dev.resources[i].start;
			j++;
		}

	return -ENOENT;
}

int platform_device_register(struct platform_device *dev);
int platform_driver_register(struct platform_driver *drv);
