#pragma once

#include <include/types.h>
#include <include/sched.h>
#include <include/list.h>

#define MSG_QUEUE_SIZE	5
#define MSG_COMMENT_SIZE	80

struct msg_msg {
	unsigned char sender_id, receiver_id;

	unsigned char used_cnt;
	kd_bool busy;

	time_t msg_time;
	umode_t msg_type;
	char msg_len;
	char msg_comment[MSG_COMMENT_SIZE];

	struct list_head msg_list;
};

struct msg_user {
	unsigned char task_id;
	struct task_struct *task;

	//unsigned char msgtype;
	struct list_head usr_list;
};

struct msg_queue {
	struct msg_msg q_messages[MSG_QUEUE_SIZE];

	struct list_head q_user;
};
#if 0
struct msg_task {
	/*
	 *  Ider: It is the abbreviation of identifier
	 */
	unsigned short m_ider;

	union {
		char str;
	} m_content;
};
#endif
enum msg_type {
	MSG_NONE = (0),
	MSG_TO_RECEIVER_ONLINE = (1 << 0),
	MSG_TO_RECEIVER_OFFLINE = (1 << 1),
	MSG_NO_REPEAT = (1 << 2),
	MSG_BROADCAST = (1 << 3),
	MSG_MAX,
};

#define MSG_USER_INIT(uname, taskid, taskinfo)		\
			static struct msg_user uname##_msg_user = {		\
					.task_id = (taskid),					\
					.task = (taskinfo),	\
			}
#define MSG_USER_INFO(uname)	\
			&uname##_msg_user

struct msg_msg *msg_check(char reciever_id, char msg_type);
int msg_destroy(struct msg_msg *msg);
int msg_create(char sender_id, char reciever_id, umode_t msg_type, void *msg_comment, char msg_len);
int msg_sender_register(struct msg_sender *sender);
int msg_sender_unregister(struct msg_sender *sender);
int msg_receiver_register(struct msg_receiver *reciever);
int msg_receiver_unregister(struct msg_receiver *reciever);
int msg_lock(struct msg_msg *msg);
int msg_unlock(struct msg_msg *msg);
int msg_clear(char reciever_id);
