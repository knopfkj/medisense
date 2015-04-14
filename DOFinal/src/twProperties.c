/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Metadata browsing and property/service functions
 */

#include "twConfig.h"
#include "twOSPort.h"
#include "twLogger.h"
#include "twProperties.h"
#include "stringUtils.h"
#include "list.h"


twPropertyDef * twPropertyDef_Create(char * name, enum BaseType type, char * description, char * pushType, double pushThreshold) {
	twPropertyDef * tmp = NULL;
	if (!name) {
		TW_LOG(TW_ERROR,"twPropertyDef_Create: NULL name pointer passed in");
		return 0;
	}
	tmp = (twPropertyDef *)TW_CALLOC(sizeof(twPropertyDef), 1);
	if (!tmp) {
		TW_LOG(TW_ERROR,"twPropertyDef_Create: Error allocating memory");
		return 0;
	}
	tmp->name = duplicateString(name);
	tmp->type = type;
	tmp->description = duplicateString(description);
	tmp->pushType = duplicateString(pushType);
	tmp->pushThreshold = pushThreshold;
	return tmp;
}

void twPropertyDef_Delete(void * input) {
	if (input) {
		twPropertyDef * tmp = (twPropertyDef *)input;
		TW_FREE(tmp->name);
		if (tmp->description) TW_FREE(tmp->description);
		if (tmp->pushType) TW_FREE(tmp->pushType);
		TW_FREE(tmp);
	}
}

twProperty * twProperty_Create(char * name, twPrimitive * value, DATETIME timestamp) {
	twProperty * tmp = NULL;
	if (!name || !value) {
		TW_LOG(TW_ERROR,"twPropertyVTQ_Create: NULL name or value pointer passed in");
		return 0;
	}
	tmp = (twProperty *)TW_CALLOC(sizeof(twProperty), 1);
	if (!tmp) {
		TW_LOG(TW_ERROR,"twPropertyVTQ_Create: Error allocating memory");
		return 0;
	}
	tmp->name = duplicateString(name);
	tmp->value = value; /* We won this pointer now */
	if (!timestamp) tmp->timestamp = twGetSystemTime(TRUE);
	else tmp->timestamp = timestamp;
	return tmp;
}

void twProperty_Delete(void * input) {
	if (input) {
		twProperty * tmp = (twProperty *)input;
		TW_FREE(tmp->name);
		twPrimitive_Delete(tmp->value);
		TW_FREE(tmp);
	}
}




