#include <include/msg.h>
#include <include/types.h>
#include <include/errno.h>
#include <include/init.h>
#include <include/time.h>

#define each_for_list(v)	for ((v) = 0; (v) < MSG_QUEUE_SIZE; (v)++)

static struct msg_queue kd_msg_queue = {0};

int msg_user_register(struct msg_user *user)
{
	struct list_head *item = NULL;

	if (!user)
		return -EINVAL;

	list_for_each(item, &kd_msg_queue.q_user) {
		if (item == &user->usr_list)
			return -EBUSY;
	}

	list_add(&user->usr_list, &kd_msg_queue.q_user);

	return 0;
}

int msg_user_unregister(struct msg_user *user)
{
	if (!user)
		return -EINVAL;

	msg_clear(user->task_id);
	list_del(&user->usr_list);

	return 0;
}

int msg_create(char sender_id, char reciever_id, 
			umode_t msg_type, void *msg_comment, char msg_len)
{
	int item;
	static int fnd_item = MSG_QUEUE_SIZE;
	struct msg_msg *msg = NULL;
	struct msg_queue *msq = &kd_msg_queue;
	struct msg_user *msr;

	if (!sender_id || !msg_type || !msg_comment || !msg_len || msg_len >= MSG_COMMENT_SIZE)
		return -EINVAL;

	if (msg_type & MSG_TO_RECEIVER_ONLINE) {
		struct list_head *item;

		list_for_each(item, &kd_msg_queue.q_user) {
			msr = container_of(item, struct msg_user, usr_list);
			if (msr->task_id == reciever_id)
				goto online_create;
		}
		goto offline_clear;
	}

	return -EINVAL;

offline_clear:
	each_for_list(item) {
		msg = &kd_msg_queue.q_messages[item];
		if (msg->busy && (msg->msg_type & MSG_TO_RECEIVER_OFFLINE) && \
				msg->receiver_id == reciever_id &&
				msg->sender_id == sender_id)
			msg_destroy(msg);
	}
	return -EINVAL;

online_create:
	each_for_list(item) {
		msg = &kd_msg_queue.q_messages[item];
		if (!msg->busy && fnd_item == MSG_QUEUE_SIZE)
			fnd_item = item;
		if ((msg_type & MSG_NO_REPEAT) && \
				msg->busy && \
				msg->msg_type == msg_type && \
				msg->msg_len == msg_len && !kmemcmp(msg->msg_comment, msg_comment, msg_len))
			return -EBUSY;
	}

	if (fnd_item == MSG_QUEUE_SIZE)
		return -ENOMEM;

	msg->sender_id = sender_id;
	msg->receiver_id = reciever_id;
	msg->busy = kd_true;
	msg->msg_time = get_timestamp();

	msg->msg_type = msg_type;
	kmemcpy(msg->msg_comment, msg_comment, msg_len);
	msg->msg_len = msg_len;

	list_add(&msg->msg_list, &msr->task->msg_list);

	return 0;
}

int msg_destroy(struct msg_msg *msg)
{
	if (msg->used_cnt)
		return -EBUSY;
	kmemset(msg, 0, sizeof(struct msg_msg));

	return 0;
}

int msg_clear(char reciever_id)
{
	int item;
	struct msg_msg *msg = NULL;

	each_for_list(item) {
		msg = &kd_msg_queue.q_messages[item];
		if (msg->receiver_id == reciever_id)
			msg_destroy(msg);
	}

	return 0;
}

struct msg_msg *msg_check(char reciever_id, char msg_type)
{
	int item;
	struct msg_msg *msg = NULL;

	each_for_list(item) {
		msg = &kd_msg_queue.q_messages[item];
		if (msg->busy && ((reciever_id && msg->receiver_id == reciever_id) || 
				(msg_type && msg->msg_type == msg_type)))
			return msg;
	}

	return NULL;
}

int msg_lock(struct msg_msg *msg)
{
	return ++msg->used_cnt;
}

int msg_unlock(struct msg_msg *msg)
{
	if (msg->used_cnt > 0)
		msg->used_cnt--;

	return msg->used_cnt;
}

int __init msg_init(void)
{
	struct msg_queue *mq = &kd_msg_queue;

	mq->q_user.prev = mq->q_user.next = &mq->q_user;

	return 0;
}

