#include "include/list.h"
#include "include/init.h"
#include "include/errno.h"
#include "include/malloc.h"
#include "include/assert.h"
#include "include/string.h"
#include "include/g-bios.h"
#include "include/stdio.h"
#include "include/arm/cpu.h"
#include "include/types.h"

#define SHIFT_GFP		0	/* Memery Source */
#define SHIFT_FREE		2
#define SHIFT_RESERVE	3

#define GFP_NEED_BITS	0x03		/* Memery Source take 2bit */
#define FREE_NEED_BITS	0x01	/* Free Flag take 1bit */

#define get_info(flag, info)		(((info) >> SHIFT_##flag) & flag##_NEED_BITS)
#define set_one_info(flag, info, val)	((info) = (((info) & ~(flag##_NEED_BITS)) | ((val) << (SHIFT_##flag))))
#define set_all_info(info, val)		((info) = (val))
#define INFO(flag, val)		((val) << (SHIFT_##flag))
#define update_info(flag, info, val)	((((info) & ~(flag##_NEED_BITS)) | ((val) << (SHIFT_##flag))))

#define GAP_SIZE			(sizeof(__u16) * 3 + sizeof(__u8) * 2)
#define LIST_NODE_SIZE			WORD_ALIGN_UP(sizeof(struct list_head))
#define LIST_NODE_ALIGN(size)	(((size) + LIST_NODE_SIZE - 1) & ~(LIST_NODE_SIZE - 1))
#define MIN_HEAP_LEN		1024
#define GET_SIZE(region)	((region)->curr_size & ~(GAP_SIZE - 1))

#define M_NOTHING	0x00
#define M_CURR_INFO	0x01
#define M_PREV_INFO	0x02


struct mem_region {
	/* Memery size is never more than 64K, so short int is enough.
	 */
	__u16 prev_size;
	__u16 curr_size;

	/* Memery region information include some follow:
	 *  1) 0~1 bit. memery alloc for some situation, include normal, supper block cache
	 *  2) 2~7 bit. reserve
	 */
	__u8 prev_info;
	__u8 curr_info;

	__u16 reserve;	/* Reserve use for more information */

	struct list_head ln_mem_region;
};

static LIST_HEAD(mem_src_list);

static LIST_HEAD(normal_free_region_list);
static __u8 ___src_alloc_normal[KB(7)] __attribute__ ((aligned (sizeof(long))));
static struct mem_src __kd_alloc_normal = 
{
	GFP_NORMAL,
	"Normal",
	___src_alloc_normal,
	sizeof(___src_alloc_normal),
	&normal_free_region_list,
	NULL,
};

int memsrc_register(struct mem_src *s)
{
	struct mem_region *head, *tail;

	if (!s || !s->src)
		return -EIO;

	kmemset(s->src, 0, s->size);

	head = (struct mem_region *)(WORD_ALIGN_UP((unsigned long)s->src));
	tail = (struct mem_region *)((WORD_ALIGN_DOWN((unsigned long)s->src + s->size)) - GAP_SIZE);

	DPRINT("Register memery source: " \
			"\"%s\", 0x%08x--0x%08x, size = 0x%08x\r\n", \
			s->name, head, tail, (unsigned long)tail - (unsigned long)head);

	head->prev_size = 0;
	head->curr_size = (unsigned long)tail - (unsigned long)head - GAP_SIZE;
	set_all_info(head->prev_info, INFO(FREE, 1) | INFO(GFP, s->gfp_id));

	tail->prev_size = head->curr_size;
	tail->curr_size = 0;
	set_all_info(tail->curr_info, INFO(FREE, 1) | INFO(GFP, s->gfp_id));

	list_add_tail(&head->ln_mem_region, s->free_region_list);
	list_add_tail(&s->src_list, &mem_src_list);

	return 0;
}

struct mem_src *memsrc_get(gfp_t flags)
{
	struct list_head *iter;

	list_for_each(iter, &mem_src_list) {
		struct mem_src *s = container_of(iter, struct mem_src, src_list);
		if (s->gfp_id == flags)
			return s;
	}

	assert(0);

	return NULL;
}

static __inline struct mem_region *
get_successor(struct mem_region *region)
{
	return (struct mem_region *)((__u8 *)region + GAP_SIZE + GET_SIZE(region));
}

static __inline struct mem_region *
get_predeccessor(struct mem_region *region)
{
	return (struct mem_region *)((__u8 *)region - (region->prev_size & ~(WORD_SIZE - 1)) - GAP_SIZE);
}

static void __inline 
region_set(struct mem_region *curr,
			__size_t size, __u16 info, __u8 modify_info)
{
	struct mem_region *next;

	curr->curr_size = size;
	if (modify_info & M_CURR_INFO)
		curr->curr_info = info;
	else if (modify_info & M_PREV_INFO)
		curr->prev_info = info;

	next = get_successor(curr);
	next->prev_size = size;
	if (modify_info & M_CURR_INFO)
		next->prev_info = info;

	return;
}

void *__real_do_kmalloc(struct mem_src *s, __size_t size, gfp_t flags)
{
	void *p = NULL;
	struct list_head *iter, *choice_list;
	__size_t alloc_size, reset_size;
	struct mem_region *curr, *next;
	unsigned char __UNUSED__ psr;

	lock_irq_psr(psr);

	alloc_size = LIST_NODE_ALIGN(size);
	list_for_each(iter, s->free_region_list) {
		curr = container_of(iter, struct mem_region, ln_mem_region);
		if (curr->curr_size >= alloc_size)
			goto do_alloc;
	}

	unlock_irq_psr(psr);

	return NULL;

do_alloc:
	list_del(iter);
	reset_size = curr->curr_size - alloc_size;

	if (reset_size < sizeof(struct mem_region))
		region_set(curr, curr->curr_size, \
				INFO(GFP, flags) | INFO(FREE, 1), \
				M_CURR_INFO);
	else {
		region_set(curr, alloc_size, INFO(GFP, flags) | INFO(FREE, 1), M_CURR_INFO);

		next = get_successor(curr);
		region_set(next, reset_size - GAP_SIZE, 0, M_NOTHING);
		list_add_tail(&next->ln_mem_region, s->free_region_list);
	}

	p = &curr->ln_mem_region;

	unlock_irq_psr(psr);

	return p;
}

void __real_do_kfree(void *p)
{
	struct mem_region *curr, *next;
	unsigned char __UNUSED__ psr;
	struct list_head *choice_list = NULL;
	struct mem_src *s;

	lock_irq_psr(psr);

	curr = (struct mem_region *)((unsigned long)p - GAP_SIZE);
	next = get_successor(curr);
	s = memsrc_get(get_info(GFP, curr->curr_info));
	if (p < s->src || p > (void *)((__u32)s->src + s->size))
		goto L1;

	if (!get_info(FREE, next->curr_info)) {
		region_set(curr, curr->curr_size + next->curr_size + GAP_SIZE, curr->curr_info, M_CURR_INFO);
		list_del(&next->ln_mem_region);
	}
	else
		region_set(curr, curr->curr_size, curr->curr_info, M_CURR_INFO);

	if (!get_info(FREE, curr->prev_info)) {
		struct mem_region *prev;

		prev = get_predeccessor(curr);
		region_set(prev, prev->curr_size + curr->curr_size + GAP_SIZE, prev->curr_info, M_CURR_INFO);
	}
	else
		list_add_tail(&curr->ln_mem_region, s->free_region_list);

L1:
	unlock_irq_psr(psr);
}

void kfree(void *p)
{
	if (!p)
		return;

	__real_do_kfree(p);
}

void *kmalloc(__size_t size, gfp_t flags)
{
	struct mem_src *s = NULL;

	if (flags >= GFP_MAX)
		return NULL;

	s = memsrc_get(flags);
	if (!s)
		return NULL;

	if (size >= s->size)
		return NULL;

	return __real_do_kmalloc(s, size, flags);
}

void *kzalloc(__size_t size, gfp_t flags)
{
	void *p;

	p = kmalloc(size, flags);
	if (p)
		kmemset(p, 0, size);

	return p;
}

void *dma_alloc_coherent(__size_t size, unsigned long *pa)
{
	void *va;

	va = kmalloc(size, GFP_NORMAL);
	if (!va)
		return NULL;

	*pa = (unsigned long)va;

	return va;
}

struct list_head *tiny_get_heap_head_list(void)
{
	return &normal_free_region_list;
}

int __init mem_normal_init(void)
{
	int ret;

	ret = memsrc_register(&__kd_alloc_normal);
	
	return ret;
}
module_init(mem_normal_init);