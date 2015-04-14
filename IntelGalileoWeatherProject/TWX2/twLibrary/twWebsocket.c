/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Portable Websocket Client  abstraction layer
 */

#include "twOSPort.h"
#include "twWebsocket.h"
#include "twErrors.h"
#include "twTls.h"
#include "twLogger.h"
#include "http_parser.h"
#include "base64.h"
#include "sha-1.h"
#include "stringUtils.h"

#include <string.h>
#include <stdlib.h> 
#include <stdio.h>

#define NOT_SET -1
#define TW_TRUE 1
#define TW_FALSE 0

#define WS_VERSION "13"
#define WS_HEADER_MAX_SIZE 10
#define KEY_LENGTH 16
/* Base 64 encoded key - add the last 1 byte for null termination */
#define ENCODED_KEY_LENGTH (KEY_LENGTH * 2)

#define RCVD_CONNECTION_HEADER 0x01
#define RCVD_UPGRADE_HEADER 0x20
#define VALID_WS_ACCEPT_KEY 0x40

signed char isLittleEndian = NOT_SET;

/**
* Websocket helper functions
**/
int sendCtlFrame(twWs * ws, unsigned char type, char * msg);
int sendDataFrame(twWs * ws, char * msg, uint16_t length, char isContinuation, char isFinal, char isText);
int validateAcceptKey(twWs * ws, const char * header_value);
int32_t handleDataFrame(http_parser* parser, const char *at, size_t length, char isText, char isContinuation);

/**
* Http_parser callbacks
**/
char header_name[256];
char header_value[256];

int32_t ws_on_message_begin(http_parser* parser) {
	TW_LOG(TW_DEBUG,"HTTP Response begun");
	return 0;
}
int32_t ws_on_url(http_parser* parser, const char *at, size_t length) {
	TW_LOG(TW_TRACE,"Url->%s : %s", header_name, header_value);
	return 0;
}
int32_t ws_on_header_field(http_parser* parser, const char *at, size_t length) {
	memset(header_name, 0, 256);
	memcpy(header_name, at, length > 255 ? 255 : length);
	lowercase(header_name);
	return 0;
}
int32_t ws_on_header_value(http_parser* parser, const char *at, size_t length) {
	if (!parser || !parser->data) {
		TW_LOG(TW_DEBUG,"ws_on_header_value: NULL parser or data value");
		return 1;
	}
	memset(header_value, 0, 256);
	memcpy(header_value, at, length > 255 ? 255 : length);
	TW_LOG(TW_TRACE,"Header->%s : %s", header_name, header_value);
	if (strcmp(header_name, "upgrade") == 0) {
		if (strcmp(header_value, "websocket") != 0) {
			TW_LOG(TW_ERROR, "ws_on_header_value: Invalid 'upgrade' header: %s", header_value);
			((twWs *)(parser->data))->connect_state = -1;
		} else ((twWs *)(parser->data))->connect_state |= RCVD_UPGRADE_HEADER;
	} else if (strcmp(header_name, "connection") == 0) {
		if (strcmp(header_value, "upgrade") != 0) {
			TW_LOG(TW_ERROR, "ws_on_header_value: Invalid 'connection' header: %s", header_value);
			((twWs *)(parser->data))->connect_state = -1;
		} else ((twWs *)(parser->data))->connect_state |= RCVD_CONNECTION_HEADER;
	} else if (strcmp(header_name, "sec-websocket-accept") == 0) {
		if (validateAcceptKey((twWs *)(parser->data), header_value) != 0) {
			TW_LOG(TW_ERROR, "ws_on_header_value: Invalid 'sec-websocket-accept' header: %s", header_value);
			((twWs *)(parser->data))->connect_state = -1;
		} else ((twWs *)(parser->data))->connect_state |= VALID_WS_ACCEPT_KEY;
	} 
	return 0;
}
int32_t ws_on_headers_complete(http_parser* parser) {
	int state;

	if (!parser || !parser->data) {
		TW_LOG(TW_DEBUG,"ws_on_headers_complete: NULL parser or data value");
		return 1;
	}
	state = ((twWs *)(parser->data))->connect_state;
	if (state != -1 && state & RCVD_UPGRADE_HEADER && state & RCVD_CONNECTION_HEADER && state & VALID_WS_ACCEPT_KEY) {
		TW_LOG(TW_DEBUG,"ws_on_headers_complete: Websocket connected!");
		((twWs *)(parser->data))->isConnected = TRUE;
		return 2;
	}
	TW_LOG(TW_ERROR,"ws_on_headers_complete: Websocket connection failed");
	return 1;
}

int32_t ws_on_body(http_parser* parser, const char *at, size_t length){
	return 0;
}

int32_t ws_on_message_complete(http_parser* parser){
	return 0;
}

int32_t ws_on_ping(http_parser* parser, const char *at, size_t length) {

	twWs * ws = NULL;
	TW_LOG(TW_TRACE,"ws_on_ping: Received Ping");
	if (!parser || !parser->data) {
		TW_LOG(TW_DEBUG,"ws_on_ping: NULL parser or data value");
		return 1;
	}
	ws = (twWs *)(parser->data);
	if (ws->on_ws_ping) return ws->on_ws_ping(ws, at, length);
	else return 0;
}

int32_t ws_on_pong(http_parser* parser, const char *at, size_t length) {

	twWs * ws = NULL;
	TW_LOG(TW_TRACE,"ws_on_pong: Received Pong");
	if (!parser || !parser->data) {
		TW_LOG(TW_DEBUG,"ws_on_pong: NULL parser or data value");
		return 1;
	}
	ws = (twWs *)(parser->data);
	if (ws->on_ws_pong) return ws->on_ws_pong(ws, at, length);
	else return 0;
}

int32_t ws_on_textframe(http_parser* parser, const char *at, size_t length) {
	TW_LOG(TW_TRACE,"ws_on_textframe: Received Text Frame");
	return handleDataFrame(parser, at, length, 1, 0);
}

int32_t ws_on_binaryframe(http_parser* parser, const char *at, size_t length) {
	TW_LOG(TW_TRACE,"ws_on_binaryframe: Received Binary Frame");
	return handleDataFrame(parser, at, length, 0, 0);
}

int32_t ws_on_continuationframe(http_parser* parser, const char *at, size_t length){
	TW_LOG(TW_TRACE,"ws_on_continuationframe: Received Continuation Frame");
	return handleDataFrame(parser, at, length, 0, 1);
}

int32_t ws_on_close(http_parser* parser, const char *at, size_t length){
	twWs * ws = NULL;
	if (!parser || !parser->data) {
		TW_LOG(TW_DEBUG,"ws_on_connected: NULL parser or data value");
		return 1;
	}
	TW_LOG(TW_WARN,"ws_on_close: Websocket closed!");
	ws = (twWs *)(parser->data);
	ws->isConnected = 0;
	if (ws->on_ws_close) return ws->on_ws_close(ws, at, length);
	else return 0;
}

int32_t ws_on_connected(http_parser* parser){
	twWs * ws = NULL;
	if (!parser || !parser->data) {
		TW_LOG(TW_DEBUG,"ws_on_connected: NULL parser or data value");
		return 1;
	}
	TW_LOG(TW_FORCE,"ws_on_connected: Websocket connected!");
	ws = (twWs *)(parser->data);
	if (ws->on_ws_connected) return (ws->on_ws_connected)(ws);
	else return 0;
}

int32_t ws_on_framelength(http_parser* parser){
	return 0;
}

/**
* Helper functions
**/
int restartSocket(twWs * ws) {
	/* Tear down the socket and create a new one */ 
    int res = twTlsClient_Reconnect(ws->connection, ws->host, ws->port);
	ws->messagePtr = ws->messageBuffer;
	ws->frameBufferPtr = ws->frameBuffer;
	http_parser_init(ws->parser, HTTP_RESPONSE);
	ws->parser->data = ws;
    return res;
}

/**
*	Context manipulation functions
**/
int twWs_Create(char * host, uint16_t port, char * resource, char * api_key, char * gatewayName, uint32_t messageChunkSize, uint16_t frameSize, twWs ** entity) {
	int err = TW_UNKNOWN_ERROR;
	twWs * ws = NULL;

	TW_LOG(TW_DEBUG, "twWs_Create: Initializing Websocket Client for %s:%d/%s", host, port, resource);

	/* Validate our host/port */
	if (!host || !port || !resource || !api_key || !entity) {
		TW_LOG(TW_ERROR, "twWs_Create: Missing required parameters");
		return TW_INVALID_PARAM;
	}
	ws = (twWs *)TW_CALLOC(sizeof(twWs), 1);
	if (!ws) {
		TW_LOG(TW_ERROR, "twWs_Create: Error allocating websocket struct");
		return TW_ERROR_ALLOCATING_MEMORY;
	}
	ws->isConnected = FALSE;
	/* Create our connection  */
	err = twTlsClient_Create(host, port, 0, &ws->connection);
	if (err) {
		TW_LOG(TW_ERROR, "twWs_Create: Error creating BSD socket to be used for the websocket");
		twWs_Delete(ws);
		return err;
	}
	ws->port = port;
	/* Create copies of any strings passed in */
	ws->host = duplicateString(host);
	if (!ws->host) {
		TW_LOG(TW_ERROR, "twWs_Create: Error allocating storage for websocket host");
		twWs_Delete(ws);
		return TW_ERROR_ALLOCATING_MEMORY;
	}	
	ws->api_key = duplicateString(api_key);
	if (!ws->api_key) {
		TW_LOG(TW_ERROR, "twWs_Create: Error allocating storage for websocket api_key");
		twWs_Delete(ws);
		return TW_ERROR_ALLOCATING_MEMORY;
	}	
	ws->resource = duplicateString(resource);
	if (!ws->resource) {
		TW_LOG(TW_ERROR, "twWs_Create: Error allocating storage for websocket resource");
		twWs_Delete(ws);
		return TW_ERROR_ALLOCATING_MEMORY;
	}	
	if (gatewayName) {
		ws->gatewayName = duplicateString(gatewayName);
		if (!ws->gatewayName) {
			TW_LOG(TW_ERROR, "twWs_Create: Error allocating storage for websocket gatewayName");
			twWs_Delete(ws);
			return TW_ERROR_ALLOCATING_MEMORY;
		}
	}
	/* Create the mutexes */
	ws->sendMessageMutex = twMutex_Create();
	ws->sendFrameMutex = twMutex_Create();
	ws->recvMutex = twMutex_Create();
	if (!ws->sendMessageMutex || !ws->recvMutex || !ws->sendFrameMutex) {
		TW_LOG(TW_ERROR, "Error allocating or creating mutex");
		twWs_Delete(ws);
		return TW_ERROR_CREATING_MTX;
	}	
	/* Allocate our message and frame storage - Add 5% just for some overrun protection */
	ws->messageChunkSize = messageChunkSize;
	ws->messageBuffer = (char *)TW_CALLOC(messageChunkSize + messageChunkSize/20, 1);
	if (!ws->messageBuffer) {
		TW_LOG(TW_ERROR, "twWs_Create: Error allocating storage for websocket message buffer");
		twWs_Delete(ws);
		return TW_ERROR_ALLOCATING_MEMORY;
	}	
	ws->messagePtr = ws->messageBuffer;
	/* Need 2X the max frame size to handle parsing */
	ws->frameSize = frameSize;
	ws->frameBuffer = (char *)TW_CALLOC((frameSize + WS_HEADER_MAX_SIZE), 1);
	if (!ws->frameBuffer) {
		TW_LOG(TW_ERROR, "twWs_Create: Error allocating storage for websocket resource");
		twWs_Delete(ws);
		return TW_ERROR_ALLOCATING_MEMORY;
	}	
	ws->frameBufferPtr = ws->frameBuffer;
	/* create and configure the http parser */
	ws->parser = (http_parser *)TW_CALLOC(sizeof(http_parser), 1);
	if (!ws->parser) {
		TW_LOG(TW_ERROR, "twWs_Create: Error allocating storage for websocket http parser");
		twWs_Delete(ws);
		return TW_ERROR_ALLOCATING_MEMORY;
	}
	ws->settings = (http_parser_settings *)TW_CALLOC(sizeof(http_parser_settings), 1);
	if (!ws->settings) {
		TW_LOG(TW_ERROR, "twWs_Create: Error allocating storage for websocket http parser settings");
		twWs_Delete(ws);
		return TW_ERROR_ALLOCATING_MEMORY;
	}
	http_parser_init(ws->parser, HTTP_RESPONSE);
	ws->parser->data = ws;
	ws->settings->on_message_begin = &ws_on_message_begin;
	ws->settings->on_url = &ws_on_url;
	ws->settings->on_header_field = &ws_on_header_field;
	ws->settings->on_header_value = &ws_on_header_value;
	ws->settings->on_headers_complete = &ws_on_headers_complete;
	ws->settings->on_body = &ws_on_body;
	ws->settings->on_message_complete = &ws_on_message_complete;
	ws->settings->on_ws_connected = &ws_on_connected;
	ws->settings->on_ws_ping = &ws_on_ping;
	ws->settings->on_ws_pong = &ws_on_pong;
	ws->settings->on_ws_textframe = &ws_on_textframe;
	ws->settings->on_ws_binaryframe = &ws_on_binaryframe;
	ws->settings->on_ws_continuationframe= &ws_on_continuationframe;
	ws->settings->on_ws_close = &ws_on_close;
	ws->settings->on_ws_framelength = &ws_on_framelength;

	*entity = ws;
	return TW_OK;
}

int twWs_Delete(twWs * ws) {
	if (!ws) { 
		TW_LOG(TW_ERROR, "twWs_Delete: NULL ws pointer"); 
		return TW_INVALID_PARAM; 
	}
	if (ws->connection) {
		twTlsClient_Delete(ws->connection); 
	}
	free(ws->api_key);
	free(ws->host);
	free(ws->frameBuffer);
	free(ws->messageBuffer);
	free(ws->resource);
	free(ws->parser);
	free(ws->settings);
	free(ws->security_key);
	if (ws->gatewayName) free(ws->gatewayName);
	twMutex_Delete(ws->sendMessageMutex);
	twMutex_Delete(ws->sendFrameMutex);
	twMutex_Delete(ws->recvMutex);
	free(ws);
	return TW_OK;
}

#define REQ_SIZE 512
int twWs_Connect(twWs * ws, uint32_t timeout) {

	int32_t i = 0;
	int32_t bytesWritten = 0;
	int32_t bytesRead = 0;
	int32_t bytesParsed = 0;
	char * parsePtr = 0;
	char key[KEY_LENGTH];
	char * req = NULL;
	char max_frame_size[16];
	DATETIME timeouttime = 0;
	DATETIME now = 0;;

	if (!ws) { 
		TW_LOG(TW_ERROR, "twWs_Connect: NULL ws pointer"); 
		return TW_INVALID_PARAM; 
	}
	if (ws->isConnected) { 
		TW_LOG(TW_WARN, "twWs_Connect: Already connected");
		return TW_OK; 
	}

	twMutex_Lock(ws->sendMessageMutex);
	ws->connect_state = 0;

	/* Create the random key */
	now = twGetSystemTime(TRUE);
	srand(now % 1000);
	for (i = 0; i < KEY_LENGTH; i++) {
		uint8_t r = rand() & 0xff;
		key[i] = r;
	}
	if (!ws->security_key) ws->security_key = (char *)TW_CALLOC(ENCODED_KEY_LENGTH, 1);
	if (!ws->security_key) {
		TW_LOG(TW_ERROR,"twWs_Connect: Error allocating security key buffer");
		twMutex_Unlock(ws->sendMessageMutex);
		return TW_ERROR_ALLOCATING_MEMORY;
	} 
	twBase64Encode(key, KEY_LENGTH, ws->security_key, ENCODED_KEY_LENGTH);

	/* Form the HTTP request */
	req = (char *)TW_CALLOC(REQ_SIZE, 1);
	if (!req) {
		TW_LOG(TW_ERROR,"twWs_Connect: Error allocating request buffer");
		twMutex_Unlock(ws->sendMessageMutex);
		return TW_ERROR_ALLOCATING_MEMORY;
	} 
	strncpy(req,"GET ", REQ_SIZE - 1);
	strncat(req, ws->resource, REQ_SIZE - strlen(req) - 1);
	strncat(req, " HTTP/1.1\r\n", REQ_SIZE - strlen(req) - 1);
	strncat(req, "User-Agent: ThingWorx C SDK\r\n", REQ_SIZE - strlen(req) - 1);
	strncat(req, "Upgrade: websocket\r\n", REQ_SIZE - strlen(req) - 1);
	strncat(req, "Connection: Upgrade\r\n", REQ_SIZE - strlen(req) - 1);
	strncat(req, "Host: ", REQ_SIZE - strlen(req) - 1);
	strncat(req, ws->host, REQ_SIZE - strlen(req) - 1);
	strncat(req, "\r\n", REQ_SIZE - strlen(req) - 1);
	strncat(req, "Sec-WebSocket-Version: ", REQ_SIZE - strlen(req) - 1);
	strncat(req, WS_VERSION, REQ_SIZE - strlen(req) - 1);
	strncat(req, "\r\n", REQ_SIZE - strlen(req) - 1);
	strncat(req, "Sec-WebSocket-Key: ", REQ_SIZE - strlen(req) - 1);
	strncat(req, ws->security_key, REQ_SIZE - strlen(req) - 1);
	strncat(req, "\r\n", REQ_SIZE - strlen(req) - 1);
	strncat(req, "Max-Frame-Size: ", REQ_SIZE - strlen(req) - 1);
        sprintf(max_frame_size, "%u", ws->frameSize);
	strncat(req, max_frame_size, REQ_SIZE - strlen(req) - 1);
	strncat(req, "\r\n", REQ_SIZE - strlen(req) - 1);
	strncat(req, "applicationKey: ", REQ_SIZE - strlen(req) - 1);
	strncat(req, ws->api_key, REQ_SIZE - strlen(req) - 1);
	strncat(req, "\r\n", REQ_SIZE - strlen(req) - 1);
	strncat(req, "\r\n", REQ_SIZE - strlen(req) - 1);
	
	/* Connect the underlying socket and send the request */
	if (restartSocket(ws)) {
		TW_LOG(TW_ERROR,"twWs_Connect: Error restarting socket.  Error %d", twSocket_GetLastError());
		TW_FREE (req);
		twMutex_Unlock(ws->sendMessageMutex);
		return TW_SOCKET_INIT_ERROR;
	}
	bytesWritten = twTlsClient_Write(ws->connection, req, strlen(req), 100);
	if (bytesWritten > 0) TW_LOG(TW_TRACE, "twWs_Connect: Connected to %s:%d", ws->host, ws->port);
	else {
		TW_LOG(TW_ERROR,"twWs_Connect: No bytes written.  Error %d", twSocket_GetLastError());
		TW_FREE (req);
		twMutex_Unlock(ws->sendMessageMutex);
		restartSocket(ws);
		return TW_ERROR_WRITING_TO_SOCKET;
	} 
	/* Done with the request */
	TW_LOG(TW_TRACE, "twWs_Connect: Sent request:\n %s", req);
	TW_FREE(req);
	/* Get the response */
	timeouttime = twGetSystemTime(TRUE);
	timeouttime = twAddMilliseconds(timeouttime,timeout);
	now = twGetSystemTime(TRUE);
	parsePtr = ws->messageBuffer;
	while (ws->connect_state >= 0 && !ws->isConnected && twTimeGreaterThan(timeouttime, now)) {
		bytesRead = twTlsClient_Read(ws->connection, ws->messagePtr, 
			ws->messageChunkSize - (ws->messagePtr - ws->messageBuffer), 100);
		if (bytesRead < 0) {
			/* Something is wrong with the socket - give up */
			TW_LOG(TW_ERROR,"twWs_Connect: Error reading from socket.  Error: %d", twSocket_GetLastError());
			twMutex_Unlock(ws->sendMessageMutex);
			return TW_ERROR_INITIALIZING_WEBSOCKET;
		}
		if (bytesRead) bytesParsed = http_parser_execute(ws->parser, ws->settings, parsePtr, bytesRead);
		if (bytesRead && bytesParsed == bytesRead) {
			TW_LOG(TW_TRACE,"twWs_Connect: Parsed %d bytes out of %d", bytesParsed, bytesRead);
			ws->messagePtr = ws->messageBuffer;
			bytesParsed = 0;
			parsePtr = ws->messageBuffer;
		} else {
			if (bytesRead > 0) TW_LOG(TW_WARN,"twWs_Connect: Only parsed %d bytes out of %d", bytesParsed, bytesRead);
			parsePtr = ws->messagePtr + bytesRead;
			ws->messagePtr = (bytesRead - bytesParsed) + ws->messagePtr;
			if (parsePtr > ws->messageBuffer + ws->messageChunkSize || 
				ws->messagePtr > ws->messageBuffer + ws->messageChunkSize) {
					TW_LOG(TW_ERROR,"twWs_Connect: Not enough buffer space to continue");
					twMutex_Unlock(ws->sendMessageMutex);
					return TW_ERROR_INITIALIZING_WEBSOCKET;
			}
			bytesParsed = 0;
		}
		now = twGetSystemTime(TRUE);
	}
	if (twTimeGreaterThan(now, timeouttime)) {
		/* We timed out */
		TW_LOG(TW_ERROR,"twWs_Connect: Timed out trying to connect");
		twMutex_Unlock(ws->sendMessageMutex);
		return TW_TIMEOUT_INITIALIZING_WEBSOCKET;
	}
	if (!ws->isConnected) {
		TW_LOG(TW_ERROR,"twWs_Connect: Error trying to connect");
		twMutex_Unlock(ws->sendMessageMutex);
		restartSocket(ws);
		return TW_ERROR_INITIALIZING_WEBSOCKET;
	}
	twMutex_Unlock(ws->sendMessageMutex);
	return TW_OK;
}

char twWs_IsConnected(twWs * ws) { 
	return (ws ? ws->isConnected : 0); 
}

int twWs_Disconnect(twWs * ws, enum close_status code, char * reason) {
	char msg[64];
	if (!ws) { 
		TW_LOG(TW_ERROR, "twWs_Disconnect: NULL ws pointer"); 
		return TW_INVALID_PARAM; 
	}
	/*  Send a close message */
	TW_LOG(TW_DEBUG, "Disconnect called.  Code: %d, Reason: %s", code, reason);
	if (code != SERVER_CLOSED) {
		/* Send a close to the server */
		msg[0] = 0x03;
		switch (code) {
			case NORMAL_CLOSE:
				strncpy(msg + 2,"Normal Close",60);
				msg[1] = (char)0xE8;
				break;
			case GOING_TO_SLEEP:
				strncpy(msg + 2,"Going to Sleep",60);
				msg[1] = (char)0xE9;
				break;
			case PROTOCOL_ERROR:
				strncpy(msg + 2,"Protocol Error",60);
				msg[1] = (char)0xEA;
				break;
			case UNSUPPORTED_DATA_TYPE:
				strncpy(msg + 2,"Unsupported Data Type",60);
				msg[1] = (char)0xEB;
				break;
			case INVALID_DATA:
				strncpy(msg + 2,"Invalid Data",60);
				msg[1] = (char)0xEF;
				break;
			case POLICY_VIOLATION:
				strncpy(msg + 2,"Policy Violation",60);
				msg[1] = (char)0xF0;
				break;
			case FRAME_TOO_LARGE:
				strncpy(msg + 2,"Frame too large",60);
				msg[1] = (char)0xF1;
				break;
			case NO_EXTENSION_FOUND:
				strncpy(msg + 2,"No extension found",60);
				msg[1] = (char)0xF2;
				break;
			case UNEXPECTED_CONDITION:
			default:
				strncpy(msg + 2,"Unexpected Condition", 60);
				msg[1] = (char)0xF3;
				break;
		}
		strncat(msg + 2, " ", 60);
		strncat(msg + 2, reason, 60);
		sendCtlFrame(ws, 0x08, msg);
	}
	ws->isConnected = FALSE;
	twTlsClient_Close(ws->connection);
	if (ws && ws->on_ws_close && msg[0] == 0x03) ws->on_ws_close(ws, msg + 2, strlen(msg + 2));
	return TW_OK;
}

int twWs_RegisterConnectCallback(twWs * ws, ws_cb cb) {
	if (!ws) { 
		TW_LOG(TW_ERROR, "twWs_RegisterConnectCallback: NULL ws pointer"); 
		return TW_INVALID_PARAM; 
	}
	ws->on_ws_connected = cb;
	return TW_OK;
}

int twWs_RegisterCloseCallback(twWs * ws, ws_data_cb cb) {
	if (!ws) { 
		TW_LOG(TW_ERROR, "twWs_RegisterCloseCallback: NULL ws pointer"); 
		return TW_INVALID_PARAM; 
	}
	ws->on_ws_close = cb;
	return TW_OK;
}

int twWs_RegisterBinaryMessageCallback(twWs * ws, ws_data_cb cb) {
	if (!ws) { 
		TW_LOG(TW_ERROR, "twWs_RegisterMessageCallback: NULL ws pointer"); 
		return TW_INVALID_PARAM; 
	}
	ws->on_ws_binaryMessage = cb;
	return TW_OK;
}

int twWs_RegisterTextMessageCallback(twWs * ws, ws_data_cb cb) {
	if (!ws) { 
		TW_LOG(TW_ERROR, "twWs_RegisterTextMessageCallback: NULL ws pointer"); 
		return TW_INVALID_PARAM;
	}
	ws->on_ws_textMessage = cb;
	return TW_OK;
}

int twWs_RegisterPingCallback(twWs * ws, ws_data_cb cb) {
	if (!ws) { 
		TW_LOG(TW_ERROR, "twWs_RegisterPingCallback: NULL ws pointer"); 
		return TW_INVALID_PARAM; 
	}
	ws->on_ws_ping = cb;
	return TW_OK;
}

int twWs_RegisterPongCallback(twWs * ws, ws_data_cb cb) {
	if (!ws) { 
		TW_LOG(TW_ERROR, "twWs_RegisterPongCallback: NULL ws pointer"); 
		return TW_INVALID_PARAM; 
	}
	ws->on_ws_pong = cb;
	return TW_OK;
}

/* Receive function for single threaded environments - does not return the data */
int twWs_Receive(twWs * ws, uint32_t timeout) {
	int32_t bytesRead = 0;
	int32_t bytesParsed = 0;
	int32_t size = WS_HEADER_MAX_SIZE;

	if (!ws) { 
		TW_LOG(TW_ERROR, "twWs_Receive: NULL ws pointer"); 
		return TW_INVALID_PARAM; 
	}
	if (!ws->isConnected) { 
		TW_LOG(TW_WARN, "twWs_Receive: Not connected"); 
		return TW_WEBSOCKET_NOT_CONNECTED; 
	}
	twMutex_Lock(ws->recvMutex);
	/**** 
	// We never want to read past frame data into another frame
	// so read only the maximum size of a ws header first and then
	// receive whatever remaining bytes are left in the frame.  The
	// reamining bytes are held in the parser's content_length field 

	// If we are not already receiving the frame content read just enough for the header
	// else read the number of bytes left in the frame content
	****/
	if (ws->parser->state == 103 || ws->parser->state == 104) size = (int32_t)ws->parser->content_length;
	bytesRead = twTlsClient_Read(ws->connection, ws->frameBufferPtr, size, 5);
	if (bytesRead > 0) {
		TW_LOG(TW_TRACE,"twWs_Receive: Read %d bytes into 0x%x", bytesRead, ws->frameBufferPtr);
		bytesParsed = http_parser_execute(ws->parser, ws->settings, ws->frameBufferPtr, bytesRead);
		if (bytesParsed == bytesRead) {
			TW_LOG(TW_TRACE,"twWs_Receive: Parsed %d bytes out of %d", bytesParsed, bytesRead);
			ws->frameBufferPtr = ws->frameBufferPtr + bytesRead;
			/* If we are at the beginning of a frame, reset our pointer to the beginning */
			if ((ws->parser->state == 101) || (ws->parser->state != 102 && ws->parser->state != 103 && ws->parser->state != 104)) {
				ws->frameBufferPtr = ws->frameBuffer;
			} 
			if (ws->frameBufferPtr > ws->frameBuffer + ws->frameSize) {
				TW_LOG(TW_ERROR,"twWs_Receive: BUFFER OVERRUN!  Something has gone terribly wrong.  Resetting buffer");
				ws->frameBufferPtr = ws->frameBuffer;
			}
			bytesRead = 0;
			twMutex_Unlock(ws->recvMutex);
			return TW_OK;
		} else {
			if (bytesRead) TW_LOG(TW_WARN,"twWs_Receive: Only parsed %d bytes out of %d", bytesParsed, bytesRead);
			twMutex_Unlock(ws->recvMutex);
			return TW_ERROR_PARSING_WEBSOCKET_DATA;
		}
	} else {
		if (bytesRead < 0) {
			TW_LOG(TW_WARN,"twWs_Receive: Error reading from socket.  Error: %d", twSocket_GetLastError());
			ws->isConnected = FALSE;
			if (ws && ws->on_ws_close) ws->on_ws_close(ws, "Socket Error", strlen("Socket Error"));
			twMutex_Unlock(ws->recvMutex);
			restartSocket(ws);
			return TW_ERROR_READING_FROM_WEBSOCKET;
		}
		twMutex_Unlock(ws->recvMutex);
		return TW_OK;
	}
}


int twWs_SendMessage(twWs * ws, char * buf, uint32_t length, char isText) {

	char * ptr = buf;
	char framesSent = 0;
	int res = -1;

	/* Do some status checks */
	if (!ws) { 
		TW_LOG(TW_ERROR, "twWs_SendMessage: NULL ws pointer"); 
		return TW_INVALID_PARAM; 
	}
	if (!ws->isConnected) { 
		TW_LOG(TW_WARN, "twWs_SendMessage: Not connected"); 
		return TW_WEBSOCKET_NOT_CONNECTED; 
	}

	/* Make sure we have a message and it fits in a frame */
	if (!buf) { TW_LOG(TW_ERROR, "twWs_SendMessage: NULL msg pointer"); return -1; }
	if (length == 0) { TW_LOG(TW_ERROR, "twWs_SendMessage: Message length is 0.  Not sending"); return -1; }
	if (ws->messageChunkSize < length) { 
		TW_LOG(TW_ERROR, "twWs_SendMessage: Frame of length %d is too large.  Max frame size is %u", 
		length, ws->frameSize); 
		return TW_WEBSCOKET_FRAME_TOO_LARGE;
	}

	twMutex_Lock(ws->sendMessageMutex);
	while (length > 0) {
		if (length > ws->frameSize) {
			if (framesSent) res = sendDataFrame(ws, ptr, length, 1, 0, isText); /* Continuation, not Final */
			else {
				res = sendDataFrame(ws, ptr, length, 0, 0, isText); /* Not Continuation, not Final */
			}
			if (res != 0) {
				TW_LOG(TW_ERROR, "twWs_SendMessage: Error sending frame %d. Error code: %d", framesSent, twSocket_GetLastError());
				twMutex_Unlock(ws->sendMessageMutex);
				return res;
			}
			framesSent++;
			ptr = ptr + ws->frameSize;
			length = length - ws->frameSize;
		} else {
			if (framesSent) res = sendDataFrame(ws, ptr, length, 1, 1, isText); /* Continuation, Final */
			else {
				res = sendDataFrame(ws, ptr, length, 0, 1, isText); /* Not Continuation, not Final */
			}
			if (res != 0) {
				TW_LOG(TW_ERROR, "twWs_SendMessage: Error sending frame %d. Error code: %d", framesSent, twSocket_GetLastError());
				twMutex_Unlock(ws->sendMessageMutex);
				return res;
			}
			framesSent++;
			ptr = ptr + length;
			length = 0;
		}
	}
	TW_LOG(TW_DEBUG,"twWs_SendMessage: Sent %d bytes using %d frames.", ptr - buf, framesSent);
	TW_LOG_HEX(buf, "Sent Message >>>>\n", ptr - buf);
	twMutex_Unlock(ws->sendMessageMutex);
	return TW_OK;
}

int twWs_SendPing(twWs * ws, char * msg) {
	char tmp[64];
	memset(tmp, 0, 64);
	if (!msg) {
		twGetSystemTimeString(tmp, "%H:%M:%S", 63, 0, 0);
		msg = tmp;
	} 
	return sendCtlFrame(ws, 0x09, msg);
}

int twWs_SendPong(twWs * ws, char * msg) {
	return sendCtlFrame(ws, 0x0A, msg);
}

/**
* Websocket helper functions
**/
int sendCtlFrame(twWs * ws, unsigned char type, char * msg) {
	/* Send a control frame */
	int bytesToWrite = 0;
	int bytesWritten = 0;
	int res = 0;
	char frameHeader[128];
	char typeStr[8] = "Unknown";
	if (type == 0x08) strcpy(typeStr,"Close");
	else if (type == 0x09) strcpy(typeStr,"Ping");
	else if (type == 0x0A) strcpy(typeStr,"Pong");

	if (!ws) { 
		TW_LOG(TW_ERROR, "sendCtlFrame: NULL ws pointer"); 
		return TW_INVALID_PARAM; 
	}
	if (!ws->isConnected) { 
		TW_LOG(TW_WARN, "sendCtlFrame: Not connected"); 
		return TW_WEBSOCKET_NOT_CONNECTED; 
	}

	/* Make sure we have a valid message */
	if (!msg) msg = typeStr;

	if (type < 0x08 && type > 0x0a) {
		TW_LOG(TW_ERROR,"sendCtlFrame: Invalid frame type: 0x%x", type);
		return TW_INVALID_WEBSOCKET_FRAME_TYPE;
	}
	if (strlen(msg) > 110) {
		TW_LOG(TW_ERROR,"sendCtlFrame: Message too long.  Length = ", strlen(msg));
		return TW_WEBSOCKET_MSG_TOO_LARGE;
	}
	twMutex_Lock(ws->sendFrameMutex);
	TW_LOG(TW_DEBUG,"sendCtlFrame: >>>>> Sending %s. Msg: %s", typeStr, msg);
	memset(frameHeader,0,6);
	frameHeader[0] = 0x80 + type;
	frameHeader[1] = 0x80 + (char)strlen(msg);
	/* Masking is set to 0x00 so nothing else to do */
	strcpy(frameHeader + 6, msg);
	bytesToWrite = strlen(msg) + 6;
	bytesWritten = twTlsClient_Write(ws->connection, frameHeader, bytesToWrite, 100);

	if (bytesWritten != bytesToWrite) {
		TW_LOG(TW_WARN,"sendCtlFrame: Error writing to socket.  Error: %d", twSocket_GetLastError());
		ws->isConnected = FALSE;
		restartSocket(ws);
		res = TW_ERROR_WRITING_TO_WEBSOCKET;
	}
	twMutex_Unlock(ws->sendFrameMutex);
	return res;
}

int sendDataFrame(twWs * ws, char * msg, uint16_t length, char isContinuation, char isFinal, char isText) {

	int bytesToWrite = 0;
	int bytesWritten = 0;
	char frameHeader[12];
	unsigned char headerLength = 6;
	char type = 0x02;  /* Default to Binary complete frame */

	/* Do some status checks */
	if (!ws) { 
		TW_LOG(TW_ERROR, "sendDataFrame: NULL ws pointer"); 
		return TW_INVALID_PARAM; 
	}
	if (!ws->isConnected) { 
		TW_LOG(TW_WARN, "sendDataFrame: Not connected"); 
		return TW_WEBSOCKET_NOT_CONNECTED; 
	}

	/* Make sure we have a message and it fits in a frame */
	if (!msg) { TW_LOG(TW_ERROR, "sendDataFrame: NULL msg pointer"); return -1; }
	if (ws->frameSize < length) { 
		TW_LOG(TW_WARN, "sendDataFrame: Frame of length %d is too large.  Max frame size is %u", 
		length, ws->frameSize); 
		return TW_WEBSOCKET_MSG_TOO_LARGE; 
	}

	twMutex_Lock(ws->sendFrameMutex);
	/* Figure out the type */
	if (isText) type = 0x01;
	if (isContinuation) type = 0x00;
	if (isFinal) type = type | 0x80;
	/* Prep the header */
	memset(frameHeader,0,12);
	frameHeader[0] = type;
	/* Set up the length */
	if (length < 126) frameHeader[1] = 0x80 + length;
	else {
		headerLength = 8;
		frameHeader[1] = (char)0xFE; /* (char)(0x80 + 126); */
		frameHeader[2] = (char)(length / 0x100);
		frameHeader[3] = (char)(length % 0x100);
	} 
	/* Masking is set to 0x00 so nothing else to do */
	bytesToWrite = headerLength + length;
	bytesWritten += twTlsClient_Write(ws->connection, frameHeader, headerLength, 100);
	bytesWritten += twTlsClient_Write(ws->connection, msg, length, 100);

	if (bytesWritten != bytesToWrite) {
		TW_LOG(TW_WARN,"sendDataFrame: Error writing to socket.  Error: %d", twSocket_GetLastError());
		ws->isConnected = FALSE;
                twMutex_Unlock(ws->sendFrameMutex);
		restartSocket(ws);
		return TW_ERROR_WRITING_TO_WEBSOCKET;
	}
	twMutex_Unlock(ws->sendFrameMutex);
	return TW_OK;
}

int validateAcceptKey(twWs * ws, const char * val) {
	char tmp[80];
	char hash[20];
	char expectedValue[40];
	if (!ws) { TW_LOG(TW_ERROR, "validateAcceptKey: NULL ws pointer"); return -1; }
	/* Calculate the expected value */
	strcpy(tmp, ws->security_key);
	strcat(tmp, "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
	calc(tmp, strlen(tmp), (unsigned char *)hash);
	memset(expectedValue, 0, 40);
	twBase64Encode(hash, 20, expectedValue, 40);
	/* Compare the received value */
	if (strcmp(val, expectedValue)) {
		TW_LOG(TW_ERROR,"validateAcceptKey: Keys don't match. Expected %s, Received %s",
			expectedValue, val);
		return TW_INVALID_ACCEPT_KEY;
	}
	return TW_OK;
}

int32_t handleDataFrame(http_parser* parser, const char *at, size_t length, char isText, char isContinuation) {
	/**
	*	Generic Data Frame handler
	*
	*	Returns 1 if an error, 0 if frame is processed successfully
	*
	**/
	twWs * ws = NULL;
	char res = 0x00;

	if (!parser || !parser->data) {
		TW_LOG(TW_DEBUG,"handleDataFrame: NULL parser or data value");
		return 1;
	}
	TW_LOG(TW_TRACE,"handleDataFrame: Received Frame.  Type = %d", res);
	ws = (twWs *)(parser->data);

	/**** twMutex_Lock(ws->recvMutex); ****/
	/* Sanity check to see if we have space */
	if (ws->messagePtr + length > ws->messageBuffer + ws->messageChunkSize) {
		TW_LOG(TW_ERROR, "handleDataFrame: Adding frame to buffer would exceed max message length");
		ws->messagePtr = ws->messageBuffer;
		/**** twMutex_Unlock(ws->recvMutex); ****/
		return 1;
	}
	/* Copy the frame data */
	memcpy(ws->messagePtr, at, length);
	ws->messagePtr += length;
	/* Figure out if this is a complete message or not */
	if (!parser->ws_fragment) {
		if (isText) {
			if (ws->on_ws_textMessage) (*ws->on_ws_textMessage)(ws, ws->messageBuffer, ws->messagePtr - ws->messageBuffer);
		} else {
			if (ws->on_ws_binaryMessage) (*ws->on_ws_binaryMessage)(ws, ws->messageBuffer, ws->messagePtr - ws->messageBuffer);
		}
		/* Rest our pointer to the beginning */
		ws->messagePtr = ws->messageBuffer;
	} 
	/**** twMutex_Unlock(ws->recvMutex); ****/
	return res;
}
