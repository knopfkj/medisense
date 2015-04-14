/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Base 64 encode/decod
 */

#ifndef TW_BASE64_H
#define TW_BASE64_H

#ifdef __cplusplus
extern "C" {
#endif

int twBase64Encode(char *input, int iplen, char *output, int oplen);
int twBase64Decode(char *input, int iplen, char *output, int oplen);

#ifdef __cplusplus
}
#endif

#endif
