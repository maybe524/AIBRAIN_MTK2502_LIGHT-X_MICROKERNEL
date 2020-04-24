#include <include/list.h>
#include <include/device.h>
#include <include/platform.h>
#include <include/types.h>
#include <include/pm.h>

LIST_HEAD(dpm_list);

static inline struct device *to_device(struct list_head *entry)
{
	return container_of(entry, struct device, power.entry);
}

/**
 * device_pm_add - Add a device to the PM core's list of active devices.
 * @dev: Device to add to the list.
 */
void device_pm_add(struct device *dev)
{
	list_add_tail(&dev->power.entry, &dpm_list);
}

/**
 * dpm_suspend - Execute "suspend" callbacks for all non-sysdev devices.
 * @state: PM transition of the system being carried out.
 */
static int dpm_suspend(pm_message_t state)
{
	int error = 0;
	struct list_head *iter;

	list_for_each(iter, &dpm_list) {
		struct device *dev = to_device(iter);
		struct platform_driver *pdrv = container_of(dev->drv, struct platform_driver, driver);
		if (!pdrv->suspend)
			continue;

		error = pdrv->suspend(NULL, state);
	}

	return error;
}

static void dpm_resume(pm_message_t state)
{
	int error = 0;
	struct list_head *iter;

	list_for_each(iter, &dpm_list) {
		struct device *dev = to_device(iter);
		struct platform_driver *pdrv = container_of(dev->drv, struct platform_driver, driver);
		if (!pdrv->resume)
			continue;

		error = pdrv->resume(NULL);
	}

	return;
}

int enter_suspend(void)
{
	pm_message_t state;

	state.event = PM_EVENT_SUSPEND;

	return dpm_suspend(state);
}

int enter_resume(void)
{
	pm_message_t state;

	state.event = PM_EVENT_RESUME;

	dpm_resume(state);
}

