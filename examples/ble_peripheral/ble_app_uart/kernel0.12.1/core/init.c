#include "include/stdio.h"
#include "include/init.h"
#include "include/syscalls.h"
#include "include/fs.h"
#include "include/board.h"

static const char banner[] = "\r\n" // CLRSCREEN
			"\r\nWelcome to Kernel system! "
			"\r\nBuilt time: "__DATE__", "__TIME__""
			"\r\nVersion: 1.1.1"
			"\r\nCopyright 37body@longcheer.net\r\n"
			;

const char *fstab_info[][3] =
{
	{"none", "/dev",		"devfs"},
	{"none", "/data",		"none"},
	{"none", "/sys",		"none"},
};

kd_bool system_inited = kd_false;

static init_func_t *tiny_init_call[] =
{
	init_call_level_2,
};

static int system_init(void)
{
	int ret;
	init_func_t init_call = NULL;

	for (int iter = 0; init_call = tiny_init_call[0][iter]; iter++) {
		printk("init call func: <%d>\r\n", iter);
		ret = init_call();
		if (ret < 0) {
			printk("System init %d fail(%d)!!!\r\n", ret);
			// return ret;
		}
	}

	system_inited = kd_true;

	return 0;
}

int populate_rootfs_init(void)
{
	int i, ret;
	const char *device;
	unsigned long flags;

	ret = mount_root(NULL, "ramfs", MS_NODEV);
	if (ret < 0) {
		DPRINT("Fetal error: fail to mount rootfs! (%d)\r\n", ret);
		return ret;
	}

	for (i = 0; i < ARRAY_ELEM_NUM(fstab_info); i++) {
		ret = sys_mkdir(fstab_info[i][1], 0755);
		if (ret < 0) {
			DPRINT("fail to create %s! (%d)\r\n", fstab_info[i][1], ret);
			goto L1;
		}

		flags = 0;
		device = fstab_info[i][0];
		if (!kstrcmp(fstab_info[i][0], "none")) {
			device = NULL;
			flags |= MS_NODEV;
		}
		if (!kstrcmp(fstab_info[i][2], "none"))
			continue;

		ret = sys_mount(device, fstab_info[i][1], fstab_info[i][2], flags);
		if (ret < 0)
			DPRINT("mount %s %s %s failed (%d)\r\n", fstab_info[i][0], fstab_info[i][1], fstab_info[i][2], ret);
	}

	return 0;

L1:
	return ret;
}
module_init(populate_rootfs_init);

static int rc_local(void)
{
	shell("mountfs -dev=/dev/mtdblock0 -poin=/data -type=ltxfs \r\n");
	shell("mountfs \r\n");
	shell("touch -path=/data/local.log \r\n");
	shell("touch -path=/data/history.hex \r\n");
	
	return 0;
}

int start_kernel_system(void *cmd_line)
{
	uart_init();
	DPRINT("%s""Cmdline = %s.\r\n\r\n", banner, !cmd_line ? "N/A" : (char *)cmd_line);

	system_init();
	rc_local();

	return 0;
}
