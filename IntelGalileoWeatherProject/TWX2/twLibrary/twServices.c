/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Service Metadata browsing service functions
 */

#include "twConfig.h"
#include "twOSPort.h"
#include "twLogger.h"
#include "twServices.h"
#include "stringUtils.h"
#include "list.h"

twServiceDef * twServiceDef_Create(char * name, char * description, twDataShape * inputs, 
								   enum BaseType outputType, twDataShape * outputDataShape) {
	twServiceDef * tmp = NULL;
	if (!name) {
		TW_LOG(TW_ERROR,"twServiceDef_Create: NULL name pointer passed in");
		return 0;
	}
	tmp = (twServiceDef *)TW_CALLOC(sizeof(twServiceDef), 1);
	if (!tmp) {
		TW_LOG(TW_ERROR,"twServiceDef_Create: Error allocating memory");
		return 0;
	}
	tmp->name = duplicateString(name);
	tmp->description = duplicateString(description);
	tmp->inputs = inputs;
	tmp->outputType = outputType;
	tmp->outputDataShape = outputDataShape;
	return tmp;
}

void twServiceDef_Delete(void * input) {
	if (input) {
		twServiceDef * tmp = (twServiceDef *)input;
		TW_FREE(tmp->name);
		if (tmp->description) TW_FREE(tmp->description);
		if (tmp->inputs) twDataShape_Delete(tmp->inputs);
		if (tmp->outputDataShape) twDataShape_Delete(tmp->outputDataShape);
		TW_FREE(tmp);
	}
}

