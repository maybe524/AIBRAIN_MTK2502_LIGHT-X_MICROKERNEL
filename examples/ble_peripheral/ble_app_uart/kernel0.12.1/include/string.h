#pragma once

//#include <stdio.h>
#include "include/types.h"

#define ISHEX(b) (((b) >= 'a' && (b) <= 'f') || ((b) >= 'A' && (b) <= 'F') || ((b) >= '0' && (b) <= '9'))
#define ISDIGIT(x) ((x) >= '0' && (x) <= '9')

//-------------- Standard String APIs ---------------

char *kstrncpy(char *, const char *,__size_t);

__size_t kstrnlen(const char *, __size_t);

__size_t kstrlen(const char *);

int kstrcmp(const char *, const char *);

int kstrncmp(const char *, const char *, __size_t);

int kstrcasecmp (const char *pstr1, const char *pstr2);

char *kstrcpy(char *, const char *);

char *kstrcat(char *, const char *);

char *kstrncat(char *, const char *, __size_t);

int kmemcmp(const void*, const void*, __size_t);

void *kmemcpy(void *, const void*, __size_t);

void *kmemmove(void *, const void*, __size_t);

void *kmemset(void *, int, __size_t);

char *kstrstr(const char *, const char *);

char *kstrcasestr(const char *, const char *);

char *kstrchr(const char *, char);

char *kstrnchr(const char *, char, __size_t);

char *kstrrchr(const char *, int);

char *kstrdup(const char *);

//------------------ Extra String APIs ------------------

int hex_str_to_val(const char *str, unsigned long *val);
int val_to_hex_str(char *str, unsigned long val);

int dec_str_to_long(const char *str, long *val);
int val_to_dec_str(char *str, long val);

int dec_str_to_int(const char *str, int *val);

int hr_str_to_val(const char *str, unsigned long *val);
int val_to_hr_str(unsigned long val, char str[]);

int str_to_val(const char *str, unsigned long *val);

int str_to_ip(__u8 ip_val[], const char *ip_str);
//int ip_to_str(char ip_str[], const __u32 ip);
int str_to_mac(__u8 mac[], const char *str);
