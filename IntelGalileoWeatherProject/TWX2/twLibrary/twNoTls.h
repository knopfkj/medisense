/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  NoTLS Client wrapper layer
 */

#ifndef NO_TLS_H
#define NO_TLS_H

#include "twOSPort.h"

#ifdef __cplusplus
extern "C" {
#endif

int16_t returnZero();
void * mallocByte();

#define TW_SSL_CTX						void
#define TW_NEW_SSL_CTX					mallocByte()

#define TW_SSL							twSocket
#define TW_NEW_SSL_CLIENT(a,b,c,d)		b

#define TW_SSL_SESSION_ID_SIZE			0
#define TW_GET_CERT_SIZE				0
#define TW_GET_CA_CERT_SIZE				0			
#define TW_HANDSHAKE_SUCCEEDED			(1)
#define TW_SSL_FREE(a)					returnZero()
#define TW_SSL_CTX_FREE(a)				returnZero()
#define TW_SSL_WRITE(a,b,c)				twSocket_Write(a, b, c, 0)
#define TW_USE_CERT_FILE(a,b,c)			returnZero()
#define TW_USE_KEY_FILE(a,b,c,d)		returnZero()
#define TW_USE_CERT_CHAIN_FILE(a,b,c)	returnZero()
#define TW_SET_CLIENT_CA_LIST(a,b)		returnZero()
#define TW_SSL_READ(a,b, c, d)			twSocket_Read(a, b, c, d)
#define TW_VALIDATE_CERT(a,b)			returnZero()


#ifdef __cplusplus
}
#endif

#endif /* NO_TLS_H  */
