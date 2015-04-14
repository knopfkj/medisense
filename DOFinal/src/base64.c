/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Base 64 encode/decoe
 */

#include "base64.h"
#include "twOSPort.h"

const char * chars64 = {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};

int twBase64Encode(char *input, int iplen,  char *output, int oplen) {
	/* Base64 encode a string */
	int i, j = 0;
	for( i = 0; i < iplen; ++i )
    {
       unsigned char c = (char)( ( input[i] >> 2 ) & 0x3f );
       output[j++] = chars64[c];
	   if (j >= oplen) return TW_BASE64_ENCODE_OVERRUN;

       c = (char)( ( input[i] << 4 ) & 0x3f );
       if( ++i < iplen )
          c = (char)( c | (char)( ( input[i] >> 4 ) & 0x0f ) );
        output[j++] = chars64[c];
		if (j >= oplen) return TW_BASE64_ENCODE_OVERRUN;

       if( i < iplen ) {
          c = (char)( ( input[i] << 2 ) & 0x3c );
          if( ++i < iplen )
             c = (char)( c | (char)( ( input[i] >> 6 ) & 0x03 ) );
           output[j++] = chars64[c];
       } else {
          ++i;
           output[j++] = '=';
       }
	   if (j >= oplen) return TW_BASE64_ENCODE_OVERRUN;

       if( i < iplen ) {
          c = (char)( input[i] & 0x3f );
           output[j++] = chars64[c];
        } else {
           output[j++] = '=';
        }
		if (j >= oplen) return TW_BASE64_ENCODE_OVERRUN;
    }
	return TW_OK;
}

#define ng  (char)-1
char table64vals[80] =
{
	62, ng, ng, ng, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, ng, ng, ng, ng, ng,
	ng, ng,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17,
	18, 19, 20, 21, 22, 23, 24, 25, ng, ng, ng, ng, ng, ng, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};

INLINE char table64( unsigned char c )
{
	return ( c < 43 || c > 122 ) ? ng : table64vals[c-43];
}

int twBase64Decode(char *input, int iplen, char *output, int oplen) {

	/* Try to do the Base64 decoding */
	int i, j = 0;
	char d;
    for( i = 0; i < iplen; ++i ) {
      char c = table64(input[i]);
      ++i;
	  if (i >= iplen) return TW_OK; 
      d = table64(input[i]);
      c = (char)( ( c << 2 ) | ( ( d >> 4 ) & 0x3 ) );
      output[j++] = c;
	  if (j >= oplen) return -1;
      if( ++i < iplen ) {
         c = input[i];
         if( '=' == c ) break;

         c = table64(input[i]);
         d = (char)( ( ( d << 4 ) & 0xf0 ) | ( ( c >> 2 ) & 0xf ) );
         output[j++] = d;
		 if (j >= oplen) return TW_BASE64_DECODE_OVERRUN;
      }
      if( ++i < iplen ) {
         d = input[i];
         if( '=' == d ) break;
  	     d = table64(input[i]);
         c = (char)( ( ( c << 6 ) & 0xc0 ) | d );
         output[j++] = c;
		 if (j >= oplen) return TW_BASE64_DECODE_OVERRUN;
		}
    }
	return TW_OK;
}

