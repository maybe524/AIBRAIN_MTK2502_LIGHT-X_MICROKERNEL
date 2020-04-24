#include "include/stdio.h"
#include "include/string.h"
#include "include/shell.h"

int getcmd(char *opt_str, struct command_opts *argv)
{
	char *vp, *cp = opt_str;
    int n = 0;

    while (true) {
        cp = u_strchr(cp, '-') + 1;
        if (NULL == cp || \
                ((n + 1) > ARRAY_ELEM_NUM(argv)) || \
                ('\r' == *cp || '\n' == *cp || '\0' == *cp))
            break;

        vp = u_strchr(cp, '=') + 1;
        (argv + n)->opt.str = cp;
        (argv + n)->opt.len = (vp - cp) - 1;

        cp = vp;
        while (true) {
            if ('\r' == *cp || '\n' == *cp || '\0' == *cp || ' ' == *cp)
                break;
            cp++;
        }
        (argv + n)->val.str = vp;
        (argv + n)->val.len = cp - vp;

        n++;
    }

	return n;
}
