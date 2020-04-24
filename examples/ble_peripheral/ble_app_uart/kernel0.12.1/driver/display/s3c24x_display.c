#include <io.h>
#include <init.h>
#include <stdio.h>
#include <errno.h>
#include <graphic/display.h>

// to be removed
#define PNRMODE   3
#define BPPMODE   BPP16_TFT
#define FRM565    1

#define s3c24x_lcdc_writel(reg, val) \
	writel(VA(S3C24X0_LCD_BASE + reg), val)

#define s3c24x_lcdc_readl(reg) \
	readl(VA(S3C24X0_LCD_BASE + reg))

// TODO: support configurable pixel format
static int s3c24x_set_vmode(struct display *disp, const struct lcd_vmode *vm)
{
	__u32 dma = disp->video_mem_pa;
	int clk_val;

	writel(VA(S3C24X0_GPCCON), 0xaaaaaaaa);
	writel(VA(S3C24X0_GPDCON), 0xaaaaaaaa);

#if 0
	val = readl(VA(0x4c00000c));
	val |= 0x20;
	writel(VA(0x4c00000c), val);
#endif

	s3c24x_lcdc_writel(LCDSADDR1, dma >> 1);
	s3c24x_lcdc_writel(LCDSADDR2, ((dma >> 1) & 0x1fffff) + \
								vm->width * vm->height);
	s3c24x_lcdc_writel(LCDSADDR3, vm->width);

	clk_val = HCLK_RATE / (vm->pix_clk * 2) - 1;

	s3c24x_lcdc_writel(LCDCON1, clk_val << 8 | PNRMODE << 5 | BPPMODE << 1 | 1);
	s3c24x_lcdc_writel(LCDCON2, (vm->vbp - 1) << 24 | \
								(vm->height - 1) << 14 | \
								(vm->vfp - 1) << 6 | \
								(vm->vpw - 1));
	s3c24x_lcdc_writel(LCDCON3, (vm->hbp - 1) << 19 | \
								(vm->width - 1) << 8 | \
								(vm->hfp - 1));
	s3c24x_lcdc_writel(LCDCON4, vm->hpw - 1);
	s3c24x_lcdc_writel(LCDCON5, FRM565 << 11 | 0 << 10 | 1 << 9 | 1 << 8 | 1 << 3 | 1);

	// s3c24x_lcdc_writel(TCONSEL, 0);

	return 0;
}

static int __init s3c24x_lcdc_init(void)
{
	int ret;
	struct display *disp;

	disp = display_create();
	// if NULL

	writel(VA(S3C24X0_GPCCON), 0xaaaaaaaa);
	writel(VA(S3C24X0_GPDCON), 0xaaaaaaaa);

	disp->set_vmode = s3c24x_set_vmode;

	ret = display_register(disp);
	if (ret < 0)
		goto error;

	return 0;

error:
	display_destroy(disp);
	return ret;
}

module_init(s3c24x_lcdc_init);
