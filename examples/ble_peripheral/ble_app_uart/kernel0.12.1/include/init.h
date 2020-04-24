#pragma once

//#include "include/types.h"

#ifdef __GBIOS_VER__
#define __init           __attribute__ ((__section__(".code.init")))
#else
#define __init           // __attribute__((constructor))
#endif

#define __sched	// use for schedule
#define __inline		inline

#define __INIT_DATA__      __attribute__ ((__section__(".data.init")))

#define INIT_CALL_LEVEL(n) __attribute__ ((__section__(".Level" #n ".gbios_init")))

#define __INIT_ARCH__     INIT_CALL_LEVEL(0)
#define __INIT_PLAT__     INIT_CALL_LEVEL(1)
#define __INIT_SUBS__     INIT_CALL_LEVEL(2)
#define __INIT_POSTSUBS__ INIT_CALL_LEVEL(3)
#define __INIT_DRV__      INIT_CALL_LEVEL(4)

// fixme!
#ifdef __GBIOS_VER__
#define arch_init(func) \
	static __USED__ __INIT_ARCH__ init_func_t __initcall_##func = func

#define plat_init(func) \
	static __USED__ __INIT_PLAT__ init_func_t __initcall_##func = func

#define subsys_init(func) \
	static __USED__ __INIT_SUBS__ init_func_t __initcall_##func = func

#define postsubs_init(func) \
		static __USED__ __INIT_POSTSUBS__ init_func_t __initcall_##func = func

#define module_init(func) \
	static __USED__ __INIT_DRV__ init_func_t __initcall_##func = func

#define module_exit(nil)

#define MODULE_DESCRIPTION(str)
#define MODULE_AUTHOR(str)
#define MODULE_LICENSE(str)
#else	/* longcheer light-x define */
#define arch_init(func)
#define plat_init(func)
#define subsys_init(func)
#define postsubs_init(func)

#define module_init(func)	//
#define module_include(func) func
#define module_extern(func) extern int func(void)

#define __INIT_ARCH__
#define __INIT_PLAT__
#define __INIT_SUBS__
#define __INIT_POSTSUBS__
#define __INIT_DRV__

#define MODULE_DESCRIPTION(str)
#define MODULE_AUTHOR(str)
#define MODULE_LICENSE(str)
#endif

typedef int (*init_func_t)(void);
#define INIT_CALL_LEVEL(n) \
			init_func_t init_call_level_##n[]

// fixme
const char* get_func_name(const void *func);
