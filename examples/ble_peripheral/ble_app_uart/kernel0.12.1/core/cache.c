#include "include/stdio.h"
#include "include/errno.h"
#include "include/syscalls.h"
#include "include/shell.h"
#include "include/types.h"
#include "include/string.h"
#include "include/sched.h"
#include "include/task.h"
#include "include/cache.h"
#include "include/malloc.h"

//static struct cache_info cache_infos[CACHE_MAX] = {0};


void *cache_alloc(__size_t size)
{
	void *p;

	//p = kmalloc(size, GFP_CACHE);
	if (p)
		kmemset(p, 0, size);

	return p;
}

unsigned char cache_handler(struct task_struct *task, void *argc)
{
	int cid;
	struct cache_opts *cop = (struct cache_opts *)argc;
#if 0
TASK_GEN_STEP_ENTRY(0) {
		if (!cop)
			return -EIO;
		switch (cop->co_cmd) {
			case CACHE_REQUEST:
				for (cid = 0; cid < CACHE_MAX; cid++) {
					if (!cop->ci_file) {
						TASK_SET_STEP(1);
						break;
					}
					if (cid == CACHE_MAX - 1) {}	// check level
				}
			break;
		}
	} /* TASK_GEN_STEP_ENTRY 0 END */

TASK_GEN_STEP_ENTRY(1) {
		cop->ci_file = (struct cache_opts *)kzalloc(sizeof(struct cache_opts), GFP_NORMAL);
		if (!cop->ci_file)
			return -EIO;
	} /* TASK_GEN_STEP_ENTRY 1 END */
#endif
}

EXPORT_CMD(cache_handler,
		""
	);
