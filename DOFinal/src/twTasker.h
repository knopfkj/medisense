/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Simple Tasker
 */

#ifndef TASKER_H
#define TASKER_H

#include "twDefaultSettings.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************/
/*       Tasker           */
/**************************/
/*
twTaskFunction - function signature of a task.  Called in round robin
fashion.  Task functions MUST return in a cooperative fashion.
Parameters:
	sys_msecs - the current non-rollover 64 bit msec counter value.  On a platform
	            with millisecond datetime capabilities, this will be the current date/time
	params - void pointer to the params of the function
Return:
	Nothing
*/
typedef void (*twTaskFunction) (uint64_t sys_msecs, void * params);

/**************************/
/*        Tasks           */
/**************************/
typedef struct twTask {
   uint32_t runTimeIntervalMsec; 
   uint64_t nextRunTick; 
   twTaskFunction func;
} twTask;

/*
twTasker_Initialize - intializes the tasker.  Task functions are called
in round robin fashion at a rate defined when the tasjks are added
Parameters:
	now - the current DATETIME
	params - void pointer to the params of the function
Return:
	Nothing
*/
void twTasker_Initialize();

/*
twTasker_CreateTask - adds a new task to the tasker
Parameters:
	runTimeIntervalMsec - period (in msec) at whcih to call this task
	func - pointer to the function to call when executing the task
Return:
	int - the id of the resulting task
*/
int twTasker_CreateTask(uint32_t runTimeIntervalMsec, twTaskFunction func);

/*
twTasker_RemoveTask - removes a task from the tasker
Parameters:
	id - id of the task to remove
Return:
	int - zero if successful, non-zero if an error occurred
*/
int twTasker_RemoveTask(int id);

#ifdef __cplusplus
}
#endif

#endif
