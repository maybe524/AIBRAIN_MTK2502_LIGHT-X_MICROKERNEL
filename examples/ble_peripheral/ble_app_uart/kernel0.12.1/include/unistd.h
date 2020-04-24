#pragma once

#include "include/getopt.h"

#define PATH_MAX   256


int chcdir(const char *path);

char *getcwd(char *buff, __size_t size);

char *get_current_dir_name(void);

int mkdir(const char *name, unsigned int mode);
