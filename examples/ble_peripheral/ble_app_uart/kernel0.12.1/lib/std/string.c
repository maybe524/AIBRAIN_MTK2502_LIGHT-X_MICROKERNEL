#include "include/types.h"
#include "include/malloc.h"
#include "include/string.h"

__size_t kstrlen(const char *src)
{
	const char *iter;

	for (iter = src; *iter; iter++);

	return iter - src;
}

__size_t kstrnlen(const char *src, __size_t count)
{
	const char *iter;

	for (iter = src; *iter && count; iter++, count--);

	return iter - src;
}

char *kstrcpy(char *dst, const char *src)
{
	char *iter = dst;

	while ((*iter = *src) != '\0') {
		iter++;
		src++;
	}

	return dst;
}

char *kstrncpy(char *dst, const char *src, __size_t count)
{
	char *iter = dst;
	__size_t n = 0;

	while ((n < count) && (*iter = *src)) {
		iter++;
		src++;
		n++;
	}

	while (n < count) {
		*iter++ = '\0';
		n++;
	}

	return dst;
}

int kstrcmp(const char *pstr1, const char *pstr2)
{
	while (*pstr1 == *pstr2) {
		if ('\0' == *pstr1)
			return 0;

		pstr1++;
		pstr2++;
	}

	return *pstr1 - *pstr2;
}

int kstrncmp(const char *pstr1, const char *pstr2, __size_t count)
{
	__size_t n = 1;

	while (*pstr1 == *pstr2) {
		if (('\0' == *pstr1) || (n == count))
			return 0;

		pstr1++;
		pstr2++;
		n++;
	}

	return *pstr1 - *pstr2;
}

// fixme!!
int kstrcasecmp (const char *pstr1, const char *pstr2)
{
	while (*pstr1 == *pstr2) {
		if ('\0' == *pstr1)
			return 0;

		pstr1++;
		pstr2++;
	}

	return *pstr1 - *pstr2;
}

char *kstrcat(char *dst, const char *src)
{
	char *iter;

	for (iter = dst; *iter; iter++);

	while ((*iter = *src) != '\0') {
		iter++;
		src++;
	}

	return dst;
}

char *kstrncat(char *dst, const char *src, __size_t count)
{
	char *iter;
	__size_t n = 0;

	for (iter = dst; *iter; iter++);

	while (n < count && (*iter = *src)) {
		iter++;
		src++;
		n++;
	}

	while (n < count) {
		*iter = '\0';
		iter++;
		n++;
	}

	return dst;
}

// why not use the KMP algo?
char *kstrstr(const char *haystack, const char *needle)
{
	const char *p, *q;

	p = haystack;
	q = needle;
	while (*p) {
		int i = 0;
		while (q[i] && q[i] == p[i]) i++;
		if (q[i] == '\0')
			return (char *)p;
		p++;
	}

	return NULL;
}

// fixme
char *kstrcasestr(const char *haystack, const char *needle)
{
	return NULL;
}

char *kstrchr(const char *src, char c)
{
	const char *iter;

	for (iter = src; *iter; iter++) {
		if (*iter == c)
			return (char *)iter;
	}

	return NULL;
}

char *kstrnchr(const char *src, char c, __size_t count)
{
	const char *iter;
    int n;

	for (iter = src, n = 0; \
            n < count && *iter; n++, iter++) {
		if (*iter == c)
			return (char *)iter;
	}

	return NULL;
}

char *kstrrchr(const char *src, int c)
{
	const char *iter;

	for (iter = src; *iter; iter++);

	while (iter > src) {
		iter--;
		if (*iter == c)
			return (char *)iter;
	}

	return NULL;
}

char *kstrdup(const char *s)
{
	char *dst;
	__size_t size = strlen(s) + 1;

	dst = (char *)kmalloc(size, GFP_NORMAL);
	if (dst)
		kstrcpy(dst, s);

	return dst;
}

void *kmemcpy(void *dst, const void *src, __size_t count)
{
	__u8 *d;
	const __u8 *s;

	d = dst;
	s = src;

	while (count > 0) {
		*d++ = *s++;
		count--;
	}

	return dst;
}

void *kmemmove(void *dst, const void *src, __size_t count)
{
	__u8 *d;
	const __u8 *s;

	if (dst < src) {
		d = dst;
		s = src;

		while (count > 0) {
			*d++ = *s++;
			count--;
		}
	} else {
		d = (__u8 *)((__u8)dst + count);
		s = (__u8 *)((__u8)src + count);

		while (count > 0) {
			*--d = *--s;
			count--;
		}
	}

	return dst;
}

void *kmemset(void *src, int c, __size_t count)
{
	char *s = src;

	while (count) {
		*s = c;

		s++;
		count--;
	}

	return src;
}

int kmemcmp(const void *dst, const void *src, __size_t count)
{
	const __u8 *s, *d;

	d = dst;
	s = src;

	while (count > 0) {
		if (*d != *s)
			return *d - *s;

		s++;
		d++;

		count--;
	}

	return 0;
}

int kstrcomb(char *dest, __size_t len, const char *str1, const char *str2)
{
	__size_t i = 0, l = kstrlen(str1);
	__u8 *d = dest;

	while (i < len) {
		if (*str1)
			*(d + i) = *str1++;
		if (*str2)
			*(d + l + i) = *str2++;
		i++;
		if (!*str1 && !*str2)
			break;
	}
	*(d + i + l) = '\0';

	return i;
}
