/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  HTTP Proxy Connection
 */

#ifndef HTTP_PROXY_H
#define HTTP_PROXY_H

#include "twOSPort.h"


#ifdef __cplusplus
extern "C" {
#endif

/*
connectToProxy - Opens the socket and connects through the proxy specified in the twSocket struct. 
Parameters:
    s - pointer to the twSocket to connect
	authCredentials - if NULL will use the credentials in the twSocket structure.  User should never pass non-NULL value.
Return:
	int - zero if successful, non-zero if an error occurred
*/
int connectToProxy(twSocket * s, char * authCredentials);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_PROXY_H */
