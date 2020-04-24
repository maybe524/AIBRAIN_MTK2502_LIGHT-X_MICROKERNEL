#pragma once

#include "include/types.h"
#include "include/g-bios.h"
#include "include/fcntl.h"

#define printk(fmt, argc...)		kprintf("<%09d> "fmt, get_systime(), ##argc)

#define KERN_EMERG	"<0>"	/* system is unusable			*/
#define KERN_ALERT	"<1>"	/* action must be taken immediately	*/
#define KERN_CRIT	"<2>"	/* critical conditions			*/
#define KERN_ERR	"<3>"	/* error conditions			*/
#define KERN_WARNING	"<4>"	/* warning conditions			*/
#define KERN_NOTICE	"<5>"	/* normal but significant condition	*/
#define KERN_INFO	"<6>"	/* informational			*/
#define KERN_DEBUG	"<7>"	/* debug-level messages			*/

//int putchar(int);

char *gets(char *);

int puts(const char *);

int kprintf(const char *, ...);

int ksprintf(char *, const char *, ...);

int ksnprintf(char *, __size_t, const char *, ...);

// right here?
#ifdef CONFIG_DEBUG
#define DPRINT(fmt, args ...) \
	printk(fmt, ##args)
#define GEN_DBG(fmt, args ...) \
	printk("%s() line %d: " fmt, __func__, __LINE__, ##args)
#else
#define DPRINT(fmt, args ...)
#define GEN_DBG(fmt, args ...)
#endif

struct ___kiobuf {
	struct file *_fp;
	char	_fd;

	char	*_ptr;		// �ļ��������һ��λ��
	int		_cnt;		// ��ǰ�����������λ��
	char	*_base;	// ָ����λ��(�����ļ�����ʼλ��)
	int		_flag;		// �ļ���־
	int		_file;		// �ļ�����Ч����֤
	int		_charbuf;	// ��黺����״��,����޻������򲻶�ȡ
	int		_bufsiz;	// �ļ��Ĵ�С
	char	*_tmpfname;	// ��ʱ�ļ���
};

typedef struct ___kiobuf KFILE;

KFILE *kfopen(const char *path, const char *mode);
__size_t kfwrite(const void *buffer, __size_t size, __size_t count, KFILE *stream);
__size_t kfread(void *buffer, __size_t size, __size_t count, KFILE *stream);
int kfclose(KFILE *stream);
loff_t kflseek(loff_t offset, int whence, KFILE *stream);
int kfstat(struct stat *stat, KFILE *stream);

