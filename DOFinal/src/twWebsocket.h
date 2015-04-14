/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Portable Websocket Client  abstraction layer
 */
#include "twOSPort.h"
#include "twDefinitions.h"

#ifndef TW_WEBSOCKET_H
#define TW_WEBSOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************/
/*      Websockets API        */
/******************************/
/*
HTTP Parser declarations for the 
joyent http-parser library
*/
struct http_parser;
struct http_parser_settings;

/* 
Forward declarations of the struct and call back functions 
used by the http-parser library
*/
struct twWs;
typedef int (*ws_cb) (struct twWs * ws);
typedef int (*ws_data_cb) (struct twWs * ws, const char *at, size_t length);

/* 
Helper macros 
*/
#define WS_TLS_CONN(a) (twTlsClient *)a->connection

/******************************/
/*  Websocket Close Reasons   */
/******************************/
enum close_status {
	 SERVER_CLOSED = 0
	,NORMAL_CLOSE = 1000
	,GOING_TO_SLEEP
	,PROTOCOL_ERROR
	,UNSUPPORTED_DATA_TYPE
	,RESERVED1
	,RESERVED2
	,RESERVED3
	,INVALID_DATA
	,POLICY_VIOLATION
	,FRAME_TOO_LARGE
	,NO_EXTENSION_FOUND
	,UNEXPECTED_CONDITION
};

/******************************/
/*  Websocket entity struct   */
/******************************/
typedef struct twWs{
	struct twTlsClient * connection;
	uint32_t messageChunkSize;
	uint16_t frameSize;
	char * messageBuffer;
	char * messagePtr;
	char * frameBuffer;
	char * frameBufferPtr;
	char * host;
	uint16_t port;
	char * api_key;
	char * gatewayName;
	char * security_key;
	uint32_t sessionId;
	char * resource;
	TW_MUTEX sendMessageMutex;
	TW_MUTEX sendFrameMutex;
	TW_MUTEX recvMutex;
	signed char connect_state;
	signed char isConnected;
	struct http_parser * parser;
	struct http_parser_settings * settings;
	ws_cb on_ws_connected;
	ws_data_cb on_ws_binaryMessage;
	ws_data_cb on_ws_textMessage;
	ws_data_cb on_ws_ping;
	ws_data_cb on_ws_pong;
	ws_data_cb on_ws_close;
} twWs;


/******************************/
/*  Websocket entity struct   */
/*  manipulation functions    */
/******************************/
/*
twWs_Create - Create a Websocket struct and the underlying dependent components.  This
	does NOT automatically establish a connection.
Parameters:
    host - the hostname of the websocket server that will be connected to
	port - the port that the websocket server is listening to
	resource - the HTTP resource to use when establishing the connection
	api_key - the api key that will be used during an ensuing authentication process
	gatewayName - an optional name if the SDK is being used to develop a gateway application which
		allows multiple Things to connect through it. If not NULL this is used during the Binding process.
	messageChunkSize - max size (in bytes) of multipart message chunk.
	frameSize - maximum websocket frame size.  Not to be confused with the max size of ThingWorx message.
    entity - pointer to a twWs pointer.
Return:
	int - 0 if successful, positive integral error code (see twErrors.h) if an was encountered.
*/
int twWs_Create(char * host, uint16_t port, char * resource, char * api_key, char * gatewayName,
				   uint32_t messageChunkSize, uint16_t frameSize, twWs ** entity);

/*
twWs_Delete - delete a twWs structure and any dependent components.  Will shut down the connection first.
Parameters:
	ws - the websocket structure to operate on
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twWs_Delete(twWs * ws);

/*
twWs_Connect - establish a web socket connection to the server
Parameters:
	ws - the websocket structure to operate on
	timeout - the amount of time (in milliseconds) to wait for the connection to be established
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twWs_Connect(twWs * ws, uint32_t timeout);

/*
twWs_Disconnect - send a close and siconnect from the server
Parameters:
	ws - the websocket structure to operate on
	code - the close status to use
	reason - the reason string to be sent with the close
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twWs_Disconnect(twWs * ws, enum close_status code, char * reason);

/*
twWs_IsConnected - return the connection status of the websocket
Parameters:
	ws - the websocket structure to operate on
Return:
	char - boolean, TRUE if connected, FALSE if not
*/
char twWs_IsConnected(twWs * ws);

/*
twWs_RegisterConnectCallback - register a function to be called when the
	websocket is successfully connected
Parameters:
	ws - the websocket structure to operate on
	cb - pointer to the function to call
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twWs_RegisterConnectCallback(twWs * ws, ws_cb cb);

/*
twWs_RegisterCloseCallback - register a function to be called when the
	websocket is closed by the server
Parameters:
	ws - the websocket structure to operate on
	cb - pointer to the function to call
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twWs_RegisterCloseCallback(twWs * ws, ws_data_cb cb);

/*
twWs_RegisterBinaryMessageCallback - register a function to be called when the
	websocket receives a complete binary message
Parameters:
	ws - the websocket structure to operate on
	cb - pointer to the function to call
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twWs_RegisterBinaryMessageCallback(twWs * ws, ws_data_cb cb);

/*
twWs_RegisterTextMessageCallback - register a function to be called when the
	websocket receives a complete text message
Parameters:
	ws - the websocket structure to operate on
	cb - pointer to the function to call
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twWs_RegisterTextMessageCallback(twWs * ws, ws_data_cb cb);

/*
twWs_RegisterPingCallback - register a function to be called when the
	websocket receives a Ping message
Parameters:
	ws - the websocket structure to operate on
	cb - pointer to the function to call
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twWs_RegisterPingCallback(twWs * ws, ws_data_cb cb);

/*
twWs_RegisterPongCallback - register a function to be called when the
	websocket receives a Pong message
Parameters:
	ws - the websocket structure to operate on
	cb - pointer to the function to call
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twWs_RegisterPongCallback(twWs * ws, ws_data_cb cb);

/*
twWs_Receive - check the websocket for data and drive the state machine of the websocket.  
	This function must be called on a regular basis.  No data is returned as the data
	is delivered thorugh the state machine callback functions.
Parameters:
	ws - the websocket structure to operate on
	timeout - time (in msec) to wait for data on the socket
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twWs_Receive(twWs * ws, uint32_t timeout);

/*
twWs_SendMessage - send a message over the websocket.  Message will be broken up
	into a series of multipart messages if needed.
Parameters:
	ws - the websocket structure to operate on
	buf - pointer to the buffer containing the message
	length - length of the message
	isText - boolean, if TRU send as a text message, if FALSE send as binary
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twWs_SendMessage(twWs * ws, char * buf, uint32_t length, char isText);

/*
twWs_SendPing - send a Ping message over the websocket.  Message data MUST be NULL terminated.
Parameters:
	ws - the websocket structure to operate on
	msg - NULL terminated string to send with the Ping.  Must be less than 126 bytes
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twWs_SendPing(twWs * ws, char * msg);

/*
twWs_SendPong - send a Pong message over the websocket.  Message data MUST be NULL terminated.
Parameters:
	ws - the websocket structure to operate on
	msg - NULL terminated string to send with the Ping.  Must be less than 126 bytes
Return:
	int - 0 if success, non-zero if a failure occured.
*/
int twWs_SendPong(twWs * ws, char * msg);

#ifdef __cplusplus
}
#endif

#endif



