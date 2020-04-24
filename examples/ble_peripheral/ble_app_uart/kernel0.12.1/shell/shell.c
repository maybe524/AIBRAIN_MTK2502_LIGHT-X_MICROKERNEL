#include "include/task.h"
#include "include/shell.h"
#include "include/stdio.h"
#include "include/errno.h"
#include "include/dirent.h"
#include "include/fs.h"
#include "include/sched.h"
#include "include/info/user.h"
#include "include/lock.h"
#include "include/uart/uart.h"
#include "include/msg.h"
#include "include/wait.h"

static struct msg_shell {
	/*
	 *	Ider: It is the abbreviation of identifier
	 */
	unsigned short m_ider;

	union {
		char str;
	} m_content;
};

TASK_EXTERN(shell_common);

struct msg_msg *task_check_msg(struct task_struct *task);
int task_del_msg(struct task_struct *task, struct msg_msg *msg);

#define SHELL_IDENTIFIER	('/.')
#define for_each_cmdinfo(var)	for ((var) = 0; ((var) < ARRAY_ELEM_NUM(cmd_info_list)); (var)++)

char copyright[] = {"37body"};

const struct command_info *cmd_info_list[] = 
{
#include "include/shell.cmds.h"
};

static int command_translate(char *cmd_line, 
        struct command_info **cmd_info,
        const char **opt_str)
{
	char *index, *p = cmd_line;
	int i, len = 0;
	struct command_info *each, **head = cmd_info_list;

	if (!cmd_line)
		return -EINVAL;

	while (kd_true) {
		if (IS_STR_END(p))
			return -EINVAL;
        
		if (' ' != *p) {
			index = p;
			break;
		}
		p++;
	}

	while (kd_true) {
		if (IS_STR_END(p) || ' ' == *p) {
			len = p - index;
			*opt_str = ++p;
			break;
		}
		p++;
	}

	for_each_cmdinfo(i) {
		if (len == kstrlen(head[i]->index) && 
				head[i] && 
				!kstrncmp(index, head[i]->index, len)) {
			*cmd_info = head[i];
			return 0;
		}
	}

end:
	return -EEXIST;
}

int shell(char *cmd_line)
{
	int ret;
	char *cmd_opt;
	struct command_info *cmd_info;
	struct nameidata nd;
	struct user_info *ui;

	get_fs_pwd(&nd.path);
	get_user_info(&ui);
	kprintf("\r\n""%s@%s:%s%c %s", ui->name, copyright, nd.path.dentry->d_iname, ui->id ? '$' : '#', cmd_line);

	ret = command_translate(cmd_line, &cmd_info, &cmd_opt);
	if (!ret)
		cmd_info->call_func(NULL, (void *)cmd_opt);
	else if (ret == -EINVAL)
		goto end;
	else
		kprintf("shell: command not found\r\n"
				"Try `help -a' for more information.\r\n");
end:
	return ret;
}

static unsigned char __sched
	shell_common(struct task_struct *task, void *data)
{
	char *p;

	if (check_lock(&task->lock))
		return -EBUSY;
	lock(&task->lock);

TASK_GEN_STEP_ENTRY(0) {
		struct msg_msg *msg = task_check_msg(task);
		if (msg) {
			struct msg_shell *msg_shell = (struct msg_shell *)msg->msg_comment;

			msg_lock(msg);
			if (msg_shell->m_ider == SHELL_IDENTIFIER)
				shell(&msg_shell->m_content.str);
			msg_unlock(msg);

			task_del_msg(task, msg);
		}
	}
	sleep_on(current);

	unlock(&task->lock);

	return TSLEEP;
}
TASK_INIT(CMD_SHELL, shell_common, kd_false, NULL, 
		EXEC_TRUE,
		SCHED_PRIORITY_HIGH);
static struct msg_user shell_msg_user = {.task_id = CMD_SHELL, .task = TASK_INFO(shell_common)};

int __init shell_init(void)
{
	int ret;

	ret = sched_task_create(TASK_INFO(shell_common),
					SCHED_COMMON_TYPE,
					EXEC_TRUE | EXEC_INTERRUPTIBLE);
	if (!ret)
		ret = wait_queue_put(WAIT_QUEUE_UART, TASK_INFO(shell_common));
	ret = msg_user_register(&shell_msg_user);

	return 0;
}

subsys_init(shell_init);