/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Portable twTLS Client  abstraction layer
 */

#include "twOSPort.h"
#include "twLogger.h"
#include "twTls.h"


int twTlsClient_Create(const char * host, int16_t port, uint32_t options, twTlsClient ** client) {
	TW_SSL_CTX *ssl_ctx = NULL;
	/**
    int cert_index = 0, ca_cert_index = 0;
    int cert_size, ca_cert_size;
    char **ca_cert, **cert;
    const char *password = NULL;
	**/
	twTlsClient * tls = NULL;

	TW_LOG(TW_DEBUG, "twTlsClient_Create: Initializing TLS Client");
	/****
    cert_size = TW_GET_CERT_SIZE;
    ca_cert_size = TW_GET_CA_CERT_SIZE;
    ca_cert = (char **)TW_CALLOC(1, sizeof(char *)*ca_cert_size);
    cert = (char **)TW_CALLOC(1, sizeof(char *)*cert_size);
	****/

	if (!client) return TW_INVALID_PARAM;
	tls = (twTlsClient *)TW_CALLOC(sizeof(twTlsClient),1);
	if (!tls) return TW_ERROR_ALLOCATING_MEMORY;
	/* Create the mutex */
	tls->mtx = twMutex_Create();
	if (!tls->mtx) return TW_ERROR_ALLOCATING_MEMORY;
	tls->options = options;
	tls->data_leftover = 0;
	tls->selfSignedOk = 0;
	tls->validateCert = TRUE;
	/*  Create our socket */
	tls->connection = twSocket_Create(host, port, 0);
	if (!tls->connection) {
		twTlsClient_Delete(tls);
		return TW_HOST_NOT_FOUND;
	}
	/* Create the ctx */
	if ((ssl_ctx = TW_NEW_SSL_CTX) == NULL){
		twTlsClient_Delete(tls);
		return TW_ERROR_CREATING_SSL_CTX;
	}
	tls->ctx = ssl_ctx;
	*client = tls;
	return TW_OK;
}

int twTlsClient_Connect(twTlsClient * t) {
	TW_SSL *ssl = NULL;
	if (!t) return -1;
	TW_LOG(TW_DEBUG, "twTlsClient_Connect: Connecting to server");
	if (twSocket_Connect(t->connection)) {
		TW_LOG(TW_ERROR,"Error intializing socket connection");
		 return TW_SOCKET_INIT_ERROR;
	}
	ssl = TW_NEW_SSL_CLIENT(t->ctx, t->connection, NULL, 0);
	t->ssl = ssl;
	if (TW_HANDSHAKE_SUCCEEDED) {
        if (t->validateCert && twTlsClient_ValidateCert(t)) {
			 TW_LOG(TW_ERROR,"twTlsClient_Connect: Error intializing TLS connection.  Invalid certificate");
			 return TW_INVALID_SSL_CERT;
        }
	 } else {
		TW_LOG(TW_ERROR,"Error intializing SSL connection");
		 return TW_SOCKET_INIT_ERROR;
 	 }
	 TW_LOG(TW_DEBUG, "twTlsClient_Connect: TLS connection established");
	 return TW_OK;
}

int twTlsClient_Reconnect(twTlsClient * t, const char * host, int16_t port) {
	/* Close out and delete the existing session and restart */
	twSocket * old = 0;
	TW_LOG(TW_DEBUG, "twTlsClient_Reconnect: Re-establishing SSL context");
	if (t->ssl) TW_SSL_FREE(t->ssl);
	if (t->connection){
		old = t->connection;
	}
	t->ssl = 0;
	t->connection = 0;
	t->connection = twSocket_Create(host, port, 0);
	if (!t->connection) {
		return TW_SOCKET_NOT_FOUND;
	}
	if (old) {
		if (old->proxyHost) twSocket_SetProxyInfo(t->connection, old->proxyHost, old->proxyPort, old->proxyUser, old->proxyPass);
		twSocket_Delete(old);
	}
	return twTlsClient_Connect(t);
}


int twTlsClient_Close(twTlsClient * t) {
	if (!t || !t->mtx) return TW_INVALID_PARAM;
	TW_LOG(TW_DEBUG, "twTlsClient_Close: Disconnecting from server");
	twMutex_Lock(t->mtx);
	twSocket_Close(t->connection);
	TW_LOG(TW_DEBUG, "twTlsClient_Close: Deleting SSL session");
	TW_SSL_FREE(t->ssl);
	t->ssl = 0;
	twMutex_Unlock(t->mtx);
	return TW_OK;
}

int twTlsClient_Read(twTlsClient * t, char * buf, int len, int timeout) {
	int bytesRead = 0;
	if (!t || !t->ssl || !t->mtx) return -1;
	if (!t->data_leftover && !twSocket_WaitFor(t->connection, timeout)) return 0;
	twMutex_Lock(t->mtx);
	bytesRead = TW_SSL_READ(t->ssl, buf, len, timeout);
	if (bytesRead > len) {
		t->data_leftover = 1;
		bytesRead = len;
	} else t->data_leftover = 0;
	twMutex_Unlock(t->mtx);
	return bytesRead;
}

int twTlsClient_Write(twTlsClient * t, char * buf, int len, int timeout) {
	int res = -1;
	if (!t || !t->ssl || !t->mtx) return -1;
	twMutex_Lock(t->mtx);
	res = TW_SSL_WRITE(t->ssl, buf, len);
	twMutex_Unlock(t->mtx);
	return res;
}

int twTlsClient_Delete(twTlsClient * t) {
	if (!t || !t->mtx) return TW_INVALID_PARAM;
	twTlsClient_Close(t);
	twMutex_Lock(t->mtx);	
	if (t->keypasswd) TW_FREE(t->keypasswd);
	if (t->extras) TW_FREE(t->extras);
	/* if (t->session) TW_FREE(t->session); */
	if (t->ssl) TW_SSL_FREE(t->ssl);
	if (t->ctx) TW_SSL_CTX_FREE( t->ctx);
	if (t->connection) twSocket_Delete(t->connection);
	twMutex_Unlock(t->mtx);
	twMutex_Delete(t->mtx);
	TW_FREE(t);
	return TW_OK;
}

void twTlsClient_SetSelfSignedOk(twTlsClient * t) {
	if (!t) return;
	t->selfSignedOk = 1;
}

void twTlsClient_DisableCertValidation(twTlsClient * t) {
	if (!t) return;
	t->validateCert = FALSE;
}

int twTlsClient_ValidateCert(twTlsClient * t) {
	if (!t) return 0;
	return TW_VALIDATE_CERT(t->ssl, t->selfSignedOk);
}

int twTlsClient_UseCertificateFile(twTlsClient * t, const char *file, int type) {
	if (!t) return -1;
	return TW_USE_CERT_FILE(t->ctx, file, type);
}

int twTlsClient_UsePrivateKeyFile(twTlsClient * t, const char *file, int type)  {
	if (!t) return -1;
	return TW_USE_KEY_FILE(t->ctx, file, type, t->keypasswd);
}

int	twTlsClient_UseCertificateChainFile(twTlsClient * t, const char *file, int type) {
	if (!t) return -1;
	return TW_USE_CERT_CHAIN_FILE(t->ctx, file, type);
}

int twTlsClient_SetClientCaList(twTlsClient * t, char * caFile)  {
	if (!t) return -1;
	return TW_SET_CLIENT_CA_LIST(t->ctx, caFile);
}

void twTlsClient_SetDefaultPasswdCbUserdata(twTlsClient * t, void *u) {
	if (t) t->keypasswd = (char *)u;
}


INLINE int16_t returnZero() { return 0; }
INLINE void * mallocByte() { return TW_MALLOC(1); }

