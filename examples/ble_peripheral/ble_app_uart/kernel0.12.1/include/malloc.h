#pragma once

#include "include/types.h"
#include "include/list.h"

typedef enum {
	GFP_NORMAL = 0,
	GFP_NRFBMEM,
	GFP_MAX,
} gfp_t;

struct mem_src {
	unsigned char gfp_id;
	char *name;
	void *src;
	unsigned int size;
	struct list_head *free_region_list;
	struct list_head src_list;
};


void __capi *kmalloc(__size_t size, gfp_t flags);

void __capi kfree(void *p);

void __capi *kzalloc(__size_t, gfp_t flags);

void __capi *u_dma_alloc_coherent(__size_t size, unsigned long *pa);

struct __capi list_head *u_get_heap_head_list(void);

int memsrc_register(struct mem_src *s);

#define SAFE_FREE(p) \
	do {  \
		free(p); \
		p = NULL; \
	} while (0)


#define vmalloc(s) malloc(s)
#define vfree(p) free(p)
