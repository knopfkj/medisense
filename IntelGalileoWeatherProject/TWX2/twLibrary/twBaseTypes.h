/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Portable ThingWorx BaseTypes
 */

#ifndef BASETYPES_H
#define BASETYPES_H

#include "twOSPort.h"
#include "twDefinitions.h"
#include "twDefaultSettings.h"

#ifdef __cplusplus
extern "C" {
#endif

struct twInfoTable;

/***************************************/
/*   Dynamically allocating byte       */
/*   array.  Automatically expands     */
/*   its length as needed. Usually     */
/*   not used directly by applications */
/***************************************/

typedef struct twStream {
	char * data;
	char * ptr;
	uint32_t length;
	uint32_t maxlength; 
	char ownsData;
} twStream;

twStream * twStream_Create();
twStream * twStream_CreateFromCharArray(const char * data, uint32_t length);  /* COPY - stream will own the data */
twStream * twStream_CreateFromCharArrayZeroCopy(const char * data, uint32_t length); /* No copy - steam doesn't own data */
void twStream_Delete(void* s);
char * twStream_GetData(struct twStream * s);
int32_t twStream_GetIndex(struct twStream * s);
int32_t twStream_GetLength(struct twStream * s);
int twStream_AddBytes(struct twStream * s, void * b, uint32_t count);
int twStream_GetBytes(struct twStream * s, void * b, uint32_t count);
int twStream_Reset(struct twStream * s);

/***************************************/
/*    Location Primitive Structure     */
/***************************************/
typedef struct twLocation {
	double latitude;
	double longitude;
	double elevation;
} twLocation;

/***************************************/
/*     Helper functions that are       */
/*    typically not directly used      */
/*     by application developers       */
/***************************************/
void swap4bytes(char * bytes);
void swap8bytes(char * bytes);
int stringToStream(char * string, twStream * s);
char * streamToString(twStream * s);
enum BaseType baseTypeFromString(const char * s);
const char * baseTypeToString(enum BaseType b);

/***************************************/
/*     BaseType Primitive structure    */
/*       used as a "variant" type      */
/*       throughout the entire API.    */
/***************************************/
typedef struct twPrimitive {
	enum BaseType type;
	enum BaseType typeFamily;
	uint32_t length;
	union {
		int32_t integer;
		double number;
		DATETIME datetime;
		twLocation location;
		char boolean;
		struct {
			char * data;
			int32_t len;
		} bytes;
		struct twInfoTable * infotable;
		struct twPrimitive * variant;
	} val;
} twPrimitive;

/*
twPrimitive_Create - Allocates a new twPrimitive structure.  Defaults
to a type of TW_NOTHING.
Parameters:
	None.  
Return:
	twPrimitive * - pointer to the allocated structure or NULL if error
*/
twPrimitive * twPrimitive_Create();

/*
twPrimitive_Create - Allocates a new twPrimitive structure and populates it
from data in the binary stream.
Parameters:
	s - The bytes stream that contains the primitive in binary format.  
Return:
	twPrimitive * - pointer to the allocated structure or NULL if error
*/
twPrimitive * twPrimitive_CreateFromStream( twStream * s);

/*
twPrimitive_CreateFromStreamTyped - Allocates a new twPrimitive structure and populates it
from data in the binary stream.
Parameters:
	s - The bytes stream that contains the primitive in binary format.  
	type - the type of primitive that the stream contains
Return:
	twPrimitive * - pointer to the allocated structure or NULL if error
*/
twPrimitive * twPrimitive_CreateFromStreamTyped(twStream * s, enum BaseType type);

/*
twPrimitive_ZeroCopy - Allocates a new twPrimitive and takes ownership of any allocated memory
that the primitive that is passed in owned.  The calling function can safely free the original primitive.
Parameters:
	p - pointer to the primitive to be copied
Return:
	twPrimitive * - pointer to the allocated structure or NULL if error
*/
twPrimitive * twPrimitive_ZeroCopy(twPrimitive * p);

/*
twPrimitive_FullCopy - Creates a new primitive that fully copies the original.
The resulting primitive shares nothing with the original.
Parameters:
    p - pointer to the twPrimitive to copy
Return:
	twInfoTable * - pointer to the newly created twPrimitive
*/
twPrimitive * twPrimitive_FullCopy(twPrimitive * p);

/*
twPrimitive_Delete - Frees all memory associated with a primitive.  
Parameters:
	p - pointer the twPrimitive.  
Return:
	Nothing
*/
void twPrimitive_Delete(void * p);

/*
twPrimitive_ToStream - Writes a primitive to a stream.  Typically not used
directly by application developers.
	p - pointer the twPrimitive.  
	s - point to the the twStream to write to
Return:
	int - 0 if successful, non-zero if an error occurred
*/
int twPrimitive_ToStream(twPrimitive * p, twStream * s);

/*
twPrimitive_DecoupleStringAndDelete - Helper function that returns 
the value of a TW_STRING family type, removes it from the primitive 
then deletes the primitive leaving the caller with responsibility 
for freeing the returned char *.
Parameters:
	p - pointer the twPrimitive.  Must be of type family TW_STRING
Return:
	char * - pointer to a null terminated char array.  Not guaranteed to be non-NULL.
*/
char * twPrimitive_DecoupleStringAndDelete(twPrimitive * p);

/*
twPrimitive_Compare - Compares two primitives for equivalence.
	p1 - pointer the twPrimitive to be compared against.  
	p2 - pointer the twPrimitive to compare. 
Return:
	int - 0 if the primitives are identical, 1 if they are not the same, -1 if an error occurred
*/
int twPrimitive_Compare(twPrimitive * p1, twPrimitive * p2);


/**************************/
/* Convenience functions  */
/**************************/
/*
twPrimitive_CreateFromLocation - Helper function that creates a
primitive of type TW_LOCATION from a location struct.  twPrimitive
does NOT take ownership of the twLoaction pointer.
Parameters:
	value - pointer to the location structure.
Return:
	twPrimitive * - pointer to the allocated twPrimitive or a NULL if an error occurred.
*/
twPrimitive * twPrimitive_CreateFromLocation(const twLocation * value);

/*
twPrimitive_CreateFromNumber - Helper function that creates a
primitive of type TW_NUMBER from a double. 
Parameters:
	value - the value that will be assigned to the primitive.
Return:
	twPrimitive * - pointer to the allocated twPrimitive or a NULL if an error occurred.
*/
twPrimitive * twPrimitive_CreateFromNumber(const double value);

/*
twPrimitive_CreateFromInteger - Helper function that creates a
primitive of type TW_INTEGER from an integral type (int, short, char). 
Parameters:
	value - the value that will be assigned to the primitive.
Return:
	twPrimitive * - pointer to the allocated twPrimitive or a NULL if an error occurred.
*/
twPrimitive * twPrimitive_CreateFromInteger(const int32_t value);

/*
twPrimitive_CreateFromDatetime - Helper function that creates a
primitive of type TW_DATETIME from a DATETIME type. 
Parameters:
	value - the value that will be assigned to the primitive.
Return:
	twPrimitive * - pointer to the allocated twPrimitive or a NULL if an error occurred.
*/
twPrimitive * twPrimitive_CreateFromDatetime(const DATETIME value);

/*
twPrimitive_CreateFromCurrentTime - Helper function that gets the current time and creates a
primitive of type TW_DATETIME from it. 
Parameters:
	value - the value that will be assigned to the primitive.
Return:
	twPrimitive * - pointer to the allocated twPrimitive or a NULL if an error occurred.
*/
twPrimitive * twPrimitive_CreateFromCurrentTime();

/*
twPrimitive_CreateFromBoolean - Helper function that creates a
primitive of type TW_BOOLEAN from a char type. 
Parameters:
	value - the value that will be assigned to the primitive.
Return:
	twPrimitive * - pointer to the allocated twPrimitive or a NULL if an error occurred.
*/
twPrimitive * twPrimitive_CreateFromBoolean(const char value);

/*
twPrimitive_CreateFromInfoTable - Helper function that creates a
primitive of type TW_INFOTABLE from a twInfoTable struct. The resulting
primitive does NOT take ownership of the pointer
Parameters:
	value - pointer to the infotable that will be assigned to the primitive.
Return:
	twPrimitive * - pointer to the allocated twPrimitive or a NULL if an error occurred.
*/
twPrimitive * twPrimitive_CreateFromInfoTable(struct twInfoTable * it);

/*
twPrimitive_CreateVariant - Helper function that creates a
primitive of type TW_VARIANT from a twPrimitive struct. The resulting
primitive does NOT take ownership of the pointer
Parameters:
	input - pointer to the twPrimitive who's value will be assigned to the primitive.
Return:
	twPrimitive * - pointer to the allocated twPrimitive or a NULL if an error occurred.
*/
twPrimitive * twPrimitive_CreateVariant(twPrimitive * input);

/* In the following functions, the resulting primitive may become owner of the pointer if duplicate = 0 */

/*
twPrimitive_CreateFromString - Helper function that creates a
primitive of type TW_STRING from a null terminated char array. The resulting
primitive does NOT take ownership of the pointer if duplicate is true.
Parameters:
	value - pointer to the null terminated string that will be assigned to the primitive.
	duplicate - boolean, if TRUE a copy of the string is made, if FALSE the primtive takes ownership of the string
Return:
	twPrimitive * - pointer to the allocated twPrimitive or a NULL if an error occurred.
*/
twPrimitive * twPrimitive_CreateFromString(const char * value, char duplicate);

/*
twPrimitive_CreateFromString - Helper function that creates a
primitive of type TW_BLOB or TW_IMAGE from a char array. The resulting
primitive does NOT take ownership of the pointer if duplicate is true.
Parameters:
	value - pointer to the char array that will be assigned to the primitive.
	length - the length of the char array
	isImage - if TRUE the type is set to TW_IMAGE, if FALSE the type is set to TW_BLOB
	duplicate - boolean, if TRUE a copy of the array is made, if FALSE the primtive takes ownership of the array
Return:
	twPrimitive * - pointer to the allocated twPrimitive or a NULL if an error occurred.
*/
twPrimitive * twPrimitive_CreateFromBlob(const char * value, int32_t length, char isImage, char duplicate);

/*
twPrimitive_CreateFromVariable - Helper function that creates a
primitive of any type type from a void *. This function is not normally
used directly by an application.
Parameters:
	value - pointer to the infotable that will be assigned to the primitive.
	type - the Base type to use for the primitive
	duplicateCharArray - boolean. if the type is a char array, a TRUE will create a copy of the array
	blobLength - if type is TW_BLOB or TW_IMAGE, this is the length of the array
Return:
	twPrimitive * - pointer to the allocated twPrimitive or a NULL if an error occurred.
*/
twPrimitive * twPrimitive_CreateFromVariable(const void * value, enum BaseType type, char duplicateCharArray, uint32_t blobLength);

#ifdef __cplusplus
}
#endif

#endif
