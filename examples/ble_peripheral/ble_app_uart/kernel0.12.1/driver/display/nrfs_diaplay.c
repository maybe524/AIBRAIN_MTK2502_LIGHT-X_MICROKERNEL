#include <include/io.h>
#include <include/init.h>
#include <include/stdio.h>
#include <include/errno.h>
#include <include/assert.h>
#include <include/graphic/display.h>
#include <include/malloc.h>

extern struct lcd_drv plat_lcd_drv;
static struct lcd_info plat_lcd_info = {0};

#include <include/malloc.h>
static LIST_HEAD(nrfbmem_free_region_list);
static __u8 ___src_alloc_nrfbmem[KB(0) + 200] __attribute__ ((aligned (sizeof(long))));
static struct mem_src __kd_alloc_nrfbmem = 
{
	GFP_NRFBMEM,
	"nrFbmem",
	___src_alloc_nrfbmem,
	sizeof(___src_alloc_nrfbmem),
	&nrfbmem_free_region_list,
	NULL,
};

static int nrfs_display_set_vmode(struct display *disp, const struct lcd_vmode *vm)
{
	__u32 fmt, bpp;
	__u32 dma = disp->video_mem_pa;

	// fixme
	switch (disp->pix_fmt) {
	case PIX_RGB24:
	case PIX_RGB32:
		fmt = 0xB;
		bpp = 4;
		break;

	case PIX_RGB16:
		fmt = 0x5;
		bpp = 2;
		break;

	case PIX_RGB15:
		fmt = 0x7;
		bpp = 2;
		break;

	default:
		BUG();
	}

	return 0;
}

static int nrfs_display_mem_init(struct display *disp)
{
	int ret;
	const struct lcd_vmode *vm;
	struct lcd_info info;
	struct mem_src *msrc = &__kd_alloc_nrfbmem;
	__u32 psize;

	disp->video_drv->get_info(&info);
	vm = &info.vmode;

	psize = vm->width * vm->height * info.pix_format / 8;
	if (msrc->size <= psize)
		DPRINT("WARNING: fbsize = %d, psize = %d\r\n", msrc->size, psize);

	ret = memsrc_register(&__kd_alloc_nrfbmem);

	return ret;
}

static int nrfs_display_update(struct display *disp, __u8 *buff,
			__u32 xres, __u32 yres,
			__u32 width, __u32 height, __u32 flags)
{
	if (disp)
		return disp->video_drv->update(buff, xres, yres, width, height, flags);

	return -ENOTSUPP;
}

int __init nrfs_display_init(void)
{
	int ret;
	__u32 val;
	struct display *disp;
	struct lcd_info lcd_info;

	disp = display_create();
	if (!disp) {
		DPRINT("Display create alloc fail!\r\n");
		return -ENOMEM;
	}

	disp->video_drv  = &plat_lcd_drv;
	disp->video_info = &plat_lcd_info;
	disp->set_vmode  = nrfs_display_set_vmode;
	disp->update = nrfs_display_update;

	ret = disp->video_drv->get_info(disp->video_info);
	if (ret < 0)
		goto error;

	ret = display_register(disp);
	if (ret < 0)
		goto error;
	ret = nrfs_display_mem_init(disp);

	return ret;

error:
	display_destroy(disp);
	return ret;
}

module_init(nrfs_display_init);

