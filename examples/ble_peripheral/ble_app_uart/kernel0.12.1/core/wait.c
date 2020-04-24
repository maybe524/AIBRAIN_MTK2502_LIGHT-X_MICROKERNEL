#include <include/list.h>
#include <include/sched.h>
#include <include/wait.h>
#include <include/errno.h>

LIST_HEAD(__kd_wait_list);

int wake_up_interruptible(wait_queue_head_t *wq)
{
	struct list_head *iter;

	list_for_each(iter, &wq->task_list) {
		struct task_struct *task = container_of(iter, struct task_struct, wait_list);
		wake_up(task);
	}

	return 0;
}

int wait_queue_put(__u8 id, struct task_struct *task)
{
	struct list_head *iter;

	list_for_each(iter, &__kd_wait_list) {
		wait_queue_head_t *wq = container_of(iter, wait_queue_head_t, queue_list);
		if (wq->id == id) {
			list_add(&task->wait_list, &wq->task_list);
			return 0;
		}
	}

	return -EINVAL;
}

int wait_queue_register(wait_queue_head_t *wq)
{
	list_add(&wq->queue_list, &__kd_wait_list);

	return 0;
}

/*
 * Note: we use "set_current_state()" _after_ the wait-queue add,
 * because we need a memory barrier there on SMP, so that any
 * wake-function that tests for the wait-queue being active
 * will be guaranteed to see waitqueue addition _or_ subsequent
 * tests in this thread will see the wakeup having taken place.
 *
 * The spin_unlock() itself is semi-permeable and only protects
 * one way (it only protects stuff inside the critical region and
 * stops them from bleeding out - it would still allow subsequent
 * loads to move into the critical region).
 */
void
prepare_to_wait(wait_queue_head_t *q, wait_queue_t *wait, int state)
{
	//
}
EXPORT_SYMBOL(prepare_to_wait);

/**
 * finish_wait - clean up after waiting in a queue
 * @q: waitqueue waited on
 * @wait: wait descriptor
 *
 * Sets current thread back to running state and removes
 * the wait descriptor from the given waitqueue if still
 * queued.
 */
void finish_wait(wait_queue_head_t *q, wait_queue_t *wait)
{
	unsigned long flags;

	__set_current_state(TASK_RUNNING);
	/*
	 * We can check for list emptiness outside the lock
	 * IFF:
	 *  - we use the "careful" check that verifies both
	 *    the next and prev pointers, so that there cannot
	 *    be any half-pending updates in progress on other
	 *    CPU's that we haven't seen yet (and that might
	 *    still change the stack area.
	 * and
	 *  - all other users take the lock (ie we can only
	 *    have _one_ other CPU that looks at or modifies
	 *    the list).
	 */
#if 0
	if (!list_empty_careful(&wait->task_list)) {
		spin_lock_irqsave(&q->lock, flags);
		list_del_init(&wait->task_list);
		spin_unlock_irqrestore(&q->lock, flags);
	}
#endif
}
EXPORT_SYMBOL(finish_wait);

