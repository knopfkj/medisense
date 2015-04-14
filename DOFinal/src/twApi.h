/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Portable ThingWorx C SDK API layer
 */


#ifndef TW_API_H
#define TW_API_H

#include "twConfig.h"
#include "twOSPort.h"
#include "twDefinitions.h"
#include "twDefaultSettings.h"
#include "twLogger.h"
#include "twBaseTypes.h"
#include "twMessaging.h"
#include "twInfoTable.h"
#include "twTasker.h"
#include "twProperties.h"
#include "twServices.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************/
/* Callback Function Signatures           */
/******************************************/
/*
property_cb - signature of a callback function that is registered to be called when a specific property request
	is received from the server.
Parameters:
	entityName (Input) - name of the entity (Thing, Resource, etc.) this request is for. Guaranteed not to be NULL.
	propertyName (Input) - name of the property being requested.  If NULL, return all properties.
	value (Input/Output) - pointer to a pointer to an infotable (see twInfoTable.h). value guaranteed to be non-NULL
		*value is not.  If this is a write *value will contain the new value for the property, if this is a read
		the function should create a new instance of an infotable on the heap and return a pointer to it.  The calling
		function will own this pointer and free it when it no longer needs it.
	isWrite (Input) - boolean.  Set to TRUE if this request is a write, FALSE if it is a read.
	userdata (Input) - a generic pointer that was passed in when the callback was registered.
Return:
	msgCodeEnum (see twDefinitions.h) - return TW_OK if the request completes successfully, an appropriate error code if not.
*/
typedef enum msgCodeEnum (*property_cb) (const char * entityName, const char * propertyName,  twInfoTable ** value, char isWrite, void * userdata);

/*
service_cb - signature of a callback function that is registered to be called when a specific service request
	is received from the server.
Parameters:
	entityName (Input) - name of the entity (Thing, Resource, etc.) this request is for. Guaranteed not to be NULL.
	serviceName (Input) - name of the service being requested. 
	params (Input) - pointer to an infotable (see twInfoTable.h). This info table contains all of the input parameters 
		for the service. May be NULL if service has no parameters.
	content (Output) - pointer to a pointer to an infotable (see twInfoTable.h). content guaranteed to be non-NULL
		*content is not.  Function should create a new instance of an infotable on the heap and return a pointer to it.  
		The calling	function will own this pointer and free it when it no longer needs it.
    userdata (Input) - a generic pointer that was passed in when the callback was registered.
Return:
	msgCodeEnum (see twDefinitions.h) - return TW_OK if the request completes successfully, an appropriate error code if not.
*/
typedef enum msgCodeEnum (*service_cb) (const char * entityName, const char * serviceName, twInfoTable * params, twInfoTable ** content, void * userdata);

/*
genericRequest_cb - signature of a callback function that is registered to be called when if the developer wants
	to explicitly parse incoming messages.  These message are guaranteed to be complete, non-multipart messages.
Parameters:
	msg (Input) - a twMessage structure (see twDefinitions.h).
Return:
	twMessage * - pointer to the response message.  User must keep the message ID the same as the request.
*/
typedef twMessage * (*genericRequest_cb)(twMessage * msg);

/*
bindEvent_cb - signature of a callback function that is registered to be called when a bind or unbind completes.  
              This is an indication that the entity that was bound can (bind) or can no longer (unbind) be used by the application.
Parameters:
	entityName (Input) - name of the entity (Thing, Resource, etc.) this request is for. Guaranteed not to be NULL.
	isBound (Input) - indicates if the entity was bound (TRUE) or unbound (FALSE)
    userdata (Input) - a generic pointer that was passed in when the callback was registered.
Return:
	Nothing
*/
typedef void (*bindEvent_cb)(char * entityName, char isBound, void * userdata);

/*******************************************/
/*      API - Structure Definition         */
/*******************************************/
/*
A singleton instance of this structure is automatically created
when the twApi_Initialize function is called.  There should be no
need to manipulate this structure directly.
*/
typedef struct twApi {
	twMessageHandler * mh;
	twList * callbackList;
	twList * boundList;
	twList * bindEventCallbackList;
	genericRequest_cb defaultRequestHandler;
	char autoreconnect;
	int8_t manuallyDisconnected;
	char isAuthenticated;
	uint8_t duty_cycle;
	uint32_t duty_cycle_period;
	TW_MUTEX mtx;
	char offlineMsgEnabled;
	twList * offlineMsgList;
	uint32_t offlineMsgSize;
	char * offlineMsgFile;
	uint32_t ping_rate;
	char handle_pongs;
	uint32_t connect_timeout;
	signed char connect_retries;
} twApi;

/*******************************************/
/*          Lifecycle Functions            */
/*******************************************/
/*
twApi_Initialize - creates the twApi singleton and any dependent structures.
Parameters:
	host (Input) - the hostname of the TW server to connect to.
	port (Input) - the TCP port number used by the TW server.
	resource (Input) - should always be /Thingworx/WS unless changed at the server
	app_key (Input) - the app key defined on the server that this entity should use as an authentication token
	gatewayName  (Input)- an optional name to register with if the application is acting as a gateway for multiple Things.
	messageChunkSize (Input) - the maximum chunk size of a websocket message.  Should match the server.  Default is 8192.
	frameSize (Input) - maximum size of a websocket frame.  Ordinarily matches the messageChunkSize.
	autoreconnect (Input) - boolean.  set to TRUE if it is desired to automatically reconnect to the server if a connection is lost.
Return:
	twApi * - 0 if successful, positive integral error code (see twErrors.h) if an was encountered.
*/
int twApi_Initialize(char * host, uint16_t port, char * resource, char * app_key, char * gatewayName,
						 uint32_t messageChunkSize, uint16_t frameSize, char autoreconnect);

/*
twApi_Delete - shuts down the websocket and frees all associated structures and memory.
Parameters:
	None
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_Delete();

/*
twApi_GetVersion - returns a constant pointer to the version string of the API.
Parameters:
	None
Return:
	char * - The current version of the API as a constant string
*/
char * twApi_GetVersion();

/*******************************************/
/*         Connection Functions            */
/*******************************************/
/*
twApi_Connect - establishes the websocket connection, performs authentication and binds any registered Things.
Parameters:
	timeout (Input) - how long to wait for the websocket to be established (in milliseconds).
	retries (Input) - the number of times to try to connect if the connection fails.
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_Connect(uint32_t timeout, int32_t retries);

/*
twApi_Disconnect - unbinds any bound items and disconnects from the server.
Parameters:
	reason (Input) - the reason for disconnecting.
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_Disconnect(char * reason);

/*
twApi_SetDutyCycle - changes the duty cycle and period of the connection.
Parameters:
	duty_cycle (Input) - the duty cycle of the connection, in percent (1-100). Values over 100 will be set to 100.
	period (Input) - the connecton period (in milliseconds). Value of 0 means AlwaysOn and overrides duty_cycle.
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_SetDutyCycle(uint8_t duty_cycle, uint32_t period);

/*
twApi_SetSelfSignedOk - passthru to notify the TLS library to accept self-signed certificates.
Parameters:
	none
Return:
	Nothing
*/
void twApi_SetSelfSignedOk();

/*
twApi_DisableCertValidation - passthru to notify the TLS library to not validate the server certificate.
Parameters:
	none
Return:
	Nothing
*/
void twApi_DisableCertValidation();

/*
twApi_LoadCACert - loads the local PEM or DER formatted certificate file used to validate the server.
Parameters:
	file (Input) - the full path to the file containing the certificate.
	type (Input) - definition is dependent on the underlying TLS library.  Can be set to 0 for AxTLS.
Return:
	int - 0 if successful, error code if an error was encountered
*/
int	twApi_LoadCACert(const char *file, int type);

/*
twApi_SetPingRate - sets the websockets ping/pong interval.
Parameters:
	rate (Input) - the new ping rate (in milliseconds).  Make sure this aligns with the server idle timeout setting.
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_SetPingRate(uint32_t rate);

/*
twApi_SetConnectTimeout - sets the amount of time (in milliseconds) the websocket waits while attempting a connection.
Parameters:
	timeout (Input) - the new timeout (in milliseconds).  
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_SetConnectTimeout(uint32_t timeout);

/*
twApi_SetConnectRetries - sets the number ofre tries the websocket uses while attempting a connection.
Parameters:
	retires (Input) - the new number of retries.  
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_SetConnectRetries(signed char retries);

/*
twApi_SetProxyInfo - sets the proxy information to be used when making a connection.
Parameters:
	proxyHost (Input) - the host name of the proxy.  
	proxyPort (Input) - the port used by the proxy.  
	proxyUser (Input) - the username to supply to the proxy.  Can be NULL if the porxy does not authenticate.  
	proxyPass (Input) - the password to supply to the proxy.  Can be NULL if the porxy does not authenticate.    
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_SetProxyInfo(char * proxyHost, uint16_t proxyPort, char * proxyUser, char * proxyPass);

/*******************************************/
/*     Connection Status Functions         */
/*******************************************/
/*
twApi_RegisterConnectCallback - register a function that gets called when the web socket connects.
Parameters:
	cb - the function to be called. Function signature can be found in messaging.h.
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_RegisterConnectCallback(eventcb cb);

/*
twApi_RegisterCloseCallback - register a function that gets called when the web socket is disconnected.
Parameters:
	cb - the function to be called. Function signature can be found in messaging.h.
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_RegisterCloseCallback(eventcb cb);

/*
twApi_isConnected - check to see if the websocket is connected.
Parameters:
	none
Return:
	char - TRUE if connected, FALSE if not connected
*/
char twApi_isConnected();

/*******************************************/
/*           Binding Functions             */
/*******************************************/
/*
twApi_BindThing - bind an entity to this connection with the server. If there
	is currently an active connection a bind message is sent.  If not, the bind 
	message will be sent on the next connection.
Parameters:
	entityName - the name of the entity to bind with the server.
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_BindThing(char * entityName);

/*
twApi_UnbindThing - unbind an entity from this connection with the server. If there
	is currently an active connection an unbind message is sent. 
Parameters:
	entityName - the name of the entity to bind with the server.
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_UnbindThing(char * entityName);

/*
twApi_RegisterBindEventCallback - register a function that gets called when an entity is bound or unbound.
Parameters:
	entityName (Input) - used to filer callbacks to a specified entity. A NULL receives all callbacks.
	cb (Input) - the function to be called. Function signature can be found in this file.
	userdata (Input) - an opaque pointer that is passed into the callback function

Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_RegisterBindEventCallback(char * entityName, bindEvent_cb cb, void * userdata);

/*
twApi_UnregisterBindEventCallback - unregister a bindEvent callback function.
Parameters:
	entityName (Input) - used to filer callbacks to a specified entity. A NULL receives all callbacks.
	cb (Input) - the function to be called. Function signature can be found in this file.
	userdata (Input) - an opaque pointer that is passed into the callback function

Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_UnregisterBindEventCallback(char * entityName, bindEvent_cb cb, void * userdata);

/*******************************************/
/*        Operational Functions            */
/*******************************************/
/*
twApi_TaskerFunction - executes all funcntions required for proper
operation of the API. This includes the connection receive loop, duty
cycle control, stale message cleanup, Ping/Pong, etc.  This funciton should
be called ata regular rate every 5 milliseoncds or so depending on your
tolerance for system latency. This function is automatically called by
the tasker
Parameters:
	now - the current timestamp.
	params - required by the tasker function signature but currently not used
Return:
	Nothing
*/
void twApi_TaskerFunction(DATETIME now, void * params);

/*******************************************/
/*     Property and Service Callback       */
/*        Registration Functions           */
/*******************************************/
/*
twApi_RegisterProperty - register a property callback function.  
Parameters:
	entityType - the type of entity that the property belongs to. Enum can be found in twDefinitions.h
	entityName - the name of the entity that the property belongs to.
	propertyName - the name of the property.
	propertyType - the BaseType of the property.  See BaseTypes definition in  twDefinitions.h.
	propertyPushType - tells the server the push type of the property.  Can be NEVER, ALWAYS, VALUE (on change).
	propertyPushThreshold - the amount the property has to change (if the type is TW_NUMBER or TW_INTEGER) before pushing the new value.
	cb - pointer to the property callback function.  Function signature is found in this file.
	userdata (Input) - a generic pointer that is passed back to the callback function when it is invoked.
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_RegisterProperty(enum entityTypeEnum entityType, char * entityName, char * propertyName, enum BaseType propertyType, 
						   char * propertyDescription, char * propertyPushType, double propertyPushThreshold, property_cb cb, void * userdata);


/*
twApi_RegisterService - register a service callback function.  
Parameters:
	entityType - the type of entity that the property belongs to. Enum can be found in twDefinitions.h
	entityName - the name of the entity that the property belongs to.
	serviceName - the name of the service.
	serviceDescription - description of the service
	inputs - a datashape that describes the service inputs.  See twInfoTable for the twDataShape definition.
	outputType - BaseType of the service result.  enum is defined in twBaseTypes.h
	outputDataShape - a datashape that describes the service output if the output type is an infotable. See twInfoTable for the twDataShape definition.
	cb - pointer to the service callback function.  Function signature is found in this file.
	userdata (Input) - a generic pointer that is passed back to the callback function when it is invoked.
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_RegisterService(enum entityTypeEnum entityType, char * entityName, char * serviceName, char * serviceDescription,
						  twDataShape * inputs, enum BaseType outputType, twDataShape * outputDataShape, service_cb cb, void * userdata);

/*
twApi_UnregisterThing - removes all property & service callbacks for an entity.  
Parameters:
	entityName - the name of the entity.
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_UnregisterThing(char * entityName);

/*
twApi_RegisterDefaultRequestHandler - register a service callback function that will get called for all unhandled requests.  
Parameters:
	genericRequest_cb - the pointer to the function to be called. Function signature can be found in this file.
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_RegisterDefaultRequestHandler(genericRequest_cb cb);

/*******************************************/
/*     Server Property/Service/Event       */
/*          Accessor Functions             */
/*******************************************/
#define propertyList twList
/* 
twApi_CreatePropertyList - convenience function to create a list of properties
Parameters:
	name - name of the first property to add to the list
	value - pointer to the primitive containing the first property type and value.  List becomes owner of this pointer.
	timestamp - timestamp of the first property.  If not supplied , the current time will be used.
Return:
	propertyList * - pointer to the created property list.  NULL is an error occurred.
*/
propertyList * twApi_CreatePropertyList(char * name, twPrimitive * value, DATETIME timestamp);

/* 
twApi_DeletePropertyList - convenience function to delete a list of properties
Parameters:
	list - pointer to the list to delete
Return:
	int - zero if successful, non-zero if error occurred
*/
int twApi_DeletePropertyList(propertyList * list);

/* 
twApi_AddPropertyToList - convenience function to add a property to a property list
Parameters:
    proplist - pointer to the list to add the property to
	name - name of the  property to add to the list
	value - pointer to the primitive containing the  property type and value.  List becomes owner of this pointer.
	timestamp - timestamp of the  property.  If not supplied , the current time will be used.
Return:
	int - zero if successful, non-zero if error occurred
*/
int twApi_AddPropertyToList(propertyList * proplist, char * name, twPrimitive * value, DATETIME timestamp);

/*
twApi_ReadProperty - gets the current value of a property from the server.  
Parameters:
	entityType - the type of entity that the property belongs to. Enum can be found in twDefinitions.h
	entityName - the name of the entity that the property belongs to.
	propertyName - name of the property to get
	result - pointer to a twPrimitive pointer (see BaseTypes.h).  Caller is responsible for the returned pointer.
	timeout - time (in milliseconds) to wait for a response from the server. -1 uses DEFAULT_MESSAGE_TIMEOUT
	forceConnect - (boolean) if in the disconnected state of the duty cycle, force a reconnect to send the message
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_ReadProperty(enum entityTypeEnum entityType, char * entityName, char * propertyName, twPrimitive ** result, int32_t timeout, char forceConnect);

/*
twApi_WriteProperty - writes a new value of the property to the server.  
Parameters:
	entityType - the type of entity that the property belongs to. Enum can be found in twDefinitions.h
	entityName - the name of the entity that the property belongs to.
	propertyName - name of the property to get
	value - pointer to a twPrimitive (see BaseTypes.h) that contains the new property value.  Called function will own the pointer and free it.
	timeout - time (in milliseconds) to wait for a response from the server. -1 uses DEFAULT_MESSAGE_TIMEOUT
	forceConnect - (boolean) if in the disconnected state of the duty cycle, force a reconnect to send the message
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_WriteProperty(enum entityTypeEnum entityType, char * entityName, char * propertyName, twPrimitive * value, int32_t timeout, char forceConnect);

/*
twApi_PushProperties - writes a set of values of various properties to the server.  
Parameters:
	entityType - the type of entity that the property belongs to. Enum can be found in twDefinitions.h
	entityName - the name of the entity that the properties belong to.
	properties - a twList (see utils/list.h) of twProperty pointers containing the values of the properties to write.  Caller owns the list pointer and must delete it.
	timeout - time (in milliseconds) to wait for a response from the server. -1 uses DEFAULT_MESSAGE_TIMEOUT
	forceConnect - (boolean) if in the disconnected state of the duty cycle, force a reconnect to send the message
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_PushProperties(enum entityTypeEnum entityType, char * entityName, propertyList * properties, int32_t timeout, char forceConnect);

/*
twApi_InvokeService - invokes a service on the server.  
Parameters:
	entityType - the type of entity that the service belongs to. Enum can be found in twDefinitions.h
	entityName - the name of the entity that the service belongs to.
	serviceName - the name of the service to invoke
	params - a pointer to an infoTable containing the service parameters.  Caller owns this pointer and must delete it.
	result - a pointer to an infoTable pointer that will contain the service response.  Caller owns the returned pointer and must delete it.
	timeout - time (in milliseconds) to wait for a response from the server. -1 uses DEFAULT_MESSAGE_TIMEOUT
	forceConnect - (boolean) if in the disconnected state of the duty cycle, force a reconnect to send the message
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_InvokeService(enum entityTypeEnum entityType, char * entityName, char * serviceName, twInfoTable * params, twInfoTable ** result, int32_t timeout, char forceConnect);

/*
twApi_FireEvent - trigers an event on the server.  
Parameters:
	entityType - the type of entity that the event belongs to. Enum can be found in twDefinitions.h
	entityName - the name of the entity that the event belongs to.
	eventName - the name of the event to trigger
	params - a pointer to an infoTable containing the event data values.  Caller owns this pointer and must delete it.
	timeout - time (in milliseconds) to wait for a response from the server. -1 uses DEFAULT_MESSAGE_TIMEOUT
	forceConnect - (boolean) if in the disconnected state of the duty cycle, force a reconnect to send the message
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered
*/
int twApi_FireEvent(enum entityTypeEnum entityType, char * entityName, char * eventName, twInfoTable * params, int32_t timeout, char forceConnect);

/*******************************************/
/*          Keep Alive Functions           */
/*******************************************/
/* Only use these functions if you want to override default keep alive handling */
/*
twApi_SendPing - sends a Ping to the server.  
Parameters:
	content - string to send with the ping.  Must be less than 128 characters.
Return:
	int - the result of the call.  A 0 means success, positive integral error code (see twErrors.h) if an was encountered.
*/
int twApi_SendPing(char * content);

/*
twApi_RegisterPingCallback - registers a callback function to be called when a Ping is received.  
Parameters:
	cb - pointer to the function to call when a Ping is received. Function signature can be found in twMessaging.h.
Return:
	int - the result of the call.  A 0 means success, positive integral error code (see twErrors.h) if an was encountered.
*/
int twApi_RegisterPingCallback(eventcb cb);

/*
twApi_RegisterPongCallback - registers a callback function to be called when a Pong is received.  
Parameters:
	cb - pointer to the function to call when a Pong is received. Function signature can be found in twMessaging.h.
Return:
	int - the result of the call.  A 0 means success, positive integral error code (see twErrors.h) if an was encountered.
*/
int twApi_RegisterPongCallback(eventcb cb);

/*******************************************/
/*            Tasker Functions             */
/*******************************************/
/*
twApi_CreateTask - adds a new task to the round robin scheduler.  
Parameters:
	runTimeIntervalMsec - Time (in milliseconds) to wait between calls to the task function specified.
	func - pointer to the function to call.  Function signature can be found in utils/twTasker.h.
Return:
	int - the result of the call.  A 0 means success, positive integral error code (see twErrors.h) if an was encountered.
*/
int twApi_CreateTask(uint32_t runTimeIntervalMsec, twTaskFunction func);

#ifdef __cplusplus
}
#endif

#endif /* TW_API_H */
