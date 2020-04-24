#include <include/stdio.h>
#include <include/errno.h>
#include <include/string.h>
#include <include/unistd.h>
#include <include/shell.h>
#include <include/getopt.h>
#include <include/sched.h>
#include <include/gpio.h>
#include <include/types.h>

#define ARGV_NUM	3

enum GPIO2S_OPT {
	GPIO2S_OPT_READ		= 0x01,
	GPIO2S_OPT_WRIET	= 0x02,
	GPIO2S_OPT_MAX,
};

static void usage(void)
{
	IMPORT_CMD(gpio2s);
	DPRINT("%s\r\n", INCLUD_CMD(gpio2s).help);
}

unsigned char gpio2s(struct task_struct *task, void *argc)
{
	int n, opt_cnt;
	struct command_opts argv[ARGV_NUM] = {0};
	int ret;
	int action = 0;
	int pin_num = -1, pin_val = -1;
	char valtmp[5] = {0};

	opt_cnt = getopt((char *)argc, argv, ARGV_NUM);
	if (!opt_cnt)
		goto few_argc;
	for_each_opt(n, opt_cnt) {
		__u8 *ostr = (argv + n)->opt.str, olen = (argv + n)->opt.len;
		__u8 *vstr = (argv + n)->val.str, vlen = (argv + n)->val.len;
		switch (fixopt(ostr, olen)) {
			case 'r':
				action = GPIO2S_OPT_READ;
				break;
			case 'w':
				action = GPIO2S_OPT_WRIET;
				break;
			case 'pin':
				kmemset(valtmp, 0, sizeof(valtmp));
				kmemcpy(valtmp, vstr, vlen > sizeof(valtmp) ? sizeof(valtmp) : vlen);
				ret = str_to_val(valtmp, &pin_num);
				break;
			case 'val':
				kmemset(valtmp, 0, sizeof(valtmp));
				kmemcpy(valtmp, vstr, vlen > sizeof(valtmp) ? sizeof(valtmp) : vlen);
				ret = str_to_val(valtmp, &pin_val);
				break;
			case 'help':
				usage();
			default:
				goto normal_end;
		}
	}

	if (action < GPIO2S_OPT_READ || action >= GPIO2S_OPT_MAX || 
			(pin_num < 0) || 
			(action == GPIO2S_OPT_WRIET && pin_val < 0))
		goto few_argc;

	if (action == GPIO2S_OPT_READ)
		DPRINT("read  %05d, val = %d\r\n", pin_num, gpio_get_val(pin_num));
	else if (action == GPIO2S_OPT_WRIET) {
		gpio_set_dir(pin_num, GPIO_DIR_OUT, 0);
		gpio_set_out(pin_num, pin_val ? 1 : 0);
		DPRINT("write %05d, val = %d\r\n", pin_num, pin_val);
	}

normal_end:
	return 0;

few_argc:
	return 0;
}

EXPORT_CMD(gpio2s,
		""
	);
