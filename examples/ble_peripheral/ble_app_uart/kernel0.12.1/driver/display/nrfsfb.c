#include <include/fb.h>
#include <include/io.h>
#include <include/init.h>
#include <include/string.h>
#include <include/malloc.h>
#include <include/platform.h>
#include <include/delay.h>
#include <include/graphic/display.h>
#include <include/task.h>
#include <include/stdio.h>
#include <include/errno.h>
#include <include/sched.h>
#include <include/lock.h>
#include <include/assert.h>

#define CLKSEL_DSS1 8

struct nrfs_fbmem_panel {
	int width, height;
	int bpp;

	// fixme
	unsigned int hfp, hbp, hpw;
	unsigned int vfp, vbp, vpw;
};


struct nrfs_fbmem_info {
	struct nrfs_fbmem_panel *panel;
	struct display *disp;
	
	void *mmio;
};

LIST_HEAD(__nrfs_fbmem_regioninfo_list);
static kd_bool nrfs_fbmem_need_sync = kd_false;

static int nrfs_fbmem_regioninfo_register(struct fb_info *fb, struct fb_regioninfo *info)
{
	__u32 size;
	struct list_head *iter;
	struct nrfs_fbmem_info *nrfb;

#if 0
	if (!ALIGN_CHECK(info->xreg, 8) || \
			!ALIGN_CHECK(info->xreg + info->width, 4))
		return -EALIGN;
#endif

	list_for_each(iter, &__nrfs_fbmem_regioninfo_list) {
		struct fb_regioninfo *region = container_of(iter, struct fb_regioninfo, list);
		if (region == info)
			return -EBUSY;
	}

	nrfb = (struct nrfs_fbmem_info *)fb->par;
	size = info->width * info->height;

	if (info->flags & FB_REGION_FLAGS_USEBASE) {
		info->regionbase = (char *)kmalloc(size, GFP_NRFBMEM);
		if (!info->regionbase)
			return -ENOMEM;
	}
	else if (info->flags & FB_REGION_FLAGS_USEBUFF)
		info->regionbase = NULL;
	else
		assert(0);

	info->fb = fb;
	list_add(&info->list, &__nrfs_fbmem_regioninfo_list);

	return 0;
}

static int nrfs_fbmem_filregion(struct fb_info *fb, struct fb_regioninfo *info)
{
	info->status |= FB_STATUS_REGION_DIRTY;
	return 0;
}

static int nrfs_fbmem_check_var(struct fb_info *fb,
		struct fb_var_screeninfo *var)
{
	struct nrfs_fbmem_info *nrfsfb = fb->par;

	if (var->xres != nrfsfb->panel->width || var->yres != nrfsfb->panel->height)
		return -ENOTSUPP;

	fb->var.xres = var->xres;
	fb->var.yres = var->yres;

	return 0;
}

static int nrfs_fbmem_set_par(struct fb_info *fb)
{
	__u32 val;
	struct fb_var_screeninfo *var = &fb->var;
	// struct fb_fix_screeninfo *fix = &fb->fix;
	struct nrfs_fbmem_info *nrfsfb = fb->par;
	struct nrfs_fbmem_panel *pan;

	pan = nrfsfb->panel;

	return 0;
}

static unsigned char __sched
		nrfs_fbmem_sync_thread(struct task_struct *task, void *data)
{
	int ret;
	struct list_head *iter;
	unsigned char *update_src;

	if (!nrfs_fbmem_need_sync || check_lock(&task->lock))
		return TSLEEP;

	lock(&task->lock);

	list_for_each(iter, &__nrfs_fbmem_regioninfo_list) {
		struct fb_regioninfo *region = container_of(iter, struct fb_regioninfo, list);
		if (region->status & FB_STATUS_REGION_DIRTY) {
			if (region->flags & FB_REGION_FLAGS_USEBASE)
				update_src = (__u8 *)region->regionbase;
			else if (region->flags & FB_REGION_FLAGS_USEBUFF)
				update_src = (__u8 *)region->buff;
			else
				assert(0);

			ret = display_update(update_src, region->xreg, region->yreg, region->width, region->height, region->flags);
			region->status &= ~FB_STATUS_REGION_DIRTY;
		}
	}
	nrfs_fbmem_need_sync = kd_false;

	unlock(&task->lock);
	sleep_on(NULL);

	return TSLEEP;
}
TASK_INIT(CMD_FB_SYNC, nrfs_fbmem_sync_thread, kd_false, NULL, 
		EXEC_TRUE,
		SCHED_PRIORITY_HIGH);

static int nrfs_fbmem_sync(struct fb_info *info)
{
	int ret = 0;
	wait_queue_head_t wq;

	nrfs_fbmem_need_sync = kd_true;
	wake_up(TASK_INFO(nrfs_fbmem_sync_thread));

	wait_event_timeout(wq, NULL, nrfs_fbmem_need_sync == kd_false, Second(10));
	if (nrfs_fbmem_need_sync)
		ret = -ETIMEDOUT;

	return ret;
}

static int nrfs_fbmem_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	int ret;

	switch (cmd) {
		case FB_IOCTRL_REGISTER_REGIONINFO:
			ret = nrfs_fbmem_regioninfo_register(info, (struct fb_regioninfo *)arg);
			break;
		case FB_IOCTRL_FILREGION:
			ret = nrfs_fbmem_filregion(info, (struct fb_regioninfo *)arg);
			break;
		case FB_IOCTRL_SYNC:
			ret = nrfs_fbmem_sync(info);
			break;
		default:
			ret = -ENOTSUPP;
			break;
	}

	return ret;
}

static ssize_t nrfs_fbmem_write(struct fb_info *info, const char __user *buf,
			__size_t count, loff_t *ppos)
{
	return 0;
}

static const struct fb_ops nrfs_fbmem_ops = {
	.fb_check_var	= nrfs_fbmem_check_var,
	.fb_set_par		= nrfs_fbmem_set_par,
	.fb_ioctl	= nrfs_fbmem_ioctl,
	.fb_sync	= nrfs_fbmem_sync,
	.fb_write	= nrfs_fbmem_write,
};

static int nrfs_fbmem_reset(struct fb_info *fb)
{
#define nrfs_TIMEOUT 0x100
	int to;
	__u32 val;
	struct nrfs_fbmem_info *nrfsfb = fb->par;

	return 0;
}

static int __init nrfs_fbmem_probe(struct platform_device *pdev)
{
	int ret;
	struct fb_info *fb;
	struct nrfs_fbmem_info *nrfsfb;
	struct resource *nres;
	struct fb_fix_screeninfo *fix;
	struct fb_var_screeninfo var;
	struct display *disp = get_system_display();

	if (!disp)
		return -ENODEV;

	fb = framebuffer_alloc(sizeof(*nrfsfb));
	// ...

	fix = &fb->fix;
	nrfsfb = fb->par;

	nrfsfb->mmio = VA(nres->start);
	nrfsfb->disp = disp;
	fb->ops = &nrfs_fbmem_ops;

	fb->screenbase = (char *)disp->video_mem_va;
	fb->fix.smem_len = disp->video_info->vmode.height * disp->video_info->vmode.width * 2 / 8;
	ret = nrfs_fbmem_reset(fb);
	// ret ...

	var.xres = nrfsfb->disp->video_mode->width;
	var.yres = nrfsfb->disp->video_mode->height;

	ret = fb->ops->fb_check_var(fb, &var);
	// if ret

	ret = fb->ops->fb_set_par(fb);
	// if ret < 0 ..

	ret = framebuffer_register(fb);
	// if ret < 0 ...
	ret = sched_task_create(TASK_INFO(nrfs_fbmem_sync_thread),
					SCHED_COMMON_TYPE,
					EXEC_TRUE | EXEC_INTERRUPTIBLE);
	return ret;
}

static int nrfs_fbmem_suspend(struct platform_device *dev, pm_message_t state)
{
	display_suspend();
	return 0;
}

static int nrfs_fbmem_resume(struct platform_device *dev)
{
	display_resume();
	return 0;
}

static struct platform_device nrfs_fbmem_device = {
	.name = "nrfs_disp",
};

static struct platform_driver nrfs_fbmem_driver = {
	.driver = {
		.name = "nrfs_disp",
	},
	.probe = nrfs_fbmem_probe,
	.suspend = nrfs_fbmem_suspend,
	.resume = nrfs_fbmem_resume,
};

int __init nrfs_fbmem_init(void)
{
	int ret;

	ret = platform_device_register(&nrfs_fbmem_device);

	return platform_driver_register(&nrfs_fbmem_driver);
}

module_init(nrfs_fbmem_init);

