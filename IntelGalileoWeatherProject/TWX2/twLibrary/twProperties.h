/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Property definitions & metadata functions
 */


#ifndef PROPERTIES_H
#define PROPERTIES_H

#include "twConfig.h"
#include "twOSPort.h"
#include "twLogger.h"
#include "twBaseTypes.h"
#include "twInfoTable.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************/
/* Property Definitions */
/************************/
typedef struct twPropertyDef {
	char * name;
	char * description;
	enum BaseType type;
	char * pushType;
	double pushThreshold;
} twPropertyDef;

/*
twPropertyDef_Create - allocate a new Property Definition structure.  
Parameters:
	name - the name of the property.
	type - the BaseType of the property.  See BaseTypes definition in  twDefinitions.h.
	pushType - tells the server the push type of the property.  Can be NEVER, ALWAYS, VALUE (on change).
	pushThreshold - the amount the property has to change (if the type is TW_NUMBER or TW_INTEGER) before pushing the new value.
Return:
	twPropertyDef - pointer to the structure that is created.  NULL is returned if the allocation failes.
*/
twPropertyDef * twPropertyDef_Create(char * name, enum BaseType type, char * description, char * pushType, double pushThreshold);

/*
twPropertyDef_Delete - deletes a Property Definition structure.  
Parameters:
	input - pointer to a twPropertyDef structure.
Return:
	Nothing.
*/
void twPropertyDef_Delete(void * input);

/************************/
/*    Property Values   */
/************************/
typedef struct twProperty {
	char * name;
	twPrimitive * value;
	DATETIME timestamp;
} twProperty;

/*
twProperty_Create - allocated a new Property value structure.  
Parameters:
	name - the name of the property.
	value - the value of the property.  See twBaseTypes for definition of twPrimitive.  The new structure owns this pointer.
	timestamp - the timestamp of the new value.
Return:
	twProperty - pointer to the structure that is created.  NULL is returned if the allocation failes.
*/
twProperty * twProperty_Create(char * name, twPrimitive * value, DATETIME timestamp);

/*
twProperty_Delete - deletes a Property structure.  
Parameters:
	input - pointer to a twProperty structure.
Return:
	Nothing.
*/
void twProperty_Delete(void * input);

#ifdef __cplusplus
}
#endif

#endif /* PROPERTIES_H */




