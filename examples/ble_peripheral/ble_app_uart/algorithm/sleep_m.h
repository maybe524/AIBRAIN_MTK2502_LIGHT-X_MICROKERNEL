#ifndef _SLEEP_H_
#define _SLEEP_H_

#include <stdbool.h>
#include <math.h>
#include "../../../../../../Include/boards/Target_board.h"

/* external variables called in sleep_m */
extern unsigned char gsr_x;
extern unsigned char Interrupt, WriteFlash;
extern bool vchg_enable, charge_signal_m2b;
extern bool g_enter_airplane_mode;
extern bool g_enter_standby_mode;

/* global variables defined in sleep_m */
/* Parameters Set */
extern unsigned int SleepTime, FallAsleep, WakeUp;
extern unsigned char SleepStatus, SleepDuration;
extern unsigned short int SleepValue;
extern unsigned short int minCounts[7], startMark;
extern unsigned char mark;
extern char SleepZone, WakeZone;
extern float score;

extern unsigned char ZeroCounter;
extern unsigned char state, lastState;
extern unsigned short int SleepMark, WakeMark;

extern void sleep_m(char *data, unsigned char pos, unsigned int timeStamp, char timeZone);

#endif 
