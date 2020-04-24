#ifndef _SAVEACTIVITY_H_
#define _SAVEACTIVITY_H_

#include <stdbool.h>

#include "Process_Acc_Data.h"

/* external variables called in saveActivity.c */
extern unsigned char WriteFlash, Interrupt;
extern bool vchg_enable, charge_signal_m2b;
extern bool g_enter_airplane_mode;
extern bool g_enter_standby_mode;

extern unsigned char lastState;
extern unsigned char mark;

/* global variables defined in saveActivity.c */
extern unsigned int SportTime, SportHourTime;
extern unsigned char SportStatus, SportHourStatus;
extern unsigned char SportDuration, SportHourDuration;
extern unsigned short int SportSteps, SportHourSteps;

extern unsigned char Current_status;
extern unsigned char activityStatus;
extern unsigned short int activityStep;
extern unsigned char DiffStepLast, DiffStepNow;

/* functions */
extern void InitActivity(void);
extern void CountActivity(void);
extern void ReturnActivity(unsigned int timeStamp);
extern void InitActivityTime(unsigned int timeStamp);

#endif
