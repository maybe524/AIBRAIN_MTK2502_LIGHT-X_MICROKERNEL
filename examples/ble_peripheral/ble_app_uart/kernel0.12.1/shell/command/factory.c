#include "include/stdio.h"
#include "include/errno.h"
#include "include/string.h"
#include "include/unistd.h"
#include "include/fcntl.h"
#include "include/dirent.h"
#include "include/shell.h"
#include "include/getopt.h"
#include "include/dirent.h"
#include "include/sched.h"
#include "include/fs.h"
#include <include/wait.h>
#include <include/lock.h>

#define ARGV_NUM		4
#define FACTORY_ITEM_CNT		7

static struct factory_info {
	kd_bool lock;
};

int factory_items[FACTORY_ITEM_CNT] = {-1, -1, -1, -1, -1, -1, -1};
struct factory_info factory_infos = {0};

int mesure_start_up(const char *idex);

static void usage(void)
{
	IMPORT_CMD(factory);
	DPRINT("%s\r\n", INCLUD_CMD(factory).help);
}

unsigned char factory(struct task_struct *task, void *argc)
{
	int ret;
	int n, opt_cnt;
	struct command_opts argv[ARGV_NUM] = {0};

	if (check_lock(&factory_infos.lock))
		return 0;
	lock(&factory_infos.lock);

	opt_cnt = getopt((char *)argc, argv, ARGV_NUM);
	if (!opt_cnt)
		goto normal_end;
	for_each_opt(n, opt_cnt) {
		__u8 *ostr = (argv + n)->opt.str, olen = (argv + n)->opt.len;
		__u8 *vstr = (argv + n)->val.str, vlen = (argv + n)->val.len;
		switch (fixopt(ostr, olen)) {
			case 'gray':
				int fd;
				kd_bool is_on;

				if (eqlopt(vstr, vlen, "on"))
					is_on = kd_true;
				else if (eqlopt(vstr, vlen, "off"))
					is_on = kd_false;
				else
					goto unknown_opt;
				fd = open("/dev/gray", O_WRONLY);
				ioctl(fd, 0x01, is_on);
				close(fd);
				break;
			case 'test':
				wait_queue_head_t wait;
				int item;

				mt2502_power_set(1);
				for (item = 0; item < FACTORY_ITEM_CNT; item++) {
					wait_event_timeout(wait, task, factory_items[item] >= 0, Second(20));
					uc8151_fillregion_middle(item, factory_items[item] >= 0 ? factory_items[item] : 0);
				}
				break;
			case 'bhrr':
				mesure_start_up("bhrr");
				break;
			case 'tird':
				mesure_start_up("tird");
				break;
			case 'gver':
				mesure_start_up("gver");
				break;
			case 'temp':
				pnm723t_read_adc();
				break;
			case 'nrfb':
				break;
			case 'vibr':
				drv2605_factory_test();
				break;
			case 'spnd':
				enter_suspend();
				break;
			case 'rsme':
				enter_resume();
				break;
			case '2502':
				mt2502_power_set(1);
				break;
			case 'bq25':
				bq25120_dump_reg();
				break;
			case 'bsnd':
				__u8 teset[20];
				for (int i = 0; i < 100; i++) {
					ret = ble_send_packet(teset, 20, Second(10));
					printk("ret=0x%x\r\n", ret);
				}
				break;
			case 'help':
				usage();
				goto normal_end;
			default:
				goto unknown_opt;
				break;
		}
	}

	unlock(&factory_infos.lock);

normal_end:
	return 0;

unknown_opt:
	DPRINT("invalid option -- %c\r\n", (argv + n)->opt.str[0]);
	return -EINVAL;
}

EXPORT_CMD(factory,
		#include "factory.hlp"
	);
