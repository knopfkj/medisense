/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Portable TLS Client  abstraction layer
 */

#ifndef TW_TLS_H
#define TW_TLS_H

#include "twOSPort.h"
#include "twErrors.h"

#include TW_TLS_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

typedef struct twTlsClient {
	twSocket * connection;
	TW_SSL_CTX * ctx;
	TW_SSL * ssl;
	/* void * session; */
	uint32_t options;
	void * extras;
	char * keypasswd;
	uint8_t * read_buf;
	char data_leftover;
	char selfSignedOk;
	char validateCert;
	TW_MUTEX mtx;
} twTlsClient;

int twTlsClient_Create(const char * host, int16_t port, uint32_t options, twTlsClient ** client);
int twTlsClient_Connect(twTlsClient * t);
int twTlsClient_Reconnect(twTlsClient* t, const char * host, int16_t port);
int twTlsClient_Close(twTlsClient * t);
int twTlsClient_Read(twTlsClient * t, char * buf, int len, int timeout);
int twTlsClient_Write(twTlsClient * t, char * buf, int len, int timeout);
int twTlsClient_Delete(twTlsClient * t);

void twTlsClient_SetSelfSignedOk(twTlsClient * t);
void twTlsClient_DisableCertValidation(twTlsClient * t);
int twTlsClient_ValidateCert(twTlsClient * t);
int twTlsClient_UseCertificateFile(twTlsClient * t, const char *file, int type);
int twTlsClient_UsePrivateKeyFile(twTlsClient * t, const char *file, int type);
int	twTlsClient_UseCertificateChainFile(twTlsClient * t, const char *file, int type);
int twTlsClient_SetClientCaList(twTlsClient * t, char * caFile);
void twTlsClient_SetDefaultPasswdCbUserdata(twTlsClient * t, void *u);

#ifdef __cplusplus
}
#endif

#endif
