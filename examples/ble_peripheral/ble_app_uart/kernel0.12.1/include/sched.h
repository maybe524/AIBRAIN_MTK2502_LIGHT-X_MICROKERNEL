#pragma once
#include "include/types.h"
#include "include/list.h"

#define SCHED_COMMON_TYPE		0x00
#define SCHED_NRFS_TYPE			0x01

#define TSTOP		0x00
#define TRUN		0x01
#define TAGAIN		0x02		/* Task Sched will be run it again */
#define TSLEEP		0x03		/* Task Sched will enter next task */
#define TDEAD		0x04
#define TIDLE		0x05
#define TCANCEL		0x06
#define TWAIT		0x07		/* Task waiting resource before */

#define SCHED_PRIORITY_HIGH	0x00
#define SCHED_PRIORITY_MIDE	0x05
#define SCHED_PRIORITY_LOW	0x09

#define TASK_RUNNING			0
#define TASK_INTERRUPTIBLE		1
#define TASK_UNINTERRUPTIBLE	2
#define TASK_ZOMBIE				3
#define TASK_STOPPED			4

struct task_sched;

struct task_struct {
	__u8 		t_id;
	__u8		t_uid;
	kd_bool		t_isdynamic;
	__u16 		t_param;
	__u16 		t_execfg;		// bit_0: exec; bit_1: trigger; bit_2: cancel
	__u8 		t_state;
	__u8 		t_step;
	__u32 		t_stime;
	__u32 		t_stime_temp;
	kd_bool		t_handle_res;
	__u8		t_trycnt;
	void		*data;

	kd_bool		lock;

	//struct		files_struct *files;
	__u32 		(*task_handler)(struct task_struct *task, void *data);
	struct list_head	task_list, wait_list, msg_list;
	struct task_sched	*scheder;

	__u8		t_priority;
};

#include <include/list.h>
struct task_sched {
	const __u8	type;

	int		(*sched)(struct task_struct **);
	int		(*sched_put)(struct task_struct *);
	int		(*sched_set)(struct task_struct *, __u16);

	struct list_head	list;
};

#define TASK_SCHED_INIT(sched, sched_put, sched_set)	\
			static struct task_sched	kd_##sched = {	\
					.sched = sched,	\
					.sched_put = sched_put, \
					.sched_set = sched_set	\
			};

#define TASK_SCHED(sched)	(&kd_##sched)


typedef enum {
	EXEC_FALSE				= 0,
	EXEC_TRUE				= (1 << 0),
	EXEC_TRUE_H2B			= (1 << 1),
	EXEC_TRUE_TRIGGER		= (1 << 2),
	EXEC_TRUE_KEEP			= (1 << 3),
	EXEC_CANCEL_H2B			= (1 << 4),
	EXEC_CANCEL_TRIGGER		= (1 << 5),
	EXEC_CANCEL_FORCE		= (1 << 6),
	EXEC_TRUE_T_KEY			= (1 << 7),
	EXEC_CANCEL_T_KEY		= (1 << 8),
	EXEC_INTERRUPTIBLE		= (1 << 9),
} task_exec_cfg;

#define TASK_GEN_STEP_ENTRY(sstep)		if (!(task) || ((task) && (task->t_step) == (sstep)))
#define TASK_NEXT_STEP()				if ((task)) task->t_step++
#define TASK_SET_STEP(sstep)			if ((task)) task->t_step = (sstep)
#define TASK_STEP(sstep)				(sstep)

#define task_check_exec(task_struct_p)	(((task_struct_p)->t_execfg) & (EXEC_TRUE))
#define task_check_msgl(task_struct_p)	(((task_struct_p)->msg_list.next != &(task_struct_p)->msg_list))
#define sleep_interruptible(task_struct_p) (((task_struct_p)->t_execfg) & (EXEC_INTERRUPTIBLE))
#define likely_dynamic(task_struct_p)		(task_struct_p->t_isdynamic)

#define TASK_INFO(func) (&task_##func)
#define TASK_INIT(id, func, dynamic, dat, cfg, priority)	\
			struct task_struct task_##func = {	\
				.t_id = id, .t_uid = 0,				\
				.t_isdynamic = dynamic, .t_param = 0, \
				.t_execfg = cfg, .t_state = TSTOP, .t_step = 0, .t_stime = 0, .t_handle_res = 0, .t_trycnt = 0, \
				.data = (void *)dat, 	\
				.task_handler = func,	\
				.task_list = {.next = &task_##func.task_list, .prev = &task_##func.task_list},	\
				.wait_list = {.next = &task_##func.wait_list, .prev = &task_##func.wait_list},	\
				.msg_list  = {.next = &task_##func.msg_list,  .prev = &task_##func.msg_list},	\
				.t_priority = priority,		\
			}
#define TASK_EXTERN(func) \
			extern struct task_struct task_##func


int sched_task_create(struct task_struct *task, __u8 type, __u16 config);
int sched_task_set(struct task_struct *task, __u16 config);
struct list_head *sched_task_del(struct task_struct *task);

extern struct task_struct *current;

static inline int signal_pending(struct task_struct *p)
{
	return 0;
}

#define Trycnt(n)	(n)