/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Simple Tasker
 */

#include "twConfig.h"
#include "twLogger.h"
#include "twOSPort.h"
#include "twTasker.h"

twTask twTasks[TW_MAX_TASKS];
int lastTaskRun = 0;

void twTaskerInitialize() {
	int i;
	for (i = 0; i < TW_MAX_TASKS; i++) {
		twTasks[i].func = NULL;
	}
	twTasker_Start();
}

uint64_t tickCount = 0;
uint8_t tickInProgress;
void tickTimerCallback (void * params) {
	int count = 0;
	int i = lastTaskRun;
	/*tickCount++;*/
	if (tickInProgress) return;
	tickInProgress = 1;
	tickCount = twGetSystemMillisecondCount();
	while (count < TW_MAX_TASKS) {
		uint64_t now = twGetSystemMillisecondCount();
		if (twTasks[i].func != 0 && tickCount > twTasks[i].nextRunTick) {
			twTasks[i].func(now, params);
			twTasks[i].nextRunTick = tickCount + twTasks[i].runTimeIntervalMsec * TICKS_PER_MSEC;
		}
		lastTaskRun = i++;
		if (i == TW_MAX_TASKS) i = 0;
		count++;
	}
	tickInProgress = 0;
}

int twTasker_CreateTask(uint32_t runTimeIntervalMsec, twTaskFunction func) {
	int i;
	/* Find and empty slot */
	for (i = 0; i < TW_MAX_TASKS; i++) {
		if (twTasks[i].func == 0) {
			twTasks[i].func = func;
			twTasks[i].runTimeIntervalMsec = runTimeIntervalMsec;
			twTasks[i].nextRunTick = 0;
			return i;
		}
	}
	/* Didn't find an open slot */
	return TW_MAX_TASKS_EXCEEDED;
}

int twTasker_RemoveTask(int id) {
	if (id < TW_MAX_TASKS) {
		twTasks[id].func = NULL;
		twTasks[id].runTimeIntervalMsec = 0;
		twTasks[id].nextRunTick = 0;
	}
	/* Didn't the task slot */
	return TW_TASK_NOT_FOUND;
} 

