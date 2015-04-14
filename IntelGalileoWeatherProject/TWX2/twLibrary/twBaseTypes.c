/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Portable ThingWorx Base Types
 */

#include "twBaseTypes.h"
#include "twInfoTable.h"
#include "twLogger.h"
#include "stringUtils.h"

#include "string.h"

/* ///////////////  Base Type Functions ///////////////// */
const char * baseTypeToString(enum BaseType b) {
	switch(b) {
	case TW_NOTHING:
		return "NOTHING";
	case TW_STRING:
		return "STRING";
	case TW_NUMBER:
		return "NUMBER";
	case TW_INTEGER:
		return "INTEGER";
	case TW_BOOLEAN:
		return "BOOLEAN";
	case TW_DATETIME:
		return "DATETIME";
	case TW_TIMESPAN:
		return "TIMESPAN";
	case TW_INFOTABLE:
		return "INFOTABLE";
	case TW_LOCATION:
		return "LOCATION";
	case TW_XML:
		return "XML";
	case TW_JSON:
		return "JSON";
	case TW_QUERY:
		return "QUERY";
	case TW_IMAGE:
		return "IMAGE";
	case TW_HYPERLINK:
		return "HYPERLINK";
	case TW_IMAGELINK:
		return "IMAGELINK";
	case TW_PASSWORD:
		return "PASSWORD";
	case TW_HTML:
		return "HTML";
	case TW_TEXT:
		return "TEXT";
	case TW_TAGS:
		return "TAGS";
	case TW_SCHEDULE:
		return "SCHEDULE";
	case TW_VARIANT:
		return "VARIANT";
	case TW_GUID:
		return "GUID";
	case TW_BLOB:
		return "BLOB";
	case TW_THINGNAME:
		return "THINGNAME";
	case TW_THINGSHAPENAME:
		return "THINGSHAPENAME";
	case TW_THINGTEMPLATENAME:
		return "THINGTEMPLATENAME";
	case TW_DATASHAPENAME:
		return "DATASHAPENAME";
	case TW_MASHUPNAME:
		return "MASHUPNAME";
	case TW_MENUNAME:
		return "MENUNAME";
	case TW_BASETYPENAME:
		return "BASETYPENAME";
	case TW_USERNAME:
		return "USERNAME";
	case TW_GROUPNAME:
		return "GROUPNAME";
	case TW_CATEGORYNAME:
		return "CATEGORYNAME";
	case TW_STATEDEFINITIONNAME:
		return "STATEDEFINITIONNAME";
	case TW_STYLEDEFINITIONNAME:
		return "STYLEDEFINITIONNAME";
	case TW_MODELTAGVOCABULARYNAME:
		return "MODELTAGVOCABULARYNAME";
	case TW_DATATAGVOCABULARYNAME:
		return "DATATAGVOCABULARYNAME";
	case TW_NETWORKNAME:
		return "NETWORKNAME";
	case TW_MEDIAENTITYNAME:
		return "MEDIAENTITYNAME";
	case TW_APPLICATIONKEYNAME:
		return "APPLICATIONKEYNAME";
	case TW_LOCALIZATIONTABLENAME:
		return "LOCALIZATIONTABLENAME";
	case TW_ORGANIZATIONNAME:
		return "ORGANIZATIONNAME";
	case TW_PROPERTYNAME:
		return "PROPERTYNAME";
	case TW_SERVICENAME:
		return "SERVICENAME";
	case TW_EVENTNAME:
		return "EVENTNAME";
	case TW_DASHBOARDNAME:
		return "DASHBOARDNAME";
	default:
		return "UNKNOWN";
	}
}

enum BaseType baseTypeFromString(const char * s) {
	if (!s || !strcmp(s,"NOTHING")) return TW_NOTHING;
	if (!strcmp(s,"STRING")) return TW_STRING;
	if (!strcmp(s,"NUMBER")) return TW_NUMBER;
	if (!strcmp(s,"BOOLEAN")) return TW_BOOLEAN;
	if (!strcmp(s,"DATETIME")) return TW_DATETIME;
	if (!strcmp(s,"TIMESPAN")) return TW_TIMESPAN;
	if (!strcmp(s,"INFOTABLE")) return TW_INFOTABLE;
	if (!strcmp(s,"LOCATION")) return TW_LOCATION;
	if (!strcmp(s,"XML")) return TW_XML;
	if (!strcmp(s,"JSON")) return TW_JSON;
	if (!strcmp(s,"QUERY")) return TW_QUERY;
	if (!strcmp(s,"IMAGE")) return TW_IMAGE;
	if (!strcmp(s,"INTEGER")) return TW_INTEGER;
	if (!strcmp(s,"HYPERLINK")) return TW_HYPERLINK;
	if (!strcmp(s,"IMAGELINK")) return TW_IMAGELINK;
	if (!strcmp(s,"PASSWORD")) return TW_PASSWORD;
	if (!strcmp(s,"HTML")) return TW_HTML;
	if (!strcmp(s,"TEXT")) return TW_TEXT;
	if (!strcmp(s,"TAGS")) return TW_TAGS;
	if (!strcmp(s,"SCHEDULE")) return TW_SCHEDULE;
	if (!strcmp(s,"VARIANT")) return TW_VARIANT;
	if (!strcmp(s,"GUID")) return TW_GUID;
	if (!strcmp(s,"BLOB")) return TW_BLOB;
	if (!strcmp(s,"THINGNAME")) return TW_THINGNAME;
	if (!strcmp(s,"THINGSHAPENAME")) return TW_THINGSHAPENAME;
	if (!strcmp(s,"THINGTEMPLATENAME")) return TW_THINGTEMPLATENAME;
	if (!strcmp(s,"DATASHAPENAME")) return TW_DATASHAPENAME;
	if (!strcmp(s,"MASHUPNAME")) return TW_MASHUPNAME;
	if (!strcmp(s,"MENUNAME")) return TW_MENUNAME;
	if (!strcmp(s,"BASETYPENAME")) return TW_BASETYPENAME;
	if (!strcmp(s,"USERNAME")) return TW_USERNAME;
	if (!strcmp(s,"GROUPNAME")) return TW_GROUPNAME;
	if (!strcmp(s,"CATEGORYNAME")) return TW_CATEGORYNAME;
	if (!strcmp(s,"STATEDEFINITIONNAME")) return TW_STATEDEFINITIONNAME;
	if (!strcmp(s,"STYLEDEFINITIONNAME")) return TW_STYLEDEFINITIONNAME;
	if (!strcmp(s,"MODELTAGVOCABULARYNAME")) return TW_MODELTAGVOCABULARYNAME;
	if (!strcmp(s,"DATATAGVOCABULARYNAME")) return TW_DATATAGVOCABULARYNAME;
	if (!strcmp(s,"NETWORKNAME")) return TW_NETWORKNAME;
	if (!strcmp(s,"MEDIAENTITYNAME")) return TW_MEDIAENTITYNAME;
	if (!strcmp(s,"PROPERTYNAME")) return TW_PROPERTYNAME;
	if (!strcmp(s,"SERVICENAME")) return TW_SERVICENAME;
	if (!strcmp(s,"EVENTNAME")) return TW_EVENTNAME;
	if (!strcmp(s,"DASHBOARDNAME")) return TW_DASHBOARDNAME;
	if (!strcmp(s,"APPLICATIONKEYNAME")) return TW_APPLICATIONKEYNAME;
	if (!strcmp(s,"LOCALIZATIONTABLENAME")) return TW_LOCALIZATIONTABLENAME;
	if (!strcmp(s,"ORGANIZATIONNAME")) return TW_ORGANIZATIONNAME;
	return TW_NOTHING;
}

/*
* Helper Functions
*/
signed char isBigEndian = -1; /* Global that should be set only once */

void swap4bytes(char * bytes) {
	uint32_t i = 0;
	char tmp[4];
	/* Only need to swap if we are on little endian architecture */
	if (isBigEndian < 0) {
		/* Test to see if we are big or little endian */
		unsigned short a=0x1234;
		isBigEndian = FALSE;
		if (*((unsigned char *)&a)==0x12) isBigEndian = TRUE;
	}
	if (isBigEndian) return;
	for (i = 0; i < 4; i++) {
		tmp[3 - i] = bytes[i] ;
	}
	memcpy(bytes, tmp, 4);
}

void swap8bytes(char * bytes) {
	uint32_t i = 0;
	char tmp[8];
	/*Only need to swap if we are on little endian architecture */
	if (isBigEndian < 0) {
		/* Test to see if we are big or little endian */
		unsigned short a=0x1234;
		isBigEndian = FALSE;
		if (*((unsigned char *)&a)==0x12) isBigEndian = TRUE;
	}
	if (isBigEndian) return;
	for (i = 0; i < 8; i++) {
		tmp[7 - i] = bytes[i];
	}
	memcpy(bytes, tmp, 8);
}

int stringToStream(char * string, twStream * s) {
	char tmp = 0;
	int32_t len = 0;
	if (!s) { 
		TW_LOG(TW_ERROR,"stringToStream: NULL Pointer passed in"); 
		return TW_ERROR_ALLOCATING_MEMORY; 
	}
	if (string) {
		len = strlen(string);
		if (len < 128) {
			tmp = (char)len;
			twStream_AddBytes(s, &tmp, 1);
		} else {
			swap4bytes((char *)&len);
			twStream_AddBytes(s, (char *)&len, 4);
		}
		twStream_AddBytes(s, string, strlen(string));
	} else {
		twStream_AddBytes(s, &tmp, 1);
	}
	return TW_OK;
}

char * streamToString(twStream *s) {
	unsigned char size[4];
	int32_t length;
	char * val = NULL;
	if (!s) {
		TW_LOG(TW_ERROR,"streamToString: NULL stream pointer"); 
		return 0; 
	}
	/* Get the first byte to check the size */
	twStream_GetBytes(s, &size[0], 1);
	if (size[0] > 127) {
		/* Need the full 4 bytes */
		twStream_GetBytes(s, &size[1], 3);
		length = size[0] * 0x1000000 + size[1] * 0x10000 + size[2] * 0x100 + size[3];
	} else {
		length = size[0];
	}
	val = (char *)TW_CALLOC(length + 1, 1);
	if (!val) {
		TW_LOG(TW_ERROR,"streamToString: Error allocating string storage"); 
		return 0; 
	}
	twStream_GetBytes(s, val, length);
	/* Null terminate the string just in case */
	val[length] = 0;
	return val;
}

/*
* General purpose binary data stream 
*/
twStream * twStream_Create() {
	twStream * s = NULL;
	s = (twStream *)TW_CALLOC(sizeof(twStream), 1);
	if (!s) {
		TW_LOG(TW_ERROR,"twStream_CreateFromCharArray: Error allocating stream");
		return NULL;
	}
	s->data = (char *)TW_CALLOC(STREAM_BLOCK_SIZE, 1);
	if (!s->data) {
		free(s);
		return NULL;
	}
	s->ptr = s->data;
	s->length = 0;
	s->maxlength = STREAM_BLOCK_SIZE;
	s->ownsData = TRUE;
	return s;
}

twStream * twStream_CreateFromCharArray(const char * data, uint32_t length) {
	twStream * s = NULL;
	if (!data) {
		TW_LOG(TW_ERROR,"twStream_CreateFromCharArray: NULL data pointer");
		return NULL;
	}
	s = (twStream *)TW_CALLOC(sizeof(twStream), 1);
	if (!s) {
		TW_LOG(TW_ERROR,"twStream_CreateFromCharArray: Error allocating stream");
		return NULL;
	}
	s->data = (char *)TW_CALLOC(length, 1);
	if (!s->data) {
		free(s);
		return NULL;
	}
	memcpy(s->data, data, length);
	s->ptr = s->data;
	s->length = length;
	s->maxlength = length;
	s->ownsData = TRUE;
	return s;
}

twStream * twStream_CreateFromCharArrayZeroCopy(const char * data, uint32_t length) {
	twStream * s = NULL;
	if (!data) {
		TW_LOG(TW_ERROR,"twStream_CreateFromCharArrayZeroCopy: NULL data pointer");
		return NULL;
	}
	s = (twStream *)TW_CALLOC(sizeof(twStream), 1);
	if (!s) {
		TW_LOG(TW_ERROR,"twStream_CreateFromCharArrayZeroCopy: Error allocating stream");
		return NULL;
	}
	s->data = (char *)data;
	s->ptr = s->data;
	s->length = length;
	s->maxlength = length;
	s->ownsData = FALSE;
	return s;
}

void twStream_Delete(void* tmp) {
	twStream * s = (struct twStream *)tmp;
	if (!s) { 
		TW_LOG(TW_ERROR,"twStream_Delete: NULL Pointer passed in"); 
		return; 
	}
	if (s->ownsData && s->data)TW_FREE(s->data);
	TW_FREE(s);
}

char * twStream_GetData(struct twStream * s) {
	if (!s) { TW_LOG(TW_ERROR,"twStream_GetData: NULL Pointer passed in"); return 0; }
	return s->data;
}

int32_t twStream_GetIndex(struct twStream * s) {
	if (!s) { TW_LOG(TW_ERROR,"twStream_GetIndex: NULL Pointer passed in"); return 0; }
	return s->ptr - s->data;
}

int32_t twStream_GetLength(struct twStream * s) {
	if (!s) { TW_LOG(TW_ERROR,"twStream_GetLength: NULL Pointer passed in"); return -1; }
	return s->length;
}

int twStream_AddBytes(struct twStream * s, void * b, uint32_t count) {
	if (!s || !b) { 
		TW_LOG(TW_ERROR,"twStream_AddBytes: NULL Pointer passed in"); 
		return TW_INVALID_PARAM; 
	}
	if (s->length + count > s->maxlength) {
		char * newData = NULL;
		TW_LOG(TW_TRACE,"twStream_AddBytes: adding %d bytes would exceed the length of %d. Expanding stream.", 
			count, s->maxlength); 
		/* add enough storage to hold the additonal bytes */
		while (s->length + count > s->maxlength) s->maxlength += STREAM_BLOCK_SIZE;
		newData = (char *)TW_CALLOC(s->maxlength, 1);
		if (!newData) {
			TW_LOG(TW_ERROR, "twStream_AddBytes: Error allocating new storage for stream");
			return TW_ERROR_ALLOCATING_MEMORY;
		}
		/* Copy the existing data */
		memcpy(newData, s->data, s->length);
		/* Adjust the pointer */
		s->ptr = newData + (s->ptr - s->data);
		/* Free up the old data if we owned it */
		if (s->ownsData) TW_FREE(s->data);
		/* Set the new data */
		s->data = newData;
		/* We now own this even if we didn't before */
		s->ownsData = TRUE;
	}
	memcpy(s->ptr, b, count);
	s->ptr += count;
	s->length += count;
	return TW_OK;
}

int twStream_GetBytes(struct twStream * s, void * buf, uint32_t count) {
	if (!s) { 
		TW_LOG(TW_ERROR,"twStream_GetBytes: NULL Pointer passed in"); 
		return TW_INVALID_PARAM; 
	}
	if (s->ptr + count > s->data + s->length) {
		TW_LOG(TW_WARN,"twStream_GetBytes: byte count of %d would exceed the length of %d", 
			count, s->length); 
		count = s->data + s->length - s->ptr;
	}
	memcpy(buf, s->ptr, count);
	s->ptr += count;
	return TW_OK;
}

int twStream_Reset(struct twStream * s) {
	if (s) {
		s->ptr = s->data;
		return TW_OK;
	} else return TW_INVALID_PARAM;
}

/*
* Generic Primitive Functions
*/
twPrimitive * twPrimitive_Create() {
	twPrimitive * p = (twPrimitive *)TW_CALLOC(sizeof(twPrimitive), 1);
	if (!p) { TW_LOG(TW_ERROR,"twPrimitive_Create: Error allocating primitive storage"); return NULL; }
	p->length = 1;
	p->type = TW_NOTHING;
	p->typeFamily = TW_NOTHING;
	return p;
}


twPrimitive * twPrimitive_CreateFromStream( twStream * s) {
	signed char tmp = -1;
	if (!s) { TW_LOG(TW_ERROR,"twPrimitive_CreateFromStream: NULL Pointer passed in"); return NULL; }
	/* Get the type */
	if (twStream_GetBytes(s, &tmp, 1)) {
		TW_LOG(TW_ERROR,"twPrimitive_CreateFromStream: Error getting type from stream"); 
		return NULL;
	}
	return twPrimitive_CreateFromStreamTyped(s, (enum BaseType)tmp);
}

twPrimitive * twPrimitive_CreateFromStreamTyped( twStream * s, enum BaseType type) {
	twPrimitive * p = NULL;
	if (!s) { TW_LOG(TW_ERROR,"twPrimitive_CreateFromStreamTyped: NULL Pointer passed in"); return NULL; }
	p = (twPrimitive *)TW_CALLOC(sizeof(twPrimitive), 1);
	if (!p) { TW_LOG(TW_ERROR,"twPrimitive_CreateFromStreamTyped: Error allocating primitive storage"); return NULL; }

	p->type = type;
	p->length = 1;
	/* Actions are based on the type */
	switch (type) {
	case 	TW_NOTHING:
		break;
	case 	TW_STRING:
	case	TW_XML:
	case	TW_QUERY:
	case	TW_HYPERLINK:
	case	TW_IMAGELINK:
	case	TW_PASSWORD:
	case	TW_HTML:
	case	TW_TEXT:
	case	TW_TAGS:
	case	TW_THINGNAME:
	case	TW_THINGSHAPENAME:
	case	TW_THINGTEMPLATENAME:
	case	TW_DATASHAPENAME:
	case	TW_MASHUPNAME:
	case	TW_MENUNAME:
	case	TW_BASETYPENAME:
	case	TW_USERNAME:
	case	TW_GROUPNAME:
	case	TW_CATEGORYNAME:
	case	TW_STATEDEFINITIONNAME:
	case	TW_STYLEDEFINITIONNAME:
	case	TW_MODELTAGVOCABULARYNAME:
	case	TW_DATATAGVOCABULARYNAME:
	case	TW_NETWORKNAME:
	case	TW_MEDIAENTITYNAME:
	case	TW_APPLICATIONKEYNAME:
	case	TW_LOCALIZATIONTABLENAME:
	case	TW_ORGANIZATIONNAME:
	case	TW_PROPERTYNAME:
	case	TW_SERVICENAME:
	case	TW_EVENTNAME:
	case	TW_DASHBOARDNAME:
	case	TW_GUID:
	case	TW_JSON:
		{
		unsigned char size[4];
		char sizeBytes = 1;
		p->typeFamily = TW_STRING;
		/* Get the first byte to check the size */
		twStream_GetBytes(s, &size[0], 1);
		if (size[0] > 127) {
			/* Need the full 4 bytes */
			twStream_GetBytes(s, &size[1], 3);
			p->val.bytes.len = (unsigned int)((size[0] & 0x7f) * 0x1000000) + (unsigned int)(size[1] * 0x10000) + 
				(unsigned int)(size[2] * 0x100) + size[3];
			sizeBytes = 4;
		} else {
			p->val.bytes.len = size[0];
		}
		p->length = p->val.bytes.len + sizeBytes;
		p->val.bytes.data = (char *)TW_CALLOC(p->val.bytes.len + 1, 1);
		if (!p->val.bytes.data) {
			TW_LOG(TW_ERROR,"twPrimitive_CreateFromStreamTyped: Error allocating string storage"); 
			twPrimitive_Delete(p);
			return 0; 
		}
		twStream_GetBytes(s, p->val.bytes.data, p->val.bytes.len);
		/* Null terminate the string just in case */
		p->val.bytes.data[p->val.bytes.len] = 0;
		if (type == TW_JSON) {

		}
		break;
		}
	case	TW_VARIANT:
		{
		char varType = 0;	
		p->typeFamily = TW_VARIANT;
		twStream_GetBytes(s, (char *)&varType, 1);
		p->val.variant = twPrimitive_CreateFromStreamTyped(s, (enum BaseType)varType);
		p->length = 1 + p->val.variant->length;
		break;
		}
	case	TW_NUMBER:
		{
		p->typeFamily = TW_NUMBER;
		p->length = 8;
		twStream_GetBytes(s, (char *)&p->val.number, 8);
		swap8bytes((char *)&p->val.number);
		break;
		}
	case	TW_INTEGER:
		{
		p->typeFamily = TW_INTEGER;
		p->length = 4;
		twStream_GetBytes(s, (char *)&p->val.integer, 4);
		swap4bytes((char *)&p->val.number);
		break;
		}
	case	TW_BOOLEAN:
		{
		p->typeFamily = TW_BOOLEAN;
		p->length = 1;
		twStream_GetBytes(s, (char *)&p->val.boolean, 1);
		break;
		}
	case	TW_DATETIME:
		{
		p->typeFamily = TW_DATETIME;
		p->length = 8;
		twStream_GetBytes(s, (char *)&p->val.datetime, 8);
		swap8bytes((char *)&p->val.datetime);
		break;
		}
	case 	TW_IMAGE:
	case	TW_BLOB:
		{
		unsigned char size[4];
		p->typeFamily = TW_BLOB;
		twStream_GetBytes(s, size, 4);
		p->val.bytes.len = size[0] * 0x1000000 + size[1] * 0x10000 + size[2] * 0x100 + size[3];
		p->val.bytes.data = (char *)TW_CALLOC(p->val.bytes.len, 1);
		if (!p->val.bytes.data) {
			TW_LOG(TW_ERROR,"twPrimitive_CreateFromStreamTyped: Error allocating string storage"); 
			twPrimitive_Delete(p);
			return 0; 
		}
		p->length = p->val.bytes.len + 4; 
		twStream_GetBytes(s, p->val.bytes.data, p->val.bytes.len);
		break;
		}
	case	TW_TIMESPAN:
		{
		TW_LOG(TW_WARN,"twPrimitive_Delete: Not handled yet"); return 0;
		break;
		}
	case	TW_LOCATION:
		{
		p->typeFamily = TW_LOCATION;
		/* Get the actual values */
		twStream_GetBytes(s, (char *)&p->val.location.longitude, 8);
		swap8bytes((char *)&p->val.location.longitude);
		twStream_GetBytes(s, (char *)&p->val.location.latitude, 8);
		swap8bytes((char *)&p->val.location.latitude);
		twStream_GetBytes(s, (char *)&p->val.location.elevation, 8);
		swap8bytes((char *)&p->val.location.elevation);
		p->length = 24;
		break;
		}
	case	TW_INFOTABLE:
		{
		p->typeFamily = TW_INFOTABLE;
		p->val.infotable = twInfoTable_CreateFromStream(s);
		if (p->val.infotable) p->length = p->val.infotable->length;
		break;
		}
	default:
		TW_LOG(TW_ERROR,"twPrimitive_CreateFromStreamTyped: Invalid type specified: %d", type); return 0;
		return NULL;
		break;
	}
	return p;
}

void twPrimitive_Delete(void * p) {
	twPrimitive * tmp = (twPrimitive *)p;
	if (!p) {  return; }
	switch (tmp->typeFamily) {
	case 	TW_STRING:
	case	TW_BLOB:
		{
		if (tmp->val.bytes.data) TW_FREE(tmp->val.bytes.data);
		break;
		}
	case	TW_INFOTABLE:
		{
		if (tmp->val.infotable) twInfoTable_Delete(tmp->val.infotable);
		break;
		}
	case	TW_VARIANT:
		{
		if (tmp->val.variant) twPrimitive_Delete(tmp->val.variant);
		break;
		}
	default:
		break;
	}
	TW_FREE(tmp);
}

int twPrimitive_ToStream(twPrimitive * p, twStream * s) {
	if (!s || !p) { 
		TW_LOG(TW_ERROR,"twPrimitive_ToStream: NULL Pointer passed in"); 
		return TW_INVALID_PARAM; 
	}
	twStream_AddBytes (s, (char *)&p->type, 1);
	/* Actions are based on the type */
	switch (p->typeFamily) {
	case 	TW_NOTHING:
		break;
	case 	TW_STRING:
	case	TW_BLOB:
		{
		char len[4];
		memcpy(len, &p->val.bytes.len, 4);
		swap4bytes(len);
		if (p->val.bytes.len > 127 || p->typeFamily == TW_BLOB) {
			if (p->typeFamily != TW_BLOB) len[0] |= 0x80;
			twStream_AddBytes (s, (char *)len, 4);
		} else {
			twStream_AddBytes (s, &len[3], 1);
		}
		twStream_AddBytes (s, p->val.bytes.data, p->val.bytes.len);
		break;
		}
	case	TW_VARIANT:
		{
		twPrimitive_ToStream(p->val.variant, s);
		break;
		}
	case	TW_NUMBER:
		{
		double tmp = p->val.number;
		swap8bytes((char *)&tmp);
		twStream_AddBytes (s, (char *)&tmp, 8);
		break;
		}
	case	TW_INTEGER:
		{
		int32_t tmp = p->val.integer;
		swap4bytes((char *)&tmp);
		twStream_AddBytes (s, (char *)&tmp, 4);
		break;
		break;
		}
	case	TW_BOOLEAN:
		{
		twStream_AddBytes (s, (char *)&p->val.boolean, 1);
		break;
		}
	case	TW_DATETIME:
		{
		uint64_t tmp = p->val.datetime;
		swap8bytes((char *)&tmp);
		twStream_AddBytes (s, (char *)&tmp, 8);
		break;
		}
	case	TW_TIMESPAN:
		{
		TW_LOG(TW_WARN,"twPrimitive_ToStream: Not handled yet"); return 0;
		break;
		}
	case	TW_LOCATION:
		{
		double tmp = p->val.location.longitude;
		swap8bytes((char *)&tmp);
		twStream_AddBytes (s, (char *)&tmp, 8);
		tmp = p->val.location.latitude;
		swap8bytes((char *)&tmp);
		twStream_AddBytes (s, (char *)&tmp, 8);
		tmp = p->val.location.elevation;
		swap8bytes((char *)&tmp);
		twStream_AddBytes (s, (char *)&tmp, 8);
		break;
		}
	case	TW_INFOTABLE:
		{
		twInfoTable_ToStream(p->val.infotable, s);
		break;
		}
	default:
		TW_LOG(TW_ERROR,"twPrimitive_ToStream: Invalid type specified: %d", p->typeFamily);
		return TW_INVALID_BASE_TYPE;
		break;
	}
	return TW_OK;
}

int twPrimitive_Compare(twPrimitive * p1, twPrimitive * p2) {
	if (!p1 || !p2) { 
		TW_LOG(TW_ERROR,"twPrimitive_Compare: NULL Pointer passed in"); 
		return -1; 
	}
	/* Quick check to see if the pointers are the same */
	if (p1 == p2) return 0;
	/* If the types are not the same then they are not equal */
	if (p1->type != p2->type) return 1;
	/* Rest of the compairsoins based on the type family*/
	switch (p1->typeFamily) {
	case 	TW_NOTHING:
		/* We know they have the same type so they both must be TW_NOTHING */
		return 0;
	case 	TW_STRING:
		{
		if (p1->val.bytes.len != p2->val.bytes.len) return 1;
		return strncmp(p1->val.bytes.data, p2->val.bytes.data, p1->val.bytes.len);
		}
	case	TW_BLOB:
		{
		uint32_t i = 0;
        if (p1->val.bytes.len != p2->val.bytes.len) return 1;
		for (i = 0; i < p1->val.bytes.len; i++) {
			if (p1->val.bytes.data[i] != p2->val.bytes.data[i]) return 1;
		}
		return 0;
		}
	case	TW_VARIANT:
		{
		return twPrimitive_Compare(p1->val.variant, p2->val.variant);
		}
	case	TW_NUMBER:
		{
		if (p1->val.number == p2->val.number) return 0;
		return 1;
		}
	case	TW_INTEGER:
		{
		if (p1->val.integer == p2->val.integer) return 0;
		return 1;
		break;
		}
	case	TW_BOOLEAN:
		{
		if (p1->val.boolean == p2->val.boolean) return 0;
		return 1;
		}
	case	TW_DATETIME:
		{
		if (twTimeGreaterThan(p1->val.datetime, p2->val.datetime)) return 1;
		if (twTimeLessThan(p1->val.datetime, p2->val.datetime)) return 1;
		return 0;
		}
	case	TW_TIMESPAN:
		{
		TW_LOG(TW_WARN,"twPrimitive_ToStream: Not handled yet"); return -1;
		}
	case	TW_LOCATION:
		{
		if ((p1->val.location.latitude == p2->val.location.latitude) &&
			(p1->val.location.longitude == p2->val.location.longitude) &&
			(p1->val.location.elevation == p2->val.location.elevation)) return 0;
		return 1;
		}
	case	TW_INFOTABLE:
		{
		return twInfoTable_Compare(p1->val.infotable, p2->val.infotable);
		}
	default:
		TW_LOG(TW_ERROR,"twPrimitive_Compare: Invalid type specified: %d", p1->typeFamily); 
		return -1;
	}
}

twPrimitive * twPrimitive_ZeroCopy(twPrimitive * p) {
	twPrimitive * copy = NULL;
	copy = twPrimitive_Create();
	if (!p || !copy) return NULL;
	copy->type = p->type;
	copy->typeFamily = p->typeFamily;
	copy->val = p->val;
	copy->length = p->length;
	p->type = TW_NOTHING; /* Do this so the byte array or infotable don't get deleted if the primitive does */
	p->typeFamily = TW_NOTHING;
	return copy;
}

twPrimitive * twPrimitive_FullCopy(twPrimitive * p) {
	if (!p) return NULL;
	switch (p->typeFamily) {
	case 	TW_NOTHING:
		/* We know they have the same type so they both must be TW_NOTHING */
		return twPrimitive_Create();
	case 	TW_STRING:
	case	TW_BLOB:
		{
		return twPrimitive_CreateFromVariable(p->val.bytes.data, p->type, TRUE, p->val.bytes.len);
		}
	case	TW_NUMBER:
		{
		return twPrimitive_CreateFromNumber(p->val.number);
		}
	case	TW_INTEGER:
		{
		return twPrimitive_CreateFromInteger(p->val.integer);
		}
	case	TW_BOOLEAN:
		{
		return twPrimitive_CreateFromBoolean(p->val.boolean);
		}
	case	TW_DATETIME:
		{
		return twPrimitive_CreateFromDatetime(p->val.datetime);
		}
	case	TW_TIMESPAN:
		{
		TW_LOG(TW_WARN,"twPrimitive_FullCopy: Not handled yet"); 
		return 0;
		}
	case	TW_LOCATION:
		{
		return twPrimitive_CreateFromLocation(&p->val.location);
		}
	case	TW_INFOTABLE:
		{
		twInfoTable * it = twInfoTable_FullCopy(p->val.infotable);
		twPrimitive * tmp = twPrimitive_CreateFromInfoTable(it);
		twInfoTable_Delete(it);
		return tmp;
		}
	default:
		TW_LOG(TW_ERROR,"twPrimitive_FullCopy: Invalid type specified: %d", p->typeFamily); 
		return NULL;
	}
}

twPrimitive * twPrimitive_CreateFromVariable(const void * value, enum BaseType type, char duplicateCharArray, uint32_t blobLength) {
	twPrimitive * p = twPrimitive_Create();
	if (!value) {		
		TW_LOG(TW_ERROR,"twPrimitive_CreateFromVariable: NULL value pointer passed in");
		return NULL;
	}
	if (!p) {
		TW_LOG(TW_ERROR,"twPrimitive_CreateFromVariable: Error creating primitve");
		return NULL;
	}
	p->type = type;
	p->length = 1;
	/* Actions are based on the type */
	switch (type) {
	case 	TW_NOTHING:
		break;
	case 	TW_STRING:
	case	TW_XML:
	case	TW_QUERY:
	case	TW_HYPERLINK:
	case	TW_IMAGELINK:
	case	TW_PASSWORD:
	case	TW_HTML:
	case	TW_TEXT:
	case	TW_TAGS:
	case	TW_THINGNAME:
	case	TW_THINGSHAPENAME:
	case	TW_THINGTEMPLATENAME:
	case	TW_DATASHAPENAME:
	case	TW_MASHUPNAME:
	case	TW_MENUNAME:
	case	TW_BASETYPENAME:
	case	TW_USERNAME:
	case	TW_GROUPNAME:
	case	TW_CATEGORYNAME:
	case	TW_STATEDEFINITIONNAME:
	case	TW_STYLEDEFINITIONNAME:
	case	TW_MODELTAGVOCABULARYNAME:
	case	TW_DATATAGVOCABULARYNAME:
	case	TW_NETWORKNAME:
	case	TW_MEDIAENTITYNAME:
	case	TW_APPLICATIONKEYNAME:
	case	TW_LOCALIZATIONTABLENAME:
	case	TW_ORGANIZATIONNAME:
	case	TW_PROPERTYNAME:
	case	TW_SERVICENAME:
	case	TW_EVENTNAME:
	case	TW_DASHBOARDNAME:
	case	TW_GUID:
	case	TW_JSON:
		{
		p->typeFamily = TW_STRING;
		if (!value) {
			p->val.bytes.data = (char *)TW_CALLOC(1,1);
			p->val.bytes.data[0] = 0x00;
			p->val.bytes.len = 0;
			p->length = 1;
		} else {
			if (duplicateCharArray) p->val.bytes.data = duplicateString((char *)value);
			else p->val.bytes.data = (char *)value;
			if (p->val.bytes.data) {
				p->val.bytes.len = strlen(p->val.bytes.data);
				p->length = p->val.bytes.len + 1;
				if (p->val.bytes.len > 127) p->length+=3;
			}
		}
		break;
		}
	case	TW_VARIANT:
		{
		p->typeFamily = TW_VARIANT;
		p->val.variant = (twPrimitive *)value;
		p->length = ((twPrimitive *)value)->length + 1;
		break;
		}
	case	TW_NUMBER:
		{
		p->typeFamily = TW_NUMBER;
		p->val.number = *((double *)value);
		p->length = 8;
		break;
		}
	case	TW_INTEGER:
		{
		p->typeFamily = TW_INTEGER;
		p->val.integer = *((int32_t *)value);
		p->length = 4;
		break;
		}
	case	TW_BOOLEAN:
		{
		p->typeFamily = TW_BOOLEAN;
		p->length = 1;
		p->val.boolean = *((char *)value);
		break;
		}
	case	TW_DATETIME:
		{
		p->typeFamily = TW_DATETIME;
		p->val.datetime = *((DATETIME *)value);
		p->length = 8;
		break;
		}
	case 	TW_IMAGE:
	case	TW_BLOB:
		{
		p->typeFamily = TW_BLOB;
		if (duplicateCharArray) {
			p->val.bytes.data = (char *)TW_CALLOC(blobLength, 1);
			if (!p->val.bytes.data) {
				TW_LOG(TW_ERROR,"twPrimitive_CreateFromVariable: Memory allocation failed");
				twPrimitive_Delete(p);
				p = 0;
			} else memcpy(p->val.bytes.data, value, blobLength);
		} else	p->val.bytes.data = (char *)value;
		p->val.bytes.len = blobLength;
		p->length = blobLength + 4;
		break;
		}
	case	TW_TIMESPAN:
		{
		TW_LOG(TW_WARN,"twPrimitive_CreateFromVariable: Timespan not handled yet"); 
		twPrimitive_Delete(p);
		p = 0;
		break;
		}
	case	TW_LOCATION:
		{
		p->typeFamily = TW_LOCATION;
		/* Get the actual values */
		p->val.location.longitude = ((twLocation *)value)->longitude;
		p->val.location.latitude = ((twLocation *)value)->latitude;
		p->val.location.elevation = ((twLocation *)value)->elevation;
		p->length = 24;
		break;
		}
	case	TW_INFOTABLE:
		{
		p->typeFamily = TW_INFOTABLE;
		p->val.infotable = twInfoTable_ZeroCopy((twInfoTable *)value);
		p->length = ((twInfoTable *)value)->length;
		break;
		}
	default:
		TW_LOG(TW_ERROR,"twPrimitive_CreateFromVariable: Invalid type specified: %d", type); return 0;
		return 0;
	}
	return p;
}

#define NA (char)0
twPrimitive * twPrimitive_CreateFromString(const char * value, char duplicate) {
	return twPrimitive_CreateFromVariable(value, TW_STRING, duplicate, NA);
}

twPrimitive * twPrimitive_CreateFromNumber(const double value) {
	return twPrimitive_CreateFromVariable(&value, TW_NUMBER, NA, NA);
}

twPrimitive * twPrimitive_CreateFromInteger(const int32_t value)  {
	return twPrimitive_CreateFromVariable(&value, TW_INTEGER, NA, NA);
}

twPrimitive * twPrimitive_CreateFromLocation(const twLocation * value) {
	return twPrimitive_CreateFromVariable(value, TW_LOCATION, NA, NA);
}

twPrimitive * twPrimitive_CreateFromBlob(const char * value, int32_t length, char isImage, char duplicate) {
	enum BaseType type = isImage ? TW_IMAGE : TW_BLOB;
	return twPrimitive_CreateFromVariable(value, type, duplicate, length);
}

twPrimitive * twPrimitive_CreateFromDatetime(const DATETIME value) {
	return twPrimitive_CreateFromVariable(&value, TW_DATETIME, NA, NA);
}

twPrimitive * twPrimitive_CreateFromCurrentTime() {
	DATETIME now = twGetSystemTime(TRUE);
	return twPrimitive_CreateFromDatetime(now);
}

twPrimitive * twPrimitive_CreateFromBoolean(const char value) {
	return twPrimitive_CreateFromVariable(&value, TW_BOOLEAN, NA, NA);
}

twPrimitive * twPrimitive_CreateFromInfoTable(twInfoTable * it) {
	return twPrimitive_CreateFromVariable(it, TW_INFOTABLE, NA, NA);
}

twPrimitive * twPrimitive_CreateVariant(twPrimitive * input) {
	/* Creates a VARIANT type primitive */
	twPrimitive * p = NULL;
	if (!input) return NULL;
	p = twPrimitive_Create();
	p->type = TW_VARIANT;
	p->typeFamily = TW_VARIANT;
	p->val.variant = input;
	p->length = input->length + 1;
	return p;
}

char * twPrimitive_DecoupleStringAndDelete(twPrimitive * p) {
	char * tmp;
	if (!p || p->typeFamily != TW_STRING) return NULL;
	tmp = p->val.bytes.data;
	p->typeFamily = TW_NOTHING;
	twPrimitive_Delete(p);
	return tmp;
}
