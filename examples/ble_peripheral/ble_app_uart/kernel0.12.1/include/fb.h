#pragma once

#include <include/types.h>
#include <include/fs.h>

struct fb_var_screeninfo {
	int xres, yres;
};

struct fb_fix_screeninfo {
	unsigned long smem_start;
	__size_t smem_len;
};

#define FB_STATUS_REGION_DIRTY		(1 << 0)

#define FB_REGION_FLAGS_USEBUFF		(1 << 0)
#define FB_REGION_FLAGS_USEBASE		(1 << 1)
#define FB_REGION_FLAGS_UPDATEFULL	(1 << 2)

struct fb_regioninfo {
	unsigned int xreg, yreg, height, width;

	char *regionbase;
	unsigned char status;

	void *buff;
	__size_t len;
	struct fb_info *fb;
	unsigned char flags;

	struct list_head list;
};

#define FB_REGIONINFO_INIT(id, x, y, w, h) \
	static struct fb_regioninfo id##fb_regioninfo = {	\
			.xreg = x,	\
			.yreg = y,	\
			.height = h,	\
			.width	= w,		\
			.status = 0,	\
			.list = {&id##fb_regioninfo.list, &id##fb_regioninfo.list},	\
		};
#define FB_REGIONINFO_CONTEXT(id)	id##fb_regioninfo

struct fb_info;

struct fb_ops {
	int (*fb_check_var)(struct fb_info *, struct fb_var_screeninfo *);
	int (*fb_set_par)(struct fb_info *);

	/* For framebuffers with strange non linear layouts or that do not
	 * work with normal memory mapped access
	 */
	ssize_t (*fb_read)(struct fb_info *info, char __user *buf,
			   __size_t count, loff_t *ppos);
	ssize_t (*fb_write)(struct fb_info *info, const char __user *buf,
				__size_t count, loff_t *ppos);

	/* wait for blit idle, optional */
	int (*fb_sync)(struct fb_info *info);

	/* perform fb specific ioctl (optional) */
	int (*fb_ioctl)(struct fb_info *info, unsigned int cmd,
			unsigned long arg);

};

struct fb_info {
	struct fb_fix_screeninfo fix;
	struct fb_var_screeninfo var;
	const struct fb_ops *ops;
	char *screenbase;
	void *par;
};

struct fb_info *framebuffer_alloc(__size_t);
// framebuffer_free()

int framebuffer_register(struct fb_info *);
// framebuffer_unregister

#define FB_IOCTRL_REGISTER_REGIONINFO		0x01
#define FB_IOCTRL_FILREGION					0x02
#define FB_IOCTRL_SYNC						0x03