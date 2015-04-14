/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Portable ThingWorx InfoTable
 */

#include "twOSPort.h"
#include "twBaseTypes.h"
#include "list.h"

#ifndef TW_INFOTABLE_H
#define TW_INFOTABLE_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************/
/*   Datashape Aspects    */
/**************************/
typedef struct twDataShapeAspect {
	char * name;
	twPrimitive * value;
} twDataShapeAspect;

/*
twDataShapeAspect_Create - Create an aspect with the passed in name and value
Parameters:
    name - the name of the aspect.
	value - pointer to the primitive containing the type and value.
Return:
	twDataShapeAspect * - pointer to the allocated twDataShapeAspect or a NULL if an error occurred.
*/
twDataShapeAspect * twDataShapeAspect_Create(const char * name, twPrimitive * value);

/*
twDataShapeAspect_CreateFromStream - Create from data in a stream
Parameters:
    s - pointer containing the stream to parse
Return:
	twDataShapeAspect * - pointer to the allocated twDataShapeAspect or a NULL if an error occurred.
*/
twDataShapeAspect * twDataShapeAspect_CreateFromStream(twStream * s);

/*
twDataShapeAspect_Delete - Delete an aspect
Parameters:
    aspect - pointer containing the aspect to delete
Return:
	Nothing
*/
void twDataShapeAspect_Delete(void * aspect);

/**************************/
/*     Datashape Entry    */
/**************************/
typedef struct twDataShapeEntry {
	char * name;
	char * description;
	enum BaseType type;
	twList * aspects;
} twDataShapeEntry;

/*
twDataShapeEntry_Create - Create a field definiton for a Datashape
Parameters:
    name - the name of the field.
	description - the description of the field.
	type - the type associated with the created field
Return:
	twDataShapeEntry * - pointer to the allocated twDataShapeEntry or a NULL if an error occurred.
*/
twDataShapeEntry * twDataShapeEntry_Create(const char * name, const char * description, enum BaseType type);

/*
twDataShapeEntry_CreateFromStream - Create from data in a stream
Parameters:
    s - pointer containing the stream to parse
Return:
	twDataShapeEntry * - pointer to the allocated twDataShapeEntry or a NULL if an error occurred.
*/
twDataShapeEntry * twDataShapeEntry_CreateFromStream(struct twStream * s);

/*
twDataShapeEntry_Delete - Delete an aspect
Parameters:
    entry - pointer containing the entry to delete
Return:
	Nothing
*/
void twDataShapeEntry_Delete(void * entry);

/*
twDataShapeEntry_AddAspect - Create an aspect with the passed in name and value abd add it to 
Parameters:
	entry - pointer to DataShapeEntry that the aspect should be added to
    name - the name of the aspect.
	value - pointer to the primitive containing the type and value.
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twDataShapeEntry_AddAspect(struct twDataShapeEntry * entry, const char * name, twPrimitive * value);

/*
twDataShapeEntry_GetLength - Retrieves the length of entry if it were to be serialized to a stream 
Parameters:
	entry - pointer to DataShapeEntry to get the length of
Return:
	uint32_t - the serialized length.
*/
uint32_t twDataShapeEntry_GetLength(struct twDataShapeEntry * entry);

/*
twDataShapeEntry_ToStream - serialize a DataShapeEntry to a stream
Parameters:
	entry - pointer to DataShapeEntry to serialize
	s - pointer to the stream to serialize to
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twDataShapeEntry_ToStream(struct twDataShapeEntry * entry, twStream * s);


/**************************/
/*       Datashape        */
/**************************/
typedef struct twDataShape {
	int numEntries;
	twList * entries;
	char * name;
} twDataShape;

/*
twDataShape_Create - Create a Datashape from the passed in DataShapeEntry
Parameters:
    firstEntry - pointer to the DataShapeEntry to seed the DataShape with
Return:
	twDataShape * - pointer to the allocated twDataShape or a NULL if an error occurred.
*/
twDataShape * twDataShape_Create(twDataShapeEntry * firstEntry);

/*
twDataShape_CreateFromStream - Create a Datashape by parsing a stream
Parameters:
    s - pointer to the stream to parse
Return:
	twDataShape * - pointer to the allocated twDataShape or a NULL if an error occurred.
*/
twDataShape * twDataShape_CreateFromStream(struct twStream * s);

/*
twDataShape_Delete - Delete a DataShape and free all memory associated with it
Parameters:
    ds - pointer to the DataShape to delete
Return:
	Nothing
*/
void twDataShape_Delete(void * ds);

/*
twDataShape_GetLength - Retrieves the length of a DataShape if it were to be serialized to a stream 
Parameters:
	entry - pointer to DataShape to get the length of
Return:
	uint32_t - the serialized length.
*/
uint32_t twDataShape_GetLength(struct twDataShape * ds);

/*
twDataShape_SetName - set the name of a DataShape.  DataShape does NOT take ownership of the pointer
Parameters:
	ds - pointer to the DataShape that should be added to
	name - pointer to name string
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twDataShape_SetName(struct twDataShape * ds, char * name);

/*
twDataShape_AddEntry - add a new field (DataShapeEntry) to  a DataShape.  DataShape
then becomes the owner of that DataShapeEntry
Parameters:
	ds - pointer to the DataShape that should be added to
	entry - pointer to DataShapeEntry to add
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twDataShape_AddEntry(struct twDataShape * ds, struct twDataShapeEntry * entry);

/*
twDataShape_GetEntryIndex - gets the index of the field (DataShapeEntry) with the
name that is passed in
Parameters:
	ds - pointer to the DataShape to get the index from
	name - name of the field that we want the index of
Return:
	int - the field index (0 based) or a positive integral error code (see twErrors.h) if an occurred
*/
int twDataShape_GetEntryIndex(struct twDataShape * ds, const char * name, int * index);

/*
twDataShape_ToStream - serialize a DataShape to a stream
Parameters:
	ds - pointer to DataShape to serialize
	s - pointer to the stream to serialize to
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twDataShape_ToStream(struct twDataShape * ds, twStream * s);

/**************************/
/*    InfoTableRow        */
/**************************/
typedef struct twInfoTableRow {
	uint16_t numFields;
	twList * fieldEntries;
} twInfoTableRow;

/*
twInfoTableRow_Create - Create an InfoTable row from the passed in primitive
Parameters:
    firstEntry - pointer to the twPrimitive tht is the first field in the row
Return:
	twInfoTableRow * - pointer to the allocated twInfoTableRow or a NULL if an error occurred.
*/
twInfoTableRow * twInfoTableRow_Create(twPrimitive * firstEntry);

/*
twInfoTableRow_CreateFromStream - Create a twInfoTableRow by parsing a stream
Parameters:
    s - pointer to the stream to parse
Return:
	twInfoTableRow * - pointer to the allocated twInfoTableRow or a NULL if an error occurred.
*/
twInfoTableRow * twInfoTableRow_CreateFromStream(twStream * s);

/*
twInfoTableRow_Delete - Delete an InfoTableRow and free all memory associated with it
Parameters:
    row - pointer to the InfoTableRow to delete
Return:
	Nothing
*/
void twInfoTableRow_Delete(void * row);

/*
twInfoTableRow_GetCount - Returns the number of fields in the InfoTableRow
Parameters:
    row - pointer to the InfoTableRow to get the count of
Return:
	int - number of fields in the row or a positive integral error code (see twErrors.h) if an occurred
*/
int twInfoTableRow_GetCount(twInfoTableRow * row);

/*
twInfoTableRow_GetLength - Retrieves the length of an InfoTableRow if it were to be serialized to a stream 
Parameters:
	row - pointer to twInfoTableRow to get the length of
Return:
	uint32_t - the serialized length.
*/
uint32_t twInfoTableRow_GetLength(twInfoTableRow * row);

/*
twInfoTableRow_AddEntry - Adds a new field to an InfoTableRow.  The InfoTableRow becomes owner
of the twPrimitive pointer that is passed in.
Parameters:
    row - pointer to the InfoTableRow to add the field to
	entry - pointer to a primitive to add to the row
Return:
	int - 0 if successful or a positive integral error code (see twErrors.h) if an occurred
*/
int twInfoTableRow_AddEntry(twInfoTableRow * row, twPrimitive * entry);

/*
twInfoTableRow_GetEntry - get the primtive value of a field entry in an InfoTableRow.  The
InfoTableRow retains ownership of the retunred pointer, so do NOT delete it.
Parameters:
	row - pointer to the InfoTableRow to get the value from
	index - zero based index of the field to retrieve
Return:
	twPrimitive - pointer to the primitive value, 0 if an error occurred.
*/
twPrimitive * twInfoTableRow_GetEntry(twInfoTableRow * row, int index);

/*
twInfoTableRow_ToStream - serialize an InfoTableRow to a stream
Parameters:
	row - pointer to the InfoTableRow to serialize
	s - pointer to the stream to serialize to
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twInfoTableRow_ToStream(twInfoTableRow * row, twStream * s);


/**************************/
/*      InfoTable         */
/**************************/
typedef struct twInfoTable {
	twDataShape * ds;
	twList * rows;
	uint32_t length;
	TW_MUTEX mtx;
} twInfoTable;

/*
twInfoTableRow_Create - Create an InfoTable from the passed in DataShape
Parameters:
    shape - pointer to the DataShape that the InfoTable will use
Return:
	twInfoTable * - pointer to the allocated twInfoTable or a NULL if an error occurred.
*/
twInfoTable * twInfoTable_Create(twDataShape * shape);

/*
twInfoTable_CreateFromStream - Create a twInfoTable by parsing a stream
Parameters:
    s - pointer to the stream to parse
Return:
	twInfoTable * - pointer to the allocated twInfoTable or a NULL if an error occurred.
*/
twInfoTable * twInfoTable_CreateFromStream(twStream * s);

/*
twInfoTable_Delete - Delete an InfoTable and free all memory associated with it
Parameters:
    it - pointer to the InfoTable to delete
Return:
	Nothing
*/
void twInfoTable_Delete(void * it);

/*
twInfoTable_FullCopy - Creates a new infotable that fully copies the original.
The resulting infotable shares nothing with the original.
Parameters:
    it - pointer to the InfoTable to delete
Return:
	twInfoTable * - pointer to the newly created infoTable
*/
twInfoTable * twInfoTable_FullCopy(twInfoTable * it);

/*
twInfoTable_ZeroCopy - Creates a new infotable using the pointers assigned to the 
original infoTable.  It zeros out the pointers in the original infotable so that it
can be safely deleted
Parameters:
    it - pointer to the InfoTable to delete
Return:
	twInfoTable * - pointer to the newly created infoTable
*/
twInfoTable * twInfoTable_ZeroCopy(twInfoTable * it);

/*
twInfoTable_Compare - Compares two InfoTables for equivalence.
	p1 - pointer the twInfoTable to be compared against.  
	p2 - pointer the twInfoTable to compare. 
Return:
	int - 0 if the twInfoTables are identical, 1 if they are not the same, -1 if an error occurred
*/
int twInfoTable_Compare(twInfoTable * p1, twInfoTable * p2);

/*
twInfoTable_AddRow - add a row to an InfoTable.  InfoTable becomes owner of the
InfoTableRow pointer
Parameters:
	it - pointer to the infoTable to add to
	row - pointer to the InfoTableRow to add
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twInfoTable_AddRow(twInfoTable * it, twInfoTableRow * row);

/*
twInfoTable_GetEntry - retrieves the row from an InfoTable at the specified
zero based index.  InfoTable retains ownership of the resulting pointer. Do NOT
delete it.
Parameters:
	it - pointer to the infoTable to add to
	index - the zero based index of the row to retrieve
Return:
	twInfoTableRow * - pointer to the row, or a zero if an error occurred
*/
twInfoTableRow * twInfoTable_GetEntry(twInfoTable * it, int index);

/*
twInfoTable_ToStream - serialize an InfoTable to a stream
Parameters:
	row - pointer to the InfoTableRow to serialize
	s - pointer to the stream to serialize to
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twInfoTable_ToStream(twInfoTable * it, twStream * s);

/***************************/
/*  Convenience Functions  */
/* for Creating InfoTables */
/***************************/
/*
twInfoTable_CreateFromPrimitive - Helper function that creates an
InfoTable with a single field, single row with a type specfied by the  
primitive.  twInfoTable does NOT take ownership of the 
twPrimitive pointer.
Parameters:
	name - name to assign the field
	value - pointer to the twPrimitive structure.
Return:
	twInfoTable * - pointer to the resulting twInfoTable or a NULL if an error occurred.
*/
twInfoTable * twInfoTable_CreateFromPrimitive(const char * name, twPrimitive * value);

/*
twInfoTable_CreateFromString - Helper function that creates an
InfoTable with a single field, single row with a type of TW_STRING .  
The twInfoTable does NOT tke ownership of the char pointer if duplicate
is TRUE.
Parameters:
	name - name to assign the field
	value - pointer to the string
	duplicate - copy the string if TRUE, if FALSE the infoTable Takes ownership of the pointer
Return:
	twInfoTable * - pointer to the resulting twInfoTable or a NULL if an error occurred.
*/
twInfoTable * twInfoTable_CreateFromString(const char * name, char * value, char duplicate);

/*
twInfoTable_CreateFromNumber - Helper function that creates an
InfoTable with a single field, single row with a type of TW_NUMBER.  
Parameters:
	name - name to assign the field
	value - the value of the number
Return:
	twInfoTable * - pointer to the resulting twInfoTable or a NULL if an error occurred.
*/
twInfoTable * twInfoTable_CreateFromNumber(const char * name, double value);

/*
twInfoTable_CreateFromNumber - Helper function that creates an
InfoTable with a single field, single row with a type of TW_INTEGER.  
Parameters:
	name - name to assign the field
	value - the value of the integer
Return:
	twInfoTable * - pointer to the resulting twInfoTable or a NULL if an error occurred.
*/
twInfoTable * twInfoTable_CreateFromInteger(const char * name, int32_t value);

/*
twInfoTable_CreateFromLocation - Helper function that creates an
InfoTable with a single field, single row of type TW_LOCATION 
from a location struct.  twInfoTable does NOT tke ownership of the 
twLocation pointer.
Parameters:
	name - name to assign the field
	value - pointer to the location structure.
Return:
	twInfoTable * - pointer to the resulting twInfoTable or a NULL if an error occurred.
*/
twInfoTable * twInfoTable_CreateFromLocation(const char * name, twLocation * value);

/*
twInfoTable_CreateFromBlob - Helper function that creates an
InfoTable with a single field, single row with a type of TW_BLOB or TW_IMAGE.  
The twInfoTable does NOT tke ownership of the char pointer if duplicate
is TRUE.
Parameters:
	name - name to assign the field
	value - pointer to the char array
	length - length of the array
	isImage - set type to TW_IMAGE if TRUE, TW_BLOB or FALSE
	duplicate - copy the string if TRUE, if FALSE the infoTable takes ownership of the pointer
Return:
	twInfoTable * - pointer to the resulting twInfoTable or a NULL if an error occurred.
*/
twInfoTable * twInfoTable_CreateFromBlob(const char * name, char * value, int32_t length, char isImage, char duplicate);

/*
twInfoTable_CreateFromDatetime - Helper function that creates an
InfoTable with a single field, single row of type TW_DATETIME.
Parameters:
	name - name to assign the field
	value - the DATETIME value.
Return:
	twInfoTable * - pointer to the resulting twInfoTable or a NULL if an error occurred.
*/
twInfoTable * twInfoTable_CreateFromDatetime(const char * name, DATETIME value);

/*
twInfoTable_CreateFromBoolean - Helper function that creates an
InfoTable with a single field, single row of type TW_BOOLEAN.
Parameters:
	name - name to assign the field
	value - the char (boolean) value.
Return:
	twInfoTable * - pointer to the resulting twInfoTable or a NULL if an error occurred.
*/
twInfoTable * twInfoTable_CreateFromBoolean(const char * name, char value);

/******************************/
/*    Convenience Functions   */
/* Accessing InfoTable Values */
/******************************/
/*
twInfoTable_GetString - Helper function that retrieves a
string from an InfoTable.
Parameters:
	it - pointer to the infotable to get the value from
	name - name of the field to retrieve
	row - zero based index of the row from which to retrieve the value
	value - pointer to a char *.  Caller will own the resulting pointer.
Return:
	int - zero if success, non-zero if an error occurred.
*/
int twInfoTable_GetString(twInfoTable * it, const char * name, int32_t row, char ** value); 

/*
twInfoTable_GetNumber - Helper function that retrieves a
number from an InfoTable.
Parameters:
	it - pointer to the infotable to get the value from
	name - name of the field to retrieve
	row - zero based index of the row from which to retrieve the value
	value - pointer to a double.  
Return:
	int - zero if success, non-zero if an error occurred.
*/
int twInfoTable_GetNumber(twInfoTable * it, const char * name, int32_t row, double * value);

/*
twInfoTable_GetInteger - Helper function that retrieves an
integer from an InfoTable.
Parameters:
	it - pointer to the infotable to get the value from
	name - name of the field to retrieve
	row - zero based index of the row from which to retrieve the value
	value - pointer to a 32 bit integer.  
Return:
	int - zero if success, non-zero if an error occurred.
*/
int twInfoTable_GetInteger(twInfoTable * it, const char * name, int32_t row, int32_t * value);

/*
twInfoTable_GetLocation - Helper function that retrieves a location structure from an InfoTable.
Parameters:
	it - pointer to the infotable to get the value from
	name - name of the field to retrieve
	row - zero based index of the row from which to retrieve the value
	value - pointer to a location structure.  
Return:
	int - zero if success, non-zero if an error occurred.
*/
int twInfoTable_GetLocation(twInfoTable * it, const char * name, int32_t row, twLocation * value);

/*
twInfoTable_GetBlob - Helper function that retrieves a character array from an InfoTable.
Parameters:
	it - pointer to the infotable to get the value from
	name - name of the field to retrieve
	row - zero based index of the row from which to retrieve the value
	value - pointer to a char array.  Caller becomes owner of the resulting pointer.
	length - pointer to what will contain the length of the array
Return:
	int - zero if success, non-zero if an error occurred.
*/
int twInfoTable_GetBlob(twInfoTable * it, const char * name, int32_t row, char ** value, int32_t * length);

/*
twInfoTable_GetDatetime - Helper function that retrieves a DATETIME from an InfoTable.
Parameters:
	it - pointer to the infotable to get the value from
	name - name of the field to retrieve
	row - zero based index of the row from which to retrieve the value
	value - pointer to a DATETIME.  
Return:
	int - zero if success, non-zero if an error occurred.
*/
int twInfoTable_GetDatetime(twInfoTable * it, const char * name, int32_t row, DATETIME * value);

/*
twInfoTable_GetBoolean - Helper function that retrieves a boolean (char) from an InfoTable.
Parameters:
	it - pointer to the infotable to get the value from
	name - name of the field to retrieve
	row - zero based index of the row from which to retrieve the value
	value - pointer to a char.  
Return:
	int - zero if success, non-zero if an error occurred.
*/
int twInfoTable_GetBoolean(twInfoTable * it, const char * name, int32_t row, char * value);

/*
twInfoTable_GetPrimitive - Helper function that retrieves a primitive from an InfoTable.
Parameters:
	it - pointer to the infotable to get the value from
	name - name of the field to retrieve
	row - zero based index of the row from which to retrieve the value
	value - pointer to primitive pointer.  Caller does NOT own the resulting pointer, so it must not delete it.
Return:
	int - zero if success, non-zero if an error occurred.
*/
int twInfoTable_GetPrimitive(twInfoTable * it, const char * name, int32_t row, twPrimitive ** value);

#ifdef __cplusplus
}
#endif

#endif
