#pragma once

#include "include/shell.h"

#define for_each_opt(e, a)	for ((e) = 0; (e) < (a); (e)++)

kd_bool eqlopt(char *vstr, int vlen, char *dstr);
__u32 fixopt(__u8 *ostr, __u8 olen);
int getopt(char *, struct command_opts *, __size_t);
