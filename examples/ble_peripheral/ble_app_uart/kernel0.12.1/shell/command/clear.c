#include "include/errno.h"
#include "include/string.h"
#include "include/assert.h"
#include "include/unistd.h"
#include "include/uart/uart.h"

unsigned char clear(struct task_struct *task, void *argc)
{
	printf("%s", CLRSCREEN);

	return 0;
}

EXPORT_CMD(clear,
		""
	);
