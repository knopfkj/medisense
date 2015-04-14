/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Default settings for C SDK
 */

#ifndef TW_DEFAULT_SETTINGS_H
#define TW_DEFAULT_SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

/********************************/
/*   General API Settings       */
/********************************/
/* 
To minimize code footprint, by default logging is enabled for debug builds 
and disabled for release builds.  Use this to over-ride that setting.
*/
/*#define DBG_LOGGING*/ 

/* 
The resource portion of the ThingWorx websocket uri.
*/
#define TW_URI			       "/Thingworx/WS"		

/* 
The maximum size of a complete message whether it is broken up as a
multipart message or not.  Messages larger than this will be rejected.
Measured in Bytes.
*/
#define MAX_MESSAGE_SIZE			16384		

/* 
The maximum size of a message chunk.  Messages large than this will
be broken up into a multipart message.  Measured in Bytes.  This value
should be the same as the server side configuration which defaults to 8192.
*/
#define MESSAGE_CHUNK_SIZE          8192       

/* 
Time to wait for a response to a message from the server. Measured
in milliseconds.
*/
#define DEFAULT_MESSAGE_TIMEOUT		10000		

/* 
Websocket keep alive rate.  Used to ensure the connection stays open.  Measured
in milliseconds.  This value should never be greater than the server side setting
which defaults to 60000 milliseconds.
*/
#define PING_RATE					55000	    

/* 
Periodic cleanup rate for multipart messages that never receive all of
the expected number of message chunks.  Measured in milliseconds.
*/
#define STALE_MSG_CLEANUP_RATE		 (DEFAULT_MESSAGE_TIMEOUT * 5)  

/* 
Time to wait for the websocket connection to be established.  Measured in milliseconds.
*/
#define CONNECT_TIMEOUT				10000		

/* 
Number of retries used to establish a websocket connect.  The twApi_Connect call
returns an error after the retries are exhausted.
*/
#define CONNECT_RETRIES				3

/* 
"ON" time of the duty cycle modulated AlwaysOn connection.  Acceptable values are 0-100%.
A value of 100% means the connection always stays alive.
*/
#define DUTY_CYCLE					20			

/* 
Period of the duty cycle modulated AlwaysOn connection measured in milliseconds.  A value 
of 0 means the connections always stays alive.  It is recommended that this value be greater
than 10 seconds at a minimum.
*/
#define DUTY_CYCLE_PERIOD			0			

/* 
Incremental block size for dynamically allocated stream (byte array) variables.  When adding
bytes to a stream, this is the size of memory allocated if more memory is needed.
*/
#define STREAM_BLOCK_SIZE			256

/* 
Maximum number of tasks allowed for the built in round robin task execution engine.
*/
#define TW_MAX_TASKS 5

/*
Enable property folding of Managed properties
*/
#define MANAGED_PROPERTY_FOLDING FALSE

/*
Maximun size of Managed Property update queue (in Bytes)
*/
#define MAX_MANAGED_PROPERTY_Q_SIZE 2048

/*
File transfer Block size (in Bytes)
*/
#define FILE_XFER_BLOCK_SIZE 8000

/*
File transfer Max file size (in Bytes)
*/
#define FILE_XFER_MAX_FILE_SIZE 8000000000

/*
File transfer MD5 Buffer size (in Bytes - shouldbe multiple of 64)
*/
#define FILE_XFER_MD5_BLOCK_SIZE 6400

/*
File transfer timeout for stalled transfers (in milliseconds)
*/
#define FILE_XFER_TIMEOUT 30000

/*
File transfer staging directory for received files
*/
#define FILE_XFER_STAGING_DIR "/opt/thingworx/tw_staging"

/*
Offline message queue max size
*/
#define OFFLINE_MSG_QUEUE_SIZE 16384

/*
Offline message store file
*/
#define OFFLINE_MSG_STORE_DIR "/opt/thingworx"

#ifdef __cplusplus
}
#endif

#endif
