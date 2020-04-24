#include <include/fb.h>
#include <include/fs.h>
#include <include/fcntl.h>
#include <include/init.h>
#include <include/errno.h>
#include <include/string.h>
#include <include/malloc.h>
#include <include/graphic/display.h>
#include <include/types.h>
#include <include/stdio.h>

#define MAX_FB   4
#define FB_MAJOR 1

static struct fb_info *g_fb_info[MAX_FB];

static int fb_open(struct file *, struct inode *);
static int fb_close(struct file *);
static ssize_t fb_read(struct file *, void *, __size_t, loff_t *);
static ssize_t fb_write(struct file *, const void *, __size_t, loff_t *);
static int fb_ioctl(struct file *, int, unsigned long);
static loff_t fb_lseek(struct file *, loff_t, int);

const struct file_operations fb_fops = {
	.open  = fb_open,
	.close = fb_close,
	.read  = fb_read,
	.write = fb_write,
	.ioctl = fb_ioctl,
	.lseek = fb_lseek,
};

static int fb_open(struct file *fp, struct inode *inode)
{
	unsigned int minor = iminor(inode);
	struct fb_info *fb;

	if (minor >= MAX_FB)
		return -EINVAL;

	fb = g_fb_info[minor];
	if (!fb)
		return -ENODEV;

	fp->private_data = fb;

	return 0;
}

static int fb_close(struct file *fp)
{
	return 0;
}

static ssize_t fb_read(struct file *fp, void *buff, __size_t size, loff_t *loff)
{
	return -EIO;
}

static ssize_t fb_write(struct file *fp, const void *buff,
		__size_t size, loff_t *loff)
{
	__size_t real_size;
	struct fb_info *fb = fp->private_data;

	// if null

	if (*loff + size > fb->fix.smem_len)
		real_size = fb->fix.smem_len - *loff;
	else
		real_size = size;

	kmemcpy(fb->screenbase + *loff, buff, real_size);
	*loff += real_size;

	return real_size;
}

static int fb_ioctl(struct file *fp, int cmd, unsigned long arg)
{
	int ret;
	struct fb_info *fb = fp->private_data;

	switch (cmd) {
		case FB_IOCTRL_REGISTER_REGIONINFO:
		case FB_IOCTRL_FILREGION:
		case FB_IOCTRL_SYNC:
			if (fb)
				ret = fb->ops->fb_ioctl(fb, cmd, arg);
			break;
		default:
			return -EINVAL;
	}

	return ret;
}

static loff_t fb_lseek(struct file *fp, loff_t loff, int whence)
{
	struct fb_info *fb = fp->private_data;

	switch (whence) {
	case SEEK_SET:
		fp->f_pos = loff;
		break;

	case SEEK_CUR:
		fp->f_pos += loff;
		break;

	case SEEK_END:
		fp->f_pos = fb->fix.smem_len - loff;
		break;

	default:
		return -ENOTSUPP;
	}

	return 0;
}

struct fb_info *framebuffer_alloc(__size_t size)
{
	struct fb_info *fb;
	void *p;

	fb = kmalloc(sizeof(struct fb_info) + size, 0);
	if (!fb)
		printk("Frambuffer alloc memery fail\r\n");
	p = (void *)((__u32)fb + (__u32)sizeof(struct fb_info));

	fb->par = p;

	return fb;
}

int framebuffer_register(struct fb_info *fb)
{
	int ret, i;
	struct fb_fix_screeninfo *fix = &fb->fix;
	char buff[5] = {0};

	for (i = 0; i < MAX_FB; i++)
		if (!g_fb_info[i])
			break;

	if (MAX_FB == i)
		return -EBUSY;

	g_fb_info[i] = fb;

	kmemset(fb->screenbase, 0x0, fix->smem_len);
	ksprintf(buff, "fb%d", i);

	ret = cdev_create(MKDEV(FB_MAJOR, i), buff, i);
	// ...

	return 0;
}

int __init fbmem_init(void)
{
	int ret;

	ret = register_chrdev(FB_MAJOR, &fb_fops, "fb");
	if (ret < 0)
		return ret;

	return 0;
}

subsys_init(fbmem_init);
