#pragma once

#include <include/types.h>
#include <include/list.h>
#include <include/sched.h>
#include <include/time.h>

struct __wait_queue_head {
	__u8	id;
	kd_bool lock;
	struct list_head task_list, queue_list;
};

typedef struct __wait_queue_head wait_queue_head_t;

typedef struct __wait_queue wait_queue_t;
typedef int (*wait_queue_func_t)(wait_queue_t *wait, unsigned mode, int flags, void *key);

struct __wait_queue {
	unsigned int flags;
#define WQ_FLAG_EXCLUSIVE	0x01
	void *private;
	wait_queue_func_t func;
	struct list_head task_list;
};

/*
 * Macros for declaration and initialisaton of the datatypes
 */
#define __WAIT_QUEUE_HEAD_INITIALIZER(wid, name) {				\
	.id			= wid,			\
	.lock		= kd_false,		\
	.task_list	= { &(name).task_list, &(name).task_list } }

#define DECLARE_WAIT_QUEUE_HEAD(wid, name) \
	wait_queue_head_t name = __WAIT_QUEUE_HEAD_INITIALIZER(wid, name)

enum WAIT_IDX {
	WAIT_QUEUE_NONE,
	WAIT_QUEUE_UART,
	WAIT_QUEUE_FLASH,
	WAIT_QUEUE_PPS960,
};

#define DEFINE_WAIT_FUNC(name, function)				\
	wait_queue_t name = {						\
		.private	= current,				\
		.func		= function,				\
		.task_list	= {&(name).task_list, &(name).task_list},	\
	}

#define DEFINE_WAIT(name) DEFINE_WAIT_FUNC(name, NULL)

void finish_wait(wait_queue_head_t *q, wait_queue_t *wait);

#define __wait_event_interruptible(wq, condition, ret)			\
do {									\
	DEFINE_WAIT(__wait);						\
									\
	sleep_on(current);						\
	for (;;) {							\
		prepare_to_wait(&wq, &__wait, TASK_INTERRUPTIBLE);	\
		if (condition) {						\
			wake_up(current);	\
			break;						\
		}								\
		if (!signal_pending(current)) {				\
			schedule();					\
			continue;					\
		}							\
		ret = -ERESTARTSYS;					\
		break;							\
	}								\
	finish_wait(&wq, &__wait);					\
} while (0)

#define wait_event_interruptible(wq, condition)				\
do {									\
	int __ret = 0;							\
	if (!(condition))						\
		__wait_event_interruptible(wq, condition, __ret);	\
	__ret;								\
} while (0)


#define __set_current_state(state_value)			\
		do { current->t_state = (state_value); } while (0)


#define __wait_event_timeout(wq, tsk, condition, ret)			\
do {									\
	DEFINE_WAIT(__wait);						\
	time_t ___times_tamp = get_systime();		\
											\
	if (tsk)							\
		sleep_on(tsk);						\
	for (;;) {							\
		prepare_to_wait(&wq, &__wait, TASK_UNINTERRUPTIBLE);	\
		if ((condition) || (ret && (get_systime() - ___times_tamp >= ret))) {						\
			wake_up(tsk);	\
			break;						\
		}							\
		sleep_on(tsk);				\
	}								\
	if (!ret && (condition))					\
		ret = 1;						\
	finish_wait(&wq, &__wait);					\
} while (0)


/**
 * wait_event_timeout - sleep until a condition gets true or a timeout elapses
 * @wq: the waitqueue to wait on
 * @condition: a C expression for the event to wait for
 * @timeout: timeout, in jiffies
 *
 * The process is put to sleep (TASK_UNINTERRUPTIBLE) until the
 * @condition evaluates to true. The @condition is checked each time
 * the waitqueue @wq is woken up.
 *
 * wake_up() has to be called after changing any variable that could
 * change the result of the wait condition.
 *
 * The function returns 0 if the @timeout elapsed, or the remaining
 * jiffies (at least 1) if the @condition evaluated to %true before
 * the @timeout elapsed.
*/
#define wait_event_timeout(wq, tsk, condition, timeout)			\
{									\
	long __ret = timeout;						\
		if (!(condition))						\
			__wait_event_timeout(wq, tsk, (condition), __ret); 	\
	__ret;								\
}

