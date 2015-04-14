/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Portable ThingWorx C SDK API layer
 */

#include "twConfig.h"
#include "twOSPort.h"
#include "twLogger.h"
#include "twMessages.h"
#include "twMessaging.h"
#include "twApi.h"
#include "stringUtils.h"
#include "list.h"
#include "twTls.h"
#include "twTasker.h"
#include "twProperties.h"
#include "twServices.h"
#include "jsonUtils.h"
#include "twVersion.h"
#ifdef ENABLE_FILE_XFER
#include "twFileManager.h"
#endif

/* Singleton API structure */
twApi * tw_api = NULL;

/* Global mutex used during intitialization */
TW_MUTEX twInitMutex;

/* Callback data structure */
extern void * fileXferCallback;

typedef struct callbackInfo {
	enum entityTypeEnum entityType;
	char * entityName;
	enum characteristicEnum characteristicType;
	char * charateristicName;
	void * charateristicDefinition;
	void * cb;
	void * userdata;
} callbackInfo;

void deleteCallbackInfo(void * info) {
	if (info) {
		callbackInfo * tmp = (callbackInfo *)info;
		if (tmp->charateristicDefinition) {
			if (tmp->characteristicType == TW_PROPERTIES) twPropertyDef_Delete(tmp->charateristicDefinition);
			else if (tmp->characteristicType == TW_SERVICES) twServiceDef_Delete(tmp->charateristicDefinition);
		}
		TW_FREE(tmp->entityName);
		/* TW_FREE(tmp->charateristicName); This just points to the name in the definition */
		TW_FREE(tmp);
	}
}

/****************************************/
/** Helper functions **/
int convertMsgCodeToErrorCode(enum msgCodeEnum code) {
	int err;
	switch (code) {
		case TWX_SUCCESS:
			err = TW_OK;
			break;
		case TWX_BAD_REQUEST:
			err = TW_BAD_REQUEST;
			break;
		case TWX_UNAUTHORIZED:
			err = TW_UNAUTHORIZED;
			break;
		case TWX_BAD_OPTION:
			err = TW_ERROR_BAD_OPTION;
			break;
		case TWX_FORBIDDEN:
			err = TW_FORBIDDEN;
			break;
		case TWX_NOT_FOUND:
			err = TW_NOT_FOUND;
			break;
		case TWX_METHOD_NOT_ALLOWED:
			err = TW_METHOD_NOT_ALLOWED;
			break;
		case TWX_NOT_ACCEPTABLE:
			err = TW_NOT_ACCEPTABLE;
			break;
		case TWX_PRECONDITION_FAILED:
			err = TW_PRECONDITION_FAILED;
			break;
		case TWX_ENTITY_TOO_LARGE:
			err = TW_ENTITY_TOO_LARGE;
			break;
		case TWX_UNSUPPORTED_CONTENT_FORMAT:
			err = TW_UNSUPPORTED_CONTENT_FORMAT;
			break;
		case TWX_INTERNAL_SERVER_ERROR:
			err = TW_INTERNAL_SERVER_ERROR;
			break;
		case TWX_NOT_IMPLEMENTED:
			err = TW_NOT_IMPLEMENTED;
			break;
		case TWX_BAD_GATEWAY:
			err = TW_BAD_GATEWAY;
			break;
		case TWX_SERVICE_UNAVAILABLE:
			err = TW_SERVICE_UNAVAILABLE;
			break;
		case TWX_GATEWAY_TIMEOUT:
			err = TW_GATEWAY_TIMEOUT;
			break;
		default:
			err = TW_UNKNOWN_ERROR;
			break;
	}
	return err;
}

void * getCallbackFromList(twList * list, enum entityTypeEnum entityType, char * entityName, 
						   enum characteristicEnum charateristicType, char * characteristicName, void ** userdata) {
	/* Get based on entity/characteristic pair */
	ListEntry * le = NULL;
	if (!list || !entityName || !characteristicName) {
		TW_LOG(TW_ERROR, "isEntyInList: NULL input parameter found");
		return 0;
	}
	le = twList_Next(list, NULL);
	while (le) {
		if (le->value) {
			callbackInfo * tmp = (callbackInfo *)(le->value);
			if (tmp->entityType != entityType || tmp->characteristicType != charateristicType || 
				strcmp(entityName, tmp->entityName) || strcmp(characteristicName, tmp->charateristicName)) {
				le = twList_Next(list, le);
				continue;
			}
			/* Return the callback */
			*userdata = tmp->userdata;
			return tmp->cb;
		}
	}
	/* If file transfer is enabled let that system handle any file transfer services */
	#ifdef ENABLE_FILE_XFER
	if (charateristicType == TW_SERVICES) {
		int i = 0;
		while (strcmp(fileXferServices[i],"SENTINEL")) {
			if (!strcmp(fileXferServices[i++],characteristicName)) {
				/* This is a file transfer service */
				return fileXferCallback;
			}
		}
	}
	#endif
	return 0;
}

enum msgCodeEnum sendMessageBlocking(twMessage * msg, int32_t timeout, twInfoTable ** result) {
	DATETIME expirationTime, now;
	if (!tw_api || !tw_api->mh || !msg) {
		TW_LOG(TW_ERROR, "api:sendMessageBlocking: NULL api, msg or message handler pointer found");
		return TWX_UNKNOWN;
	}
	expirationTime = twGetSystemMillisecondCount();
	expirationTime = twAddMilliseconds(expirationTime, timeout);
	/* Register the response before we send to prevent a race condition */
	twMessageHandler_RegisterResponseCallback(tw_api->mh, 0, msg->requestId, expirationTime);
	/* Send the message and wait for it to be received */
	if (twMessage_Send(msg, tw_api->mh->ws) == TW_OK) {
		twResponseCallbackStruct * cb = 0;
		now = twGetSystemMillisecondCount();
		while (twTimeLessThan(now, expirationTime)) {
			if (twWs_Receive(tw_api->mh->ws, 5)) {
				TW_LOG(TW_WARN,"api:sendMessageBlocking: Receive failed.");
				break;
			}
			twMessageHandler_msgHandlerTask(now, NULL);
			cb = twMessageHandler_GetCompletedResponseStruct(tw_api->mh, msg->requestId);
			if (cb) break;
			now = twGetSystemMillisecondCount();
		}
		if (!cb) {
			TW_LOG(TW_WARN,"api:sendMessageBlocking: Message %d timed out", msg->requestId);
			twMessageHandler_CleanupOldMessages(tw_api->mh);
			return TWX_GATEWAY_TIMEOUT;
		} else {
			enum msgCodeEnum code = cb->code;
			TW_LOG(TW_WARN,"api:sendMessageBlocking: Received Response to Message %d.  Code: %d", msg->requestId, code);
			if (result) *result = cb->content;
			/* If this was an auth request we need to grab the session ID */
			if (msg->type == TW_AUTH) {
				if (code == TWX_SUCCESS) { 
					TW_LOG(TW_TRACE,"api:sendMessageBlocking: AUTH Message %d succeeded", msg->requestId, code);
					tw_api->mh->ws->sessionId = cb->sessionId;
				} else {
					TW_LOG(TW_WARN,"api:sendMessageBlocking: AUTH Message %d failed.  Code:", msg->requestId, code);
				}
			}
			twMessageHandler_UnegisterResponseCallback(tw_api->mh, msg->requestId);
			return code;
		}
	} else {
		/* Message failed to send - remove it from the response callback list */
		twMessageHandler_UnegisterResponseCallback(tw_api->mh, msg->requestId);
		return TWX_PRECONDITION_FAILED;
	}
}

enum msgCodeEnum makeRequest(enum msgCodeEnum method, enum entityTypeEnum entityType, char * entityName, 
	                         enum characteristicEnum characteristicType, char * characteristicName, 
							 twInfoTable * params, twInfoTable ** result, int32_t timeout, char forceConnect) {
	enum msgCodeEnum res = TWX_PRECONDITION_FAILED;
	twMessage * msg = NULL;
	if (!tw_api || !entityName || !characteristicName || !result) {
		TW_LOG(TW_ERROR, "api:makeRequest: NULL tw_api, entityName, charateristicName or result pointer");
		return res;
	}
	/* Check to see if we are offline and should attempt to reconnect */
	if (!twApi_isConnected()) {
		if (forceConnect) {
			if (twApi_Connect(CONNECT_TIMEOUT, CONNECT_RETRIES)) {
				TW_LOG(TW_ERROR, "api:makeRequest: Error trying to force a reconnect");
				return TWX_SERVICE_UNAVAILABLE;
			}
		} else if (!tw_api->offlineMsgEnabled) {
			TW_LOG(TW_INFO, "api:makeRequest: Currently offline and 'forceConnect' is FALSE");
			return TWX_SERVICE_UNAVAILABLE;
		}
	}
	/* Create the Request message */
	msg = twMessage_CreateRequestMsg(method);
	if (!msg) {
		TW_LOG(TW_ERROR, "api:makeRequest: Error creating request message");
		return res;
	}
	if (timeout < 0) timeout = DEFAULT_MESSAGE_TIMEOUT;
	/* Set the body of the message */
	twRequestBody_SetEntity((twRequestBody *)(msg->body), entityType, entityName);
	twRequestBody_SetCharateristic((twRequestBody *)(msg->body), characteristicType, characteristicName);
	twRequestBody_SetParams((twRequestBody *)(msg->body), twInfoTable_ZeroCopy(params));
	twMutex_Lock(tw_api->mtx);
	res = sendMessageBlocking(msg, timeout, result);
	twMutex_Unlock(tw_api->mtx);
	if (msg) twMessage_Delete(msg);
	return res;
}

enum msgCodeEnum makePropertyRequest(enum msgCodeEnum method, enum entityTypeEnum entityType, char * entityName, 
	                         char * propertyName, twPrimitive * value, twPrimitive ** result, int32_t timeout, char forceConnect) {
	twInfoTable * value_it = NULL;
	twInfoTable * result_it = NULL;
	twPrimitive * tmp = NULL;
	enum msgCodeEnum res;
	if (!result) return TWX_PRECONDITION_FAILED;
	if (value) {
		value_it = twInfoTable_CreateFromPrimitive(propertyName, twPrimitive_ZeroCopy(value));
	}
	res = makeRequest(method, entityType, entityName, TW_PROPERTIES, propertyName, value_it, &result_it, timeout, forceConnect);
	twInfoTable_Delete(value_it);
	if (result_it) {
		twInfoTable_GetPrimitive(result_it, propertyName, 0, &tmp);
		*result = twPrimitive_ZeroCopy(tmp);
		twInfoTable_Delete(result_it);
	}
	return res;
}

int twApi_SendResponse(twMessage * msg) {
	if (tw_api && tw_api->mh && tw_api->mh->ws) return twMessage_Send(msg, tw_api->mh->ws);
	return TW_ERROR_SENDING_RESP;
}

int api_requesthandler(struct twWs * ws, struct twMessage * msg) {
	/*
	Need to look this up to see if we have a property or service callback registered.
	If not we check to see if there is a generic callback
	*/
	int res = TW_UNKNOWN_ERROR;
	twRequestBody * b = NULL;
	twMessage * resp = NULL;
	enum msgCodeEnum respCode = TWX_UNKNOWN;
	twInfoTable * result = NULL;
	void * cb = NULL;
	void * userdata = NULL;
	if (!msg || !tw_api) {
		TW_LOG(TW_ERROR,"api_requesthandler: Null msg pointer");
		return TW_INVALID_PARAM;
	}
	if (msg->type != TW_REQUEST) {
		TW_LOG(TW_ERROR,"api_requesthandler: Non Request message received");
		return TW_INVALID_MSG_TYPE;
	}
	b = (twRequestBody *) (msg->body);
	if (!b || !b->entityName) {
		TW_LOG(TW_ERROR,"api_requesthandler: No valid message body found");
		return TW_INVALID_MSG_BODY;
	}
	cb = getCallbackFromList(tw_api->callbackList, b->entityType, b->entityName, b->characteristicType, b->characteristicName, &userdata);
	if (cb) {
		switch (b->characteristicType) {
			case TW_PROPERTIES:
				{
					property_cb callback = (property_cb)cb;
					if (msg->code == TWX_PUT) {
						/*  This is a property write */
						if (!b->params) {
							TW_LOG(TW_ERROR,"api_requesthandler: Missing param in PUT message");
							return TW_INVALID_MSG_PARAMS;
						}
						respCode = callback(b->entityName, b->characteristicName, &b->params, TRUE, userdata);
						resp = twMessage_CreateResponseMsg(respCode, msg->requestId);
					} else {
						/* This is a read */
						respCode = callback(b->entityName, b->characteristicName, &result, FALSE, userdata);
						resp = twMessage_CreateResponseMsg(respCode, msg->requestId);
						if (resp && result) twResponseBody_SetContent((twResponseBody *)(resp->body), result);
					}
					break;
				}
			case TW_SERVICES:
				{
					service_cb callback = (service_cb)cb;
					respCode = callback(b->entityName, b->characteristicName, b->params, &result, userdata);
					resp = twMessage_CreateResponseMsg(respCode, msg->requestId);
					if (resp && result) twResponseBody_SetContent((twResponseBody *)(resp->body), result);
					break;
				}
			default:
				/* Try our generic message handler */
				if (tw_api->defaultRequestHandler) {
					resp = tw_api->defaultRequestHandler(msg);
				} else {
					/* No handler - return a 404 */
					resp = twMessage_CreateResponseMsg(TWX_NOT_FOUND, msg->requestId);
					if (!resp) TW_LOG(TW_ERROR,"api_requesthandler: Error allocating response message");
				}
		}
	} else {
		/* Try our generic message handler */
		if (tw_api->defaultRequestHandler) {
			resp = tw_api->defaultRequestHandler(msg);
		} else {
			/* No handler - return a 404 */
			TW_LOG(TW_INFO,"api_requesthandler: No handler found.  Returning 404");
			resp = twMessage_CreateResponseMsg(TWX_NOT_FOUND, msg->requestId);
			if (!resp) TW_LOG(TW_ERROR,"api_requesthandler: Error allocating response message");
		}
	}
	/* Send our response */
	if (resp){
		res = twApi_SendResponse(resp);
		twMessage_Delete(resp);
		return res;
	}
	return TW_INVALID_RESP_MSG;
}


enum msgCodeEnum getMetadataService(const char * entityName, const char * serviceName, twInfoTable * params, twInfoTable ** content, void * userdata) {
	ListEntry * le = NULL;
	twStream * propJson = NULL;
	twStream * svcJson = NULL;
	propJson = twStream_Create();
	svcJson = twStream_Create();
	TW_LOG(TW_TRACE,"getMetadataService - Function called");
	if (!content || !propJson || !svcJson ||!tw_api || !tw_api->callbackList || !entityName) {
		TW_LOG(TW_ERROR,"getMetadataService - NULL stream,callback, params or content pointer");
		twStream_Delete(propJson);
		twStream_Delete(svcJson);
		return TWX_BAD_REQUEST;
	}
	/* Prep the JSON string */
	addStringsToStream(propJson, "{\"name\":\"", entityName, "\",\"description\":\"\",\"isSystemObject\":false,\"propertyDefinitions\":{", NULL);
	le = twList_Next(tw_api->callbackList, NULL);
	while (le) {
		if (le->value) {
			callbackInfo * tmp = (callbackInfo *)(le->value);
			if (strcmp(entityName, tmp->entityName) == 0) {
				/* If this is a property add it to the stream */
				if (tmp->charateristicDefinition) {
					if (tmp->characteristicType == TW_PROPERTIES) {
						addPropertyDefJsonToStream((twPropertyDef *)tmp->charateristicDefinition, propJson);
						le = twList_Next(tw_api->callbackList, le);
						if (le) addStringsToStream(propJson, ",",NULL);
					} else if (tmp->characteristicType == TW_SERVICES) {
						addServiceDefJsonToStream((twServiceDef *)tmp->charateristicDefinition, svcJson);
						le = twList_Next(tw_api->callbackList, le);
						if (le) addStringsToStream(svcJson, ",",NULL);
					} else {
                            // nothing of interest, move on
                            le = twList_Next( tw_api->callbackList, le ) ;
					} 
				} else {
                        // No charateristic definition, move on
                        le = twList_Next( tw_api->callbackList, le ) ;
                }
            } else {
                    // not the value we are looking for? NEXT!
                    le = twList_Next( tw_api->callbackList, le ) ;
            }
        } else {
                // no eternal loops when empty value please
                le = twList_Next( tw_api->callbackList, le ) ;
        }
	}
	/* need to remove any trailing commas from the stream */
	if (propJson->data[propJson->length-1] == ',') {
		propJson->data[propJson->length-1] = 0x00;
		propJson->length--;
		propJson->ptr--;
	}
	if (svcJson->data[svcJson->length-1] == ',') {
		svcJson->data[svcJson->length-1] = 0x00;
		svcJson->length--;
	}
	/* Combine the two streams and terminate the JSON */
	addStringsToStream(propJson, "},\"serviceDefinitions\":{",NULL);
	twStream_AddBytes(propJson, svcJson->data, svcJson->length);
	addStringsToStream(propJson, "}}",NULL);
	/* Create the result infotable */
	*content = twInfoTable_CreateFromPrimitive("result", twPrimitive_CreateFromVariable(propJson->data, TW_JSON, TRUE, 0));
	twStream_Delete(propJson);
	twStream_Delete(svcJson);
	if (*content) {
		return TWX_SUCCESS;
	}
	return TWX_INTERNAL_SERVER_ERROR;
}

char receivedPong = FALSE;
int pong_handler (struct twWs * ws, const char * data, size_t length) {
	receivedPong = TRUE;
	return 0;
}

/*********************************************************/
/* API Functions */
int twApi_Initialize(char * host, uint16_t port, char * resource, char * app_key, char * gatewayName,
						 uint32_t messageChunkSize, uint16_t frameSize, char autoreconnect) {
	int err = TW_UNKNOWN_ERROR;
    twWs * ws = NULL;
	/* Validate all inputs */
	if (!host || port == 0 || !resource || !app_key || messageChunkSize < frameSize) {
		TW_LOG(TW_ERROR, "twApi_Initialize: Invalid parameter found");
		return TW_INVALID_PARAM;
	}
	/* Create our global initialization mutex */
	twInitMutex = twMutex_Create();
	if (!twInitMutex) {
		TW_LOG(TW_ERROR, "twApi_Initialize: Error creating initialization mutex");
		return TW_ERROR_CREATING_MTX;
	}
	/* Create the websocket */
	err = twWs_Create(host, port, resource, app_key, gatewayName, messageChunkSize, frameSize, &ws);
	if (err) {
		TW_LOG(TW_ERROR, "twApi_Initialize: Error creating websocket structure");
		return err;
	}
	/* Allocate space for the structure */
	tw_api = (twApi *)TW_CALLOC(sizeof(twApi), 1);
	if (!tw_api) {
		TW_LOG(TW_ERROR, "twApi_Initialize: Error allocating api structure");
		return TW_ERROR_ALLOCATING_MEMORY;
	}
	/* Initialize the message handler, mutex, and lists */
	tw_api->mh = twMessageHandler_Instance(ws);
	tw_api->mtx = twMutex_Create();
	tw_api->callbackList = twList_Create(deleteCallbackInfo);
	tw_api->bindEventCallbackList = twList_Create(deleteCallbackInfo);
	tw_api->boundList = twList_Create(0);
	if (!tw_api->mh || !tw_api->mtx || !tw_api->callbackList || !tw_api->boundList || !tw_api->bindEventCallbackList) {
		TW_LOG(TW_ERROR, "twApi_Initialize: Error initializing api");
		twApi_Delete();
		return TW_ERROR_INITIALIZING_API;
	}
	twMessageHandler_RegisterDefaultRequestCallback(tw_api->mh, api_requesthandler);
	tw_api->autoreconnect = autoreconnect;
	tw_api->manuallyDisconnected = TRUE;
	tw_api->isAuthenticated = FALSE;
	tw_api->defaultRequestHandler = NULL;
	twApi_SetDutyCycle(DUTY_CYCLE, DUTY_CYCLE_PERIOD);
	/* Set up our ping/pong handling */
	tw_api->ping_rate = PING_RATE;
	tw_api->handle_pongs = TRUE;
	twMessageHandler_RegisterPongCallback(tw_api->mh, pong_handler);

#ifdef OFFLINE_MSG_STORE
	tw_api->offlineMsgEnabled = TRUE;
#if (OFFLINE_MSG_STORE == 1) 
	tw_api->offlineMsgList = twList_Create(twStream_Delete);
	if (!tw_api->offlineMsgList) {
		TW_LOG(TW_ERROR, "twApi_Initialize: Error allocating offline message store queue");
	}
#endif
#if (OFFLINE_MSG_STORE == 2)
	if (twDirectory_CreateDirectory(OFFLINE_MSG_STORE_DIR)) {
		TW_LOG(TW_ERROR, "twApi_Initialize: Error creating offline message directory %s",OFFLINE_MSG_STORE_DIR);
	} else {
		tw_api->offlineMsgFile = (char *)TW_CALLOC(strlen(OFFLINE_MSG_STORE_DIR) + strlen("offline_msgs.bin") + 2, 1);
		if (tw_api->offlineMsgFile) {
			uint64_t size = 0;
			DATETIME lastModified;
			char isDir;
			char isReadOnly;
			strcpy(tw_api->offlineMsgFile, OFFLINE_MSG_STORE_DIR);
			strcat(tw_api->offlineMsgFile,"/");
			strcat(tw_api->offlineMsgFile,"offline_msgs.bin");
			/* Get the size of the file in case there are some persisted messages */
			twDirectory_GetFileInfo(tw_api->offlineMsgFile, &size, &lastModified, &isDir, &isReadOnly);
			tw_api->offlineMsgSize = size;
		} else TW_LOG(TW_ERROR, "twApi_Initialize: Error allocating offline message file name");
	}
#endif
#endif

#ifdef ENABLE_TASKER
	/* Initalize our tasker */
	twTasker_CreateTask(5, &twApi_TaskerFunction);
	twTasker_Start();
#endif
	return TW_OK;
}
	
int twApi_Delete() {
	twApi * tmp = tw_api;
	if (!tw_api) return TW_OK;
	twApi_Disconnect("Shutting down");
	/* Stop the tasker */
	twTasker_Stop();
	/* Set the singleton to NULL so no one else uses it */
	twMutex_Lock(twInitMutex);
	twMutex_Lock(tmp->mtx);	
	tw_api = NULL;

	if (tmp->mh) twMessageHandler_Delete(NULL);
	if (tmp->callbackList) twList_Delete(tmp->callbackList);
	if (tmp->bindEventCallbackList) twList_Delete(tmp->bindEventCallbackList);
	if (tmp->boundList) twList_Delete(tmp->boundList);
	if (tmp->offlineMsgList) twList_Delete(tmp->offlineMsgList);
	if (tmp->offlineMsgFile) TW_FREE(tmp->offlineMsgFile);
    twMutex_Unlock(tmp->mtx);
	twMutex_Delete(tmp->mtx);
	twMutex_Unlock(twInitMutex);
	twMutex_Delete(twInitMutex);
	TW_FREE(tmp);
	return TW_OK;
}

int twApi_SetProxyInfo(char * proxyHost, uint16_t proxyPort, char * proxyUser, char * proxyPass) {
	if (!tw_api || !tw_api->mh || !tw_api->mh->ws || !tw_api->mh->ws->connection || !tw_api->mh->ws->connection->connection) return TW_SOCKET_INIT_ERROR;
	return twSocket_SetProxyInfo(tw_api->mh->ws->connection->connection, proxyHost, proxyPort, proxyUser, proxyPass);
}

char * twApi_GetVersion() {
	return C_SDK_VERSION;
}

int twApi_BindAll(char unbind) {
	int res = TW_OK;
	twMessage * msg = NULL;
	ListEntry * le = NULL;
	ListEntry *cb_le = NULL;
	if (!tw_api || !tw_api->mh || !tw_api->mh->ws || !tw_api->boundList) return TW_INVALID_PARAM;
	twMutex_Lock(tw_api->mtx);
	le = twList_Next(tw_api->boundList, NULL);
	if (le && le->value) {
		msg = twMessage_CreateBindMsg((char *) le->value, unbind);
		while (le && le->value) {
			le = twList_Next(tw_api->boundList, le);
			if (le && le->value) twBindBody_AddName((twBindBody *)(msg->body), (char *)le->value);
		}
		res = convertMsgCodeToErrorCode(sendMessageBlocking(msg, DEFAULT_MESSAGE_TIMEOUT, NULL));
	}
	twMutex_Unlock(tw_api->mtx);
	if (msg) twMessage_Delete(msg);
	/* Look for any callbacks */
	cb_le = twList_Next(tw_api->bindEventCallbackList, NULL);
	while (cb_le && cb_le->value) {
		callbackInfo * info = (callbackInfo *)cb_le->value;
		/* Inner loop to see if we bount this one */
		le = twList_Next(tw_api->boundList, NULL);
		while (le && le->value) {
			if (!info->entityName || !strcmp(info->entityName, (char *) le->value)) {
				bindEvent_cb cb = (bindEvent_cb)info->cb;
				cb((char *) le->value, !unbind, info->userdata);
			}
			le = twList_Next(tw_api->boundList, le);
		}
		cb_le = twList_Next(tw_api->bindEventCallbackList, cb_le);
	}
	return res;
}

int twApi_Authenticate() {
	int res = TW_UNKNOWN_ERROR;
	twMessage * msg = NULL;
	if (!tw_api || !tw_api->mh || !tw_api->mh->ws) return TW_INVALID_PARAM;
	twMutex_Lock(tw_api->mtx);
	msg = twMessage_CreateAuthMsg("appKey", tw_api->mh->ws->api_key);
	res = convertMsgCodeToErrorCode(sendMessageBlocking(msg, DEFAULT_MESSAGE_TIMEOUT, NULL));
	if (res == TW_OK) tw_api->isAuthenticated = TRUE;
	twMutex_Unlock(tw_api->mtx);
	if (msg) twMessage_Delete(msg);
	return res;
}

int twApi_Connect(uint32_t timeout, int32_t retries) {
	int res = TW_UNKNOWN_ERROR;
	if (!tw_api) return TW_NULL_API_SINGLETON;
	tw_api->connect_timeout = timeout;
	tw_api->connect_retries = retries;
	if (tw_api->mh && tw_api->mh->ws) {
		while (retries != 0) {
			twMutex_Lock(tw_api->mtx);
			res = twWs_Connect(tw_api->mh->ws, timeout);
			if (res == TW_OK) tw_api->manuallyDisconnected = FALSE;
			twMutex_Unlock(tw_api->mtx);
			if (!res) res = twApi_Authenticate();
			if (!res) res = twApi_BindAll(FALSE);
			if (!res) break;
			if (retries != -1) retries--;
			twSleepMsec(5000);
		}
	}
	return res;
}

int twApi_Disconnect(char * reason) {
	int res = TW_UNKNOWN_ERROR;
	if (!tw_api) return TW_NULL_API_SINGLETON;
	twApi_BindAll(TRUE);
	twMutex_Lock(tw_api->mtx);
	if (tw_api->mh){
		tw_api->manuallyDisconnected = TRUE;
		tw_api->isAuthenticated = FALSE;
		res = twWs_Disconnect(tw_api->mh->ws, NORMAL_CLOSE, reason);
	}
	
	twMutex_Unlock(tw_api->mtx);
	return res;
}

char twApi_isConnected() {
	if (tw_api && tw_api->mh && tw_api->mh->ws) return twWs_IsConnected(tw_api->mh->ws) && tw_api->isAuthenticated;
	return FALSE;
}


int twApi_SetDutyCycle(uint8_t duty_cycle, uint32_t period) {
	if (!tw_api) return TW_NULL_API_SINGLETON;
	if (duty_cycle > 100) duty_cycle = 100;
	tw_api->duty_cycle = duty_cycle;
	tw_api->duty_cycle_period = period;
	return TW_OK;
}

int twApi_SetPingRate(uint32_t rate) {
	if (!tw_api) return TW_NULL_API_SINGLETON;
	tw_api->ping_rate = rate;
	return TW_OK;
}

int twApi_SetConnectTimeout(uint32_t timeout) {
	if (!tw_api) return TW_NULL_API_SINGLETON;
	tw_api->connect_timeout = timeout;
	return TW_OK;
}

int twApi_SetConnectRetries(signed char retries) {
	if (!tw_api) return TW_NULL_API_SINGLETON;
	tw_api->connect_retries = retries;
	return TW_OK;
}


int twApi_BindThing(char * entityName) {
	int res = TW_UNKNOWN_ERROR;
	ListEntry * le = NULL;
	enum msgCodeEnum resp;
	twMessage * msg = NULL;
	if (!tw_api || !entityName || !tw_api->bindEventCallbackList) {
		TW_LOG(TW_ERROR, "twApi_BindThing: NULL tw_api or entityName");
		return TW_INVALID_PARAM;
	}
	/* Add it to the list */
	twList_Add(tw_api->boundList, duplicateString(entityName));
	/* Register our metadata service handler */
	twApi_RegisterService(TW_THING, entityName, "GetMetadata", NULL, NULL, TW_JSON, NULL, getMetadataService, NULL);
	/* If we are not connected, we are done */
	if (!twApi_isConnected()) return TW_OK;
	/* Create the bind message */
	msg = twMessage_CreateBindMsg(entityName, FALSE);
	if (!msg) {
		TW_LOG(TW_ERROR, "twApi_BindThing: Error creating Bind message");
		return TW_ERROR_CREATING_MSG;
	}
	twMutex_Lock(tw_api->mtx);
	resp = sendMessageBlocking(msg, 10000, NULL);
	twMutex_Unlock(tw_api->mtx);
	res = convertMsgCodeToErrorCode(resp);
	if (res != TW_OK) TW_LOG(TW_ERROR, "twApi_BindThing: Error sending Bind message");
	if (msg) twMessage_Delete(msg);
	/* Look for any callbacks */
	le = twList_Next(tw_api->bindEventCallbackList, NULL);
	while (le && le->value) {
		callbackInfo * info = (callbackInfo *)le->value;
		if (!info->entityName || !strcmp(info->entityName, entityName)) {
			bindEvent_cb cb = (bindEvent_cb)info->cb;
			cb(entityName, TRUE, info->userdata);
		}
		le = twList_Next(tw_api->bindEventCallbackList, le);
	}
	return res;
}

int twApi_UnbindThing(char * entityName) {
	int res = TW_UNKNOWN_ERROR;
	twMessage * msg = NULL;
	enum msgCodeEnum resp;
	ListEntry * le = NULL;
	if (!tw_api || !entityName || !tw_api->boundList) {
		TW_LOG(TW_ERROR, "twApi_UnbindThing: NULL tw_api or entityName");
		return TW_INVALID_PARAM;
	}
	/* Unregister all call backs for this entity */
    if (twApi_UnregisterThing(entityName));
	/* Remove it from the Bind list */
	le = twList_Next(tw_api->boundList, NULL);
	while (le) {
		if (le->value && !strcmp(entityName, (char *)le->value)) {
			twList_Remove(tw_api->boundList, le, TRUE);
			break;
		}
		le = twList_Next(tw_api->boundList, le);
	}
	/* Create the bind message */
	msg = twMessage_CreateBindMsg(entityName, TRUE);
	if (!msg) {
		TW_LOG(TW_ERROR, "twApi_UnbindThing: Error creating Unbind message");
		return TW_ERROR_CREATING_MSG;
	}
	twMutex_Lock(tw_api->mtx);
	resp = sendMessageBlocking(msg, 10000, NULL);
	twMutex_Unlock(tw_api->mtx);
	res = convertMsgCodeToErrorCode(resp);
	if (res != TW_OK) TW_LOG(TW_ERROR, "twApi_UnbindThing: Error creating sending Unbind message");
	if (msg) twMessage_Delete(msg);
	/* Look for any callbacks */
	le = twList_Next(tw_api->bindEventCallbackList, NULL);
	while (le && le->value) {
		callbackInfo * info = (callbackInfo *)le->value;
		if (!info->entityName || !strcmp(info->entityName, entityName)) {
			bindEvent_cb cb = (bindEvent_cb)info->cb;
			cb(entityName, FALSE, info->userdata);
		}
		le = twList_Next(tw_api->bindEventCallbackList, le);
	}
	return res;
}

void twApi_TaskerFunction(DATETIME now, void * params) {
	static DATETIME nextPingTime;
	static DATETIME expectedPongTime;
	static DATETIME nextCleanupTime;
	static DATETIME nextDutyCycleEvent;

	/* This is the main "loop" of the api */
	if (tw_api && tw_api->mh && tw_api->mh->ws) {
		if (twApi_isConnected()) {
			if (twTimeGreaterThan(now, nextPingTime)) {
				/* Time to send our keep alive ping */
				expectedPongTime = twAddMilliseconds(now, DEFAULT_MESSAGE_TIMEOUT);
				receivedPong = FALSE;
				twWs_SendPing(tw_api->mh->ws,0);
				nextPingTime = twAddMilliseconds(now, tw_api->ping_rate);
			}
			if (tw_api->handle_pongs && !receivedPong && twTimeGreaterThan(now, expectedPongTime)) {
				/* We didn't receive a pong in time */
				TW_LOG(TW_WARN,"apiThread: Did not receive pong in time");
				tw_api->manuallyDisconnected = FALSE;
				tw_api->isAuthenticated = FALSE;
				/* This call will likely fail since we are already not connected, but do it just in case */
				twWs_Disconnect(tw_api->mh->ws, NORMAL_CLOSE, "Pong timeout");
				/* Force a ping next time we connect */
				nextPingTime = 0;
			}
		}
		if (twTimeGreaterThan(now, nextCleanupTime)) {
			twMessageHandler_CleanupOldMessages(tw_api->mh);
			twMultipartMessageStore_RemoveStaleMessages();
			nextCleanupTime = twAddMilliseconds(now, STALE_MSG_CLEANUP_RATE);
		}
		if (tw_api->duty_cycle_period && twTimeGreaterThan(now, nextDutyCycleEvent)) {
			if (twApi_isConnected()) {
				TW_LOG(TW_INFO,"apiThread: Entering Duty Cycle OFF state.");
				twApi_Disconnect("Duty cycle off time");
				nextDutyCycleEvent = twAddMilliseconds(now, (tw_api->duty_cycle_period * (100 - tw_api->duty_cycle))/100);
			}  else {
				TW_LOG(TW_INFO,"apiThread: Entering Duty Cycle ON state.");
				twApi_Connect(tw_api->connect_timeout, tw_api->connect_retries);
				nextDutyCycleEvent = twAddMilliseconds(now, (tw_api->duty_cycle_period * tw_api->duty_cycle)/100);
			}
		}
		if (tw_api && tw_api->mh && tw_api->mh->ws && !tw_api->mh->ws->isConnected) tw_api->isAuthenticated = FALSE;
		if (!twWs_IsConnected(tw_api->mh->ws) && tw_api && tw_api->autoreconnect && tw_api->manuallyDisconnected == FALSE) {
			twApi_Connect(tw_api->connect_timeout, 1);
		}
	}
	if (tw_api && tw_api->manuallyDisconnected == FALSE) twWs_Receive(tw_api->mh->ws, 1);
}

int twApi_RegisterProperty(enum entityTypeEnum entityType, char * entityName, char * propertyName, enum BaseType propertyType, 
						   char * propertyDescription, char * propertyPushType, double propertyPushThreshold, property_cb cb, void * userdata) {
	callbackInfo * info = NULL;
	if (tw_api && tw_api->callbackList && entityName && propertyName && cb) {
		twPropertyDef * property = twPropertyDef_Create(propertyName, propertyType, propertyDescription, propertyPushType, propertyPushThreshold);
		info = (callbackInfo *)TW_CALLOC(sizeof(callbackInfo), 1);
		if (!info || !property) {
			TW_LOG(TW_ERROR, "twApi_RegisterProperty: Error allocating callback info");
			twPropertyDef_Delete(property);
			if (info) TW_FREE(info);
			return TW_ERROR_ALLOCATING_MEMORY;
		}
		info->entityType = entityType;
		info->entityName = duplicateString(entityName);
		info->characteristicType = TW_PROPERTIES;
		info->charateristicName = property->name;
		info->charateristicDefinition = property;
		info->cb = cb;
		info->userdata = userdata;
		return (twList_Add(tw_api->callbackList, info));
	}
	TW_LOG(TW_ERROR, "twApi_RegisterProperty: Invalid params or missing api pointer");
	return TW_INVALID_PARAM;
}

int twApi_RegisterService(enum entityTypeEnum entityType, char * entityName, char * serviceName, char * serviceDescription,
						  twDataShape * inputs, enum BaseType outputType, twDataShape * outputDataShape, service_cb cb, void * userdata) {
	callbackInfo * info = NULL;
	if (tw_api && tw_api->callbackList && entityName && serviceName && cb) {
		twServiceDef * service = twServiceDef_Create(serviceName, serviceDescription, inputs, outputType, 
														outputDataShape);
		info = (callbackInfo *)TW_CALLOC(sizeof(callbackInfo), 1);
		if (!info || !service) {
			TW_LOG(TW_ERROR, "twApi_RegisterService: Error allocating callback info");
			if (info) TW_FREE(info);
			twServiceDef_Delete(service);
			return TW_ERROR_ALLOCATING_MEMORY;
		}
		info->entityType = entityType;
		info->entityName = duplicateString(entityName);
		info->characteristicType = TW_SERVICES;
		info->charateristicName = service->name;
		info->charateristicDefinition = service;
		info->cb = cb;
		info->userdata = userdata;
		return twList_Add(tw_api->callbackList, info);
	}
	TW_LOG(TW_ERROR, "twApi_RegisterService: Invalid params or missing api pointer");
	return TW_INVALID_PARAM;
}

int twApi_UnregisterThing(char * entityName) {
	if (tw_api && tw_api->callbackList && entityName) {
		/* Get all callbacks registered to this entity */
		ListEntry * le = NULL;
		le = twList_Next(tw_api->callbackList, NULL);
		while (le) {
			if (le->value) {
				callbackInfo * tmp = (callbackInfo *)(le->value);
				if (strcmp(entityName, tmp->entityName)) {
					le = twList_Next(tw_api->callbackList, le);
					continue;
				}
				/* Delete this entry */
				twList_Remove(tw_api->callbackList, le, TRUE);
				le = twList_Next(tw_api->callbackList, le->prev);
			}
		}
		return 0;
	}
	TW_LOG(TW_ERROR, "twApi_UnregisterThing: Invalid params or missing api pointer");
	return TW_INVALID_PARAM;
}

int twApi_RegisterDefaultRequestHandler(genericRequest_cb cb) {
	if (tw_api) {
		tw_api->defaultRequestHandler = cb;
		return TW_OK;
	}
	return TW_NULL_API_SINGLETON;
}

propertyList * twApi_CreatePropertyList(char * name, twPrimitive * value, DATETIME timestamp) {
	propertyList * proplist = twList_Create(twProperty_Delete);
	if (!proplist) {
		TW_LOG(TW_ERROR,"twApi_CreatePropertyList: Error allocating property list");
		return NULL;
	}
	if (twList_Add(proplist, twProperty_Create(name, value, timestamp))) {
		TW_LOG(TW_ERROR,"twApi_CreatePropertyList: Error adding initial property  to list");
		twList_Delete(proplist);
		return NULL;
	}
	return proplist;
}

int twApi_DeletePropertyList(propertyList * list) {
	return twList_Delete(list);
}

int twApi_AddPropertyToList(propertyList * proplist, char * name, twPrimitive * value, DATETIME timestamp) {
	return twList_Add(proplist, twProperty_Create(name, value, timestamp));
}

int twApi_ReadProperty(enum entityTypeEnum entityType, char * entityName, char * propertyName, twPrimitive ** result, int32_t timeout, char forceConnect) {
	enum msgCodeEnum res = makePropertyRequest(TWX_GET, entityType, entityName, propertyName, 0, result, timeout, forceConnect);
	return convertMsgCodeToErrorCode(res);
}

int twApi_WriteProperty(enum entityTypeEnum entityType, char * entityName, char * propertyName, twPrimitive * value, int32_t timeout, char forceConnect) {
	twPrimitive * result = NULL;
	enum msgCodeEnum res;
	res = makePropertyRequest(TWX_PUT, entityType, entityName, propertyName, value, &result, timeout, forceConnect);
	twPrimitive_Delete(result);
	return convertMsgCodeToErrorCode(res);
}

int twApi_PushProperties(enum entityTypeEnum entityType, char * entityName, propertyList * properties, int32_t timeout, char forceConnect) {
	twDataShape* ds = NULL;
	twInfoTable * it = NULL;
	twInfoTable * values = NULL;
	twInfoTable * result = NULL;
	ListEntry * le = NULL;
	int res = TW_UNKNOWN_ERROR;
	/* Validate the inoputs */
	if (!entityName || !properties) {
		TW_LOG(TW_ERROR,"twApi_PushProperties: Missing inputs");
		return TWX_BAD_REQUEST;
	}
	/* Create the data shape */
	ds = twDataShape_Create(twDataShapeEntry_Create("name", NULL, TW_STRING));
	if (!ds) {
		TW_LOG(TW_ERROR,"twApi_PushProperties: Error allocating data shape");
		return TWX_INTERNAL_SERVER_ERROR;
	}
	twDataShape_AddEntry(ds,twDataShapeEntry_Create("value", NULL, TW_VARIANT));
	twDataShape_AddEntry(ds,twDataShapeEntry_Create("time", NULL, TW_DATETIME));
	twDataShape_AddEntry(ds,twDataShapeEntry_Create("quality", NULL, TW_STRING));
	/* Create the infotable */
	it = twInfoTable_Create(ds);
	if (!it) {
		TW_LOG(TW_ERROR,"twApi_PushProperties: Error creating infotable");
		twDataShape_Delete(ds);
		return TWX_INTERNAL_SERVER_ERROR;
	}
	/* Loop through the list and create a row per entry */
	le = twList_Next(properties, NULL);
	while (le) {
		twProperty * prop = (twProperty *)le->value;
		twInfoTableRow * row = NULL;
		row = twInfoTableRow_Create(twPrimitive_CreateFromString(prop->name, TRUE));
		if (!row) {
			TW_LOG(TW_ERROR,"twApi_PushProperties: Error creating infotable row");
			break;
		}
		twInfoTableRow_AddEntry(row, twPrimitive_CreateVariant(twPrimitive_ZeroCopy(prop->value)));
		twInfoTableRow_AddEntry(row,twPrimitive_CreateFromDatetime(prop->timestamp));
		twInfoTableRow_AddEntry(row,twPrimitive_CreateFromString("GOOD",TRUE));
		twInfoTable_AddRow(it,row);
		le = twList_Next(properties, le);
	}
	/* Make the service request */
	values = twInfoTable_Create(twDataShape_Create(twDataShapeEntry_Create("values", NULL, TW_INFOTABLE)));
	twInfoTable_AddRow(values, twInfoTableRow_Create(twPrimitive_CreateFromInfoTable(it)));
	res = twApi_InvokeService(entityType, entityName, "UpdateSubscribedPropertyValues", values, &result, timeout, forceConnect);
	twInfoTable_Delete(it);
	twInfoTable_Delete(values);
	twInfoTable_Delete(result);
	return res;
}

int twApi_InvokeService(enum entityTypeEnum entityType, char * entityName, char * serviceName, twInfoTable * params, twInfoTable ** result, int32_t timeout, char forceConnect) {
	enum msgCodeEnum res = makeRequest(TWX_POST, entityType, entityName, TW_SERVICES, serviceName, params, result, timeout, forceConnect);
	return convertMsgCodeToErrorCode(res);
}

int twApi_FireEvent(enum entityTypeEnum entityType, char * entityName, char * eventName, twInfoTable * params, int32_t timeout, char forceConnect) {
	twInfoTable * result = NULL;
	enum msgCodeEnum res;
	res = makeRequest(TWX_POST, entityType, entityName, TW_EVENTS, eventName, params, &result, timeout, forceConnect);
	twInfoTable_Delete(result);
	return convertMsgCodeToErrorCode(res);
}

int twApi_RegisterConnectCallback(eventcb cb) {
	if (tw_api && tw_api->mh) return twMessageHandler_RegisterConnectCallback(tw_api->mh, cb);
	return TW_NULL_OR_INVALID_API_SINGLETON;
}

int twApi_RegisterCloseCallback(eventcb cb) {
	if (tw_api && tw_api->mh) return twMessageHandler_RegisterCloseCallback(tw_api->mh, cb);
	return TW_NULL_OR_INVALID_API_SINGLETON;
}

int twApi_RegisterPingCallback(eventcb cb) {
	if (tw_api && tw_api->mh) return twMessageHandler_RegisterPingCallback(tw_api->mh, cb);
	return TW_NULL_OR_INVALID_API_SINGLETON;
}

int twApi_RegisterPongCallback(eventcb cb) {
	if (tw_api && tw_api->mh){
		/* We are no longer handling pongs in the api */
		tw_api->handle_pongs = FALSE;
		return twMessageHandler_RegisterPongCallback(tw_api->mh, cb);
	}
	return TW_NULL_OR_INVALID_API_SINGLETON;
}

int twApi_RegisterBindEventCallback(char * entityName, bindEvent_cb cb, void * userdata) {
	callbackInfo * info = (callbackInfo *)TW_CALLOC(sizeof(callbackInfo), 1);
	if (!tw_api || !tw_api->bindEventCallbackList || !cb) return TW_NULL_OR_INVALID_API_SINGLETON;
	if (!info) return TW_ERROR_ALLOCATING_MEMORY;
	if (entityName) info->entityName = duplicateString(entityName);
	info->cb = cb;
	info->userdata = userdata;
	return twList_Add(tw_api->bindEventCallbackList, info);
}

int twApi_UnregisterBindEventCallback(char * entityName, bindEvent_cb cb, void * userdata) {
	ListEntry * le = NULL;
	if (!tw_api || !tw_api->bindEventCallbackList) return TW_NULL_OR_INVALID_API_SINGLETON;
	le = twList_Next(tw_api->bindEventCallbackList, NULL);
	while (le && le->value) {
		callbackInfo * info = (callbackInfo *)le->value;
		if (info->cb == cb && info->userdata == userdata && !strcmp(info->entityName, entityName)) {
			twList_Remove(tw_api->bindEventCallbackList, le, TRUE);
			return TW_OK;
		}
		le = twList_Next(tw_api->bindEventCallbackList, le);
	}
	return TW_NOT_FOUND;
}

int twApi_CleanupOldMessages() {
	if (tw_api && tw_api->mh) return twMessageHandler_CleanupOldMessages(tw_api->mh);
	return TW_NULL_OR_INVALID_API_SINGLETON;
}

int twApi_SendPing(char * content) {
	if (tw_api && tw_api->mh && tw_api->mh->ws) return twWs_SendPing(tw_api->mh->ws, content);
	return TW_NULL_OR_INVALID_API_SINGLETON;
}

int twApi_CreateTask(uint32_t runTintervalMsec, twTaskFunction func) {
	return twTasker_CreateTask(runTintervalMsec, func);
}

void twApi_SetSelfSignedOk() {
	if (tw_api && tw_api->mh && tw_api->mh->ws) twTlsClient_SetSelfSignedOk(tw_api->mh->ws->connection);
}

void twApi_DisableCertValidation() {
	if (tw_api && tw_api->mh && tw_api->mh->ws) twTlsClient_DisableCertValidation(tw_api->mh->ws->connection);
}

int	twApi_LoadCACert(const char *file, int type) {
	if (tw_api && tw_api->mh && tw_api->mh->ws) return twTlsClient_UseCertificateChainFile(tw_api->mh->ws->connection, file,type);
	else return TW_NULL_OR_INVALID_MSG_HANDLER;
}
