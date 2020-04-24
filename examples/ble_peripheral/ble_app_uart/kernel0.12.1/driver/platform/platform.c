#include <include/init.h>
#include <include/string.h>
#include <include/platform.h>

static int match(struct device *dev, struct driver *drv);

static struct bus_type platform_bus_type = {
	.name     = "platform",
	.match    = match,
	.dev_list = HEAD_INIT(platform_bus_type.dev_list),
	.drv_list = HEAD_INIT(platform_bus_type.drv_list),
};

static int match(struct device *dev, struct driver *drv)
{
	struct platform_device *pdev;
	// struct platform_driver *pdrv;

	pdev = container_of(dev, struct platform_device, dev);
	// pdrv = container_of(drv, struct platform_driver, drv);

	return !kstrcmp(pdev->name, drv->name);
}

static int platform_driver_probe(struct device *dev)
{
	struct platform_device *pdev;
	struct platform_driver *pdrv;

	pdev = container_of(dev, struct platform_device, dev);
	pdrv = container_of(dev->drv, struct platform_driver, driver);

	return pdrv->probe(pdev);
}

int platform_device_register(struct platform_device *pdev)
{
	int ret;

	pdev->dev.bus = &platform_bus_type;

	if (pdev->init) {
		ret = pdev->init(pdev);
		if (ret < 0)
			return ret;
	}

	return device_register(&pdev->dev);
}

int platform_driver_register(struct platform_driver *pdrv)
{
	pdrv->driver.bus = &platform_bus_type;
	pdrv->driver.probe = platform_driver_probe;
	INIT_LIST_HEAD(&pdrv->driver.dev_list);

	return driver_register(&pdrv->driver);
}

static int __init platform_init(void)
{
	return 0;
}

subsys_init(platform_init);
