#pragma once

#define L0		0x00
#define L1		0x01
#define L2		0x02
#define L3		0x03

int log_store(__u8 *buff);

#define logs(level, fmt, argc)	do {		\
				char log[100];				\
				if (level > L2)				\
					break;					\
				ksprintf(log, fmt, ##argc);	\
				log_store(log);				\
			} while (0)