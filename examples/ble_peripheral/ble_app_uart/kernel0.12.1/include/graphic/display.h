#pragma once

#include <types.h>

typedef enum {
	PIX_RGB15,
	PIX_RGB16,
	PIX_RGB24,
	PIX_RGB32,
	PIX_BIT2,
	PIX_BIT1,
	PIX_MAX,
} pix_fmt_t;

struct lcd_vmode {
	const char *model;
	int pix_clk;
	int width, height;
	int hfp, hbp, hpw;
	int vfp, vbp, vpw;
};

struct lcd_info {
	const char *model;
	pix_fmt_t pix_format;

	struct lcd_vmode vmode;
};

struct lcd_drv {
	const char *name;
	int (*get_info)(struct lcd_info *info);

	int (*init)(void);
	int (*suspend)(void);
	int (*resume)(void);

	int (*update)(void *buff, __u32 x, __u32 y, __u32 width, __u32 height, __u32 flags);
};


const struct lcd_vmode *lcd_get_vmode_by_id(int lcd_id);
const struct lcd_vmode *lcd_get_vmode_by_name(const char *model);
void *video_mem_alloc(unsigned long *phy_addr, const struct lcd_vmode *vm, pix_fmt_t pix_fmt);

//
struct display {
	void *mmio;
	void *video_mem_va;
	__u32 video_mem_pa;

	pix_fmt_t pix_fmt;
	struct lcd_vmode *video_mode;
	struct lcd_drv *video_drv;
	struct lcd_info *video_info;

	struct fb_info *fb;
	void *private_data;

	int (*set_vmode)(struct display *, const struct lcd_vmode *);
	int (*update)(struct display *disp, void *buff,
			__u32 xres, __u32 yres, __u32 width, __u32 height, __u32 flags);
};

struct display* display_create(void);
void display_destroy(struct display *disp);
int display_register(struct display* disp);
struct display* get_system_display(void);
