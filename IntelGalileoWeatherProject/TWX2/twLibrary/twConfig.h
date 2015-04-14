/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  General configuration for C SDK
 */

#ifndef TW_CONFIG_H
#define TW_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************************/
/*	 Tasker Configuration        */
/*********************************/
/*
If defined, the built-in tasker will be used.  If this is not defined then the application
developer is responsible for calling both twMessageHandler_msgHandlerTask and
twApi_TaskerFunction at regular intervals.
*/
#define ENABLE_TASKER 1

/*********************************/
/*  File Transfer Configuration  */
/*********************************/
/*
If defined, the file transfer system will be enabled
*/
#define ENABLE_FILE_XFER 0

/*********************************/
/*   Offline Message Handling    */
/*********************************/
/*
Following settings define how to handle outgoing
Request messages that occur when offline
0 or undefined - Do nothing
1 - store in memory up to a limit of OFFLINE_MSG_QUEUE_SIZE
2 - persist to the directory OFFLINE_MSG_STORE_DIR
*/
#define OFFLINE_MSG_STORE 1

#ifdef __cplusplus
}
#endif

#endif
