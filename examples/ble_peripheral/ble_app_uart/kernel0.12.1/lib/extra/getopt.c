#include "include/stdio.h"
#include "include/string.h"
#include "include/shell.h"
#include "include/errno.h"

int getopt(char *opt_str, struct command_opts *argv, __size_t opt_cnt)
{
    char *vp, *cp = opt_str;
    int n = 0;

    if (!opt_str || !argv || !opt_cnt)
        return -EINVAL;

    while (kd_true) {
        cp = kstrchr(cp, '-');
        if (!cp) break;

        cp++;
        if ((n + 1) > opt_cnt || IS_STR_END(cp))
            break;

        vp = cp;
        while (!IS_STR_END(vp) && ' ' != *vp && '=' != *vp) vp++;

        if (IS_STR_END(vp) || ' ' == *vp) {
            (argv + n)->opt.str = cp;
            (argv + n)->opt.len = vp - cp;
            (argv + n)->val.str = NULL;
            (argv + n)->val.len = 0;
        }
        else {
            vp++;
            (argv + n)->opt.str = cp;
            (argv + n)->opt.len = (vp - cp) - 1;

            cp = vp;
            while (!IS_STR_END(cp) && ' ' != *cp && '=' != *cp) cp++;

            (argv + n)->val.str = vp;
            (argv + n)->val.len = cp - vp;
        }
        n++;
    }

    return n;
}

__u32 fixopt(__u8 *ostr, __u8 olen)
{
	__u32 fix_result = 0;

	if (olen > sizeof(__u32))
		return 0;
	for (__u8 i = 0; i < olen; i++)
		fix_result |= ostr[i] << ((olen - i - 1) * 8);
	return fix_result;
}

kd_bool eqlopt(char *vstr, int vlen, char *dstr)
{
	__u8 dlen = kstrlen(dstr);

	return (dlen == vlen && !kstrncmp(vstr, dstr, dlen)) ? kd_true : kd_false;
}