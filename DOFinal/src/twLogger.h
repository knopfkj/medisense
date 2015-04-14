/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Logging facility
 */

#ifndef twLogger_H
#define twLogger_H

#include "twOSPort.h"
#include "twErrors.h"

#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*log_function) ( enum LogLevel level, const char * timestamp, const char * message);

typedef struct twLogger {
    enum LogLevel level;
    log_function f;
	char isVerbose;
	char * buffer;
	TW_MUTEX mtx;
} twLogger;

twLogger * twLogger_Instance();
int twLogger_Delete();
int twLogger_SetLevel(enum  LogLevel level);
int twLogger_SetFunction(log_function f);
int twLogger_SetIsVerbose(char val);

void twLog(enum LogLevel level, const char * format, ... );
void twLogHexString(const char * msg, char * preamble, int32_t length);
void twLogMessage(void * m, char * preamble);

#ifdef __cplusplus
}
#endif

#endif
