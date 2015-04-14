/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Service definitions & metadata functions
 */


#ifndef SERVICES_H
#define SERVICES_H

#include "twConfig.h"
#include "twOSPort.h"
#include "twLogger.h"
#include "twBaseTypes.h"
#include "twInfoTable.h"

#ifdef __cplusplus
extern "C" {
#endif

/************************/
/* Service Definitions  */
/************************/
typedef struct twServiceDef {
	char * name;
	char * description;
	twDataShape * inputs;
	enum BaseType outputType;
	twDataShape * outputDataShape;
} twServiceDef;

/*
twServiceDef_Create - allocate a new Service Definition structure.  
Parameters:
	name - the name of the service.
	description - description of the service
	inputs - a datashape that describes the service inputs.  See twInfoTable for the twDataShape definition.
	outputType - BaseType of the service result.  enum is defined in twBaseTypes.h
	outputDataShape - a datashape that describes the service output if the output type is an infotable. See twInfoTable for the twDataShape definition.
Return:
	twServiceDef - pointer to the structure that is created.  NULL is returned if the allocation failes.
*/
twServiceDef * twServiceDef_Create(char * name, char * description, twDataShape * inputs, 
								   enum BaseType outputType, twDataShape * outputDataShape);

/*
twServiceDef_Delete - deletes a Service Definition structure.  
Parameters:
	input - pointer to a twServiceDef structure.
Return:
	Nothing.
*/
void twServiceDef_Delete(void * input);

#ifdef __cplusplus
}
#endif

#endif /* SERVICES_H */




