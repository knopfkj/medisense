/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Wrappers for OS Specific functionality
 */

#ifndef TW_OS_PORT_H
#define TW_OS_PORT_H

#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#include "twConfig.h"
#include "twDefinitions.h"

#include "twLinux.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Logging */
enum LogLevel {
	TW_TRACE,
	TW_DEBUG,
	TW_INFO,
	TW_WARN,
	TW_ERROR,
	TW_FORCE,
	TW_AUDIT
};
extern char * levelString(enum LogLevel level); /* Implemented in utils/logger.c */
void LOGGING_FUNCTION( enum LogLevel level, const char * timestamp, const char * message );

/* Time Functions */
/* Time is represented as a 64 bit value represenitng milliseconds since the epoch */
char twTimeGreaterThan(DATETIME t1, DATETIME t2);
char twTimeLessThan(DATETIME t1, DATETIME t2);
DATETIME twAddMilliseconds(DATETIME t1, int32_t msec);
DATETIME twGetSystemTime(char utc);
uint64_t twGetSystemMillisecondCount();
void twGetSystemTimeString(char * s, const char * format, int length, char msec, char utc);
void twGetTimeString(DATETIME time, char * s, const char * format, int length, char msec, char utc);
void twSleepMsec(int milliseconds);

/* Mutex Functions */
TW_MUTEX twMutex_Create();
void twMutex_Delete(TW_MUTEX m);
void twMutex_Lock(TW_MUTEX m);
void twMutex_Unlock(TW_MUTEX m);

/* Socket Functions */
#define CLOSED (char)0
#define OPEN (char)1

typedef struct twSocket {
	TW_SOCKET_TYPE sock; /* socket descriptor */
	TW_ADDR_INFO addr; /* address to use */
	TW_ADDR_INFO * addrInfo; /* Addr Info struct head - use to free */
	char state;
	char * host;
	uint16_t port;
	char * proxyHost;
	uint16_t proxyPort;
	char * proxyUser;
	char * proxyPass;
} twSocket;

/* MSG_NOSIGNAL is not defined on some implementations */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

twSocket * twSocket_Create(const char * host, int16_t port, uint32_t options);
int twSocket_Connect(twSocket * s);
int twSocket_Reconnect(twSocket * s);
int twSocket_Close(twSocket * s);
int twSocket_WaitFor(twSocket * s, int timeout);
int twSocket_Read(twSocket * s, char * buf, int len, int timeout);
int twSocket_Write(twSocket * s, char * buf, int len, int timeout);
int twSocket_Delete(twSocket * s);
int twSocket_GetLastError();
int twSocket_SetProxyInfo(twSocket * s,char * proxyHost, uint16_t proxyPort, char * proxyUser, char * proxyPass);

/* Thread/Task Functions */
void twTasker_Start();
void twTasker_Stop();

/* File Transfer */
int twDirectory_GetFileInfo(char * filename, uint64_t * size, DATETIME * lastModified, char * isDirectory, char * isReadOnly);
char twDirectory_FileExists(char * name);
int twDirectory_CreateFile(char * name);
int twDirectory_MoveFile(char * fromName, char * toName);
int twDirectory_DeleteFile(char * name);
int twDirectory_CreateDirectory(char * name);
int twDirectory_DeleteDirectory(char * name);
TW_DIR twDirectory_IterateEntries(char * dirName, TW_DIR dir, char ** name, uint64_t * size,
											   DATETIME * lastModified, char * isDirectory, char * isReadOnly);
int twDirectory_GetLastError();

#ifdef __cplusplus
}
#endif

#endif
