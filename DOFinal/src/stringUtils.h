/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  String utils
 */

#ifndef TW_STRING_UTILS_H
#define TW_STRING_UTILS_H

#include "string.h"

#ifdef __cplusplus
extern "C" {
#endif

char * lowercase(char *input);
char * uppercase(char *input);

char * duplicateString(const char * input);

#ifdef __cplusplus
}
#endif

#endif
