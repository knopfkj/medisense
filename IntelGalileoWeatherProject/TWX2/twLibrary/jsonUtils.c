/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  JSON Utils
 */

#include "twDefinitions.h"
#include "jsonUtils.h"
#include "twLogger.h"

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

void addStringsToStream( twStream * s, ...) {
	va_list va;
	char buf[256];
	char * tmp = NULL;
	if (!s) return;
	va_start(va, s);
	tmp = va_arg(va, char *);
	while (tmp) {
		memset(buf,0,256);
		snprintf(buf, 255, "%s", tmp);
		twStream_AddBytes(s, buf, strlen(buf));
		tmp = va_arg(va, char *);
	}
	va_end(va);
}

int addPropertyDefJsonToStream(twPropertyDef * p, twStream * s) {
	char val[32];
	if (!p || !p->name || !s) return TW_INVALID_PARAM;
	addStringsToStream(s,"\"", p->name,"\":{\"name\":\"", p->name,"\",", NULL);
	addStringsToStream(s,"\"description\":\"", p->description ? p->description : "", "\",", NULL);
	addStringsToStream(s,"\"baseType\":\"", baseTypeToString(p->type), "\",",0);
	addStringsToStream(s,"\"aspects\":{\"pushType\":\"", p->pushType ? p->pushType : "ALWAYS", "\",", NULL);
	addStringsToStream(s,"\"cacheTime\":", !strcmp(p->pushType, "NEVER") ? "-1" : "0", ",", NULL);
	sprintf(val, "%f",p->pushThreshold);
	addStringsToStream(s,"\"pushThreshold\":", val, "}}", NULL);
	return TW_OK;
}

int addDataShapeEntryJsonToStream(twStream * s, twDataShapeEntry * entry) {
	if (!entry || !s) return TW_INVALID_PARAM;
	addStringsToStream(s,"\"", entry->name,"\":{\"name\":\"", entry->name,"\",", NULL);
	addStringsToStream(s,"\"description\":\"", entry->description ? entry->description : "", "\",", NULL);
	addStringsToStream(s,"\"baseType\":\"", baseTypeToString(entry->type), "\",",0);
	addStringsToStream(s,"\"aspects\":{", NULL);
	if (entry->aspects) {
		ListEntry * aspectEntry = NULL;
		aspectEntry = twList_Next(entry->aspects,NULL);
		while (aspectEntry && aspectEntry->value) {
			twDataShapeAspect * aspect = (twDataShapeAspect *)aspectEntry->value;
			addStringsToStream(s,"\"", aspect->name,"\":", NULL);
			switch (aspect->value->typeFamily) {
			case TW_NUMBER:
			case TW_INTEGER:
				{
				char val[32];
				if (aspect->value->typeFamily) sprintf(val, "%d",aspect->value->val.integer);
				else  sprintf(val, "%f",aspect->value->val.number);
				addStringsToStream(s,val, NULL);
				break;
				}
			case TW_BOOLEAN:
				if (aspect->value->val.boolean) addStringsToStream(s,"true", NULL);
				else addStringsToStream(s,"false", NULL);
				break;
			case TW_STRING:
				addStringsToStream(s, "\"", aspect->value->val.bytes.data, "\"", NULL);
				break;
			default:
				break;
			}
			aspectEntry = twList_Next(entry->aspects,aspectEntry);
			if (aspectEntry && aspectEntry->value) addStringsToStream(s,",", NULL);
		}
	}
	addStringsToStream(s,"}}", NULL);
	return TW_OK;
}

int addServiceDefJsonToStream(twServiceDef * svc, twStream * s) {
	ListEntry * le = NULL;
	if (!svc || !svc->name || !s) return TW_INVALID_PARAM;
	addStringsToStream(s,"\"", svc->name,"\":{\"name\":\"", svc->name,"\",", NULL);
	addStringsToStream(s,"\"description\":\"", svc->description ? svc->description : "", "\",", NULL);
	addStringsToStream(s,"\"Inputs\":{", NULL);
	if (svc->inputs && svc->inputs->entries) {
		addStringsToStream(s,"\"fieldDefinitions\":{", NULL);
		le = twList_Next(svc->inputs->entries,NULL);
		while (le && le->value) {
			twDataShapeEntry * entry = (twDataShapeEntry *)le->value;
			addDataShapeEntryJsonToStream(s, entry);
			le = twList_Next(svc->inputs->entries,le);
			if (le && le->value) addStringsToStream(s,",", NULL);
		}
		addStringsToStream(s,"}",NULL);
	}
	addStringsToStream(s,"},\"Outputs\":{\"name\":\"result\",", NULL);
	addStringsToStream(s,"\"baseType\":\"", baseTypeToString(svc->outputType), "\",",NULL);
	if (svc->outputDataShape && svc->outputDataShape->name) {
		addStringsToStream(s,"\"aspects\":{\"dataShape\":\"", svc->outputDataShape->name, "\"},",NULL);
	}
	addStringsToStream(s,"\"fieldDefinitions\":{",NULL);
	if (svc->outputDataShape && svc->outputDataShape->entries) {
		ListEntry * le = twList_Next(svc->outputDataShape->entries,NULL);
		while (le && le->value) {
			addDataShapeEntryJsonToStream(s, (twDataShapeEntry *)(le->value));
			le = twList_Next(svc->outputDataShape->entries,le);
			if (le && le->value) addStringsToStream(s,",",NULL);
		}
	}
	addStringsToStream(s,"}}}",NULL);

	//printf("\n\n%s\n\n", s->data);

	return TW_OK;
}
