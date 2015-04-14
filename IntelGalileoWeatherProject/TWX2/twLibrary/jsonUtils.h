/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  JSON utils
 */

#ifndef TW_JSON_UTILS_H
#define TW_JSON_UTILS_H

#include "twInfoTable.h"
#include "twProperties.h"
#include "twServices.h"

#ifdef __cplusplus
extern "C" {
#endif

void addStringsToStream( twStream * s, ...);
int addPropertyDefJsonToStream(twPropertyDef * p, twStream * s);
int addServiceDefJsonToStream(twServiceDef * svc, twStream * s);

#ifdef __cplusplus
}
#endif

#endif
