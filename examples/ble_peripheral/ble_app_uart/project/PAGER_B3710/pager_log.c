#include <include/stdio.h>
#include <include/errno.h>
#include <include/delay.h>
#include <include/errno.h>
#include <include/lock.h>
#include <include/time.h>
#include <include/init.h>
#include <include/sched.h>
#include <include/task.h>
#include <include/msg.h>
#include <include/wait.h>

#include "inc/pager_time.h"

int log_store(__u8 *buff)
{
	int ret, storesize, logsize;
	KFILE *kfile;
	__u8 logbuff[100];
	struct localtime lt = {0};

	ret = get_localtime(&lt);
	ksprintf(logbuff, "%04d-%02d-%02d %02d-%02d-%02d:%s", lt.Year, lt.Month, lt.Day, lt.Hour, lt.Minute, lt.Second, buff);
	logsize = kstrlen(logbuff);
	storesize = ALIGN_UP(logsize, 4);
	kmemset(logbuff + logsize, 0x30, storesize - logsize);

	kfile = kfopen("/data/local.log", "ab+");
	if (!kfile) {
		ret = -EBUSY;
		goto store_end;
	}

	kfwrite(logbuff, storesize, 0, kfile);
	kfclose(kfile);

	return 0;

store_end:
	printk("Log store fail(%d)", ret);
	return 0;
}
