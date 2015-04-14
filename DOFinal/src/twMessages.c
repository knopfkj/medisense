/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Portable ThingWorx Binary Messaging layer
 */

#include "twOSPort.h"
#include "twMessages.h"
#include "twLogger.h"
#include "list.h"
#include "stringUtils.h"
#include "twBaseTypes.h"
#include "twInfoTable.h"
#include "twApi.h"

#define MSG_HEADER_SIZE 15
#define MULTIPART_MSG_HEADER_SIZE 6

extern TW_MUTEX twInitMutex;

uint32_t globalRequestId = 0;

twMessage * twMessage_Create(enum msgCodeEnum code, uint32_t reqId) {
	twMessage * msg = (twMessage *)TW_CALLOC(sizeof(twMessage), 1);
	if (!msg) { TW_LOG(TW_ERROR, "twMessage_Create: Error allocating msg storage"); return NULL; }
	msg->version = TW_MSG_VERSION;
	msg->code = code;
	msg->length = MSG_HEADER_SIZE;
	if (!reqId) {
		twMutex_Lock(twInitMutex);
		msg->requestId = ++globalRequestId;
		twMutex_Unlock(twInitMutex);
	}
	return msg;
}

twMessage * twMessage_CreateRequestMsg(enum msgCodeEnum code) {
	twMessage * msg = 0;
	if (code != TWX_GET && code != TWX_PUT && code != TWX_POST && code != TWX_DEL) {
		TW_LOG(TW_ERROR, "twMessage_CreateRequestMsg: Not  valid code for a request messge"); 
		return NULL;
	}
	msg = twMessage_Create(code, 0);
	if (msg) {
		msg->type = TW_REQUEST;
		msg->body = twRequestBody_Create();
		if (!msg->body) {
			TW_LOG(TW_ERROR, "twMessage_CreateRequestMsg: Error allocating body"); 
			twMessage_Delete(msg);
			return NULL;
		}
	}
	return msg;
}

twMessage * twMessage_CreateResponseMsg(enum msgCodeEnum code, uint32_t id)  {
	twMessage * msg = NULL;
	if (code < TWX_SUCCESS) {
		TW_LOG(TW_ERROR, "twMessage_CreateResponseMsg: Not valid code for a response messge"); 
		return NULL;
	}
	msg = twMessage_Create(code, 0);
	if (msg) {
		msg->type = TW_RESPONSE;
		msg->code = code;
		msg->requestId = id;
		msg->body = twResponseBody_Create();
		if (!msg->body) {
			TW_LOG(TW_ERROR, "twMessage_CreateResponseMsg: Error allocating body"); 
			twMessage_Delete(msg);
			return NULL;
		}
	}
	return msg;
}

twMessage * twMessage_CreateBindMsg(char * name, char isUnbind)  {
	twBindBody * b = NULL;
	twMessage * msg = twMessage_Create(isUnbind ? TWX_UNBIND : TWX_BIND, 0);
	if (msg) {
		msg->type = TW_BIND;
		b = twBindBody_Create(name);
		if (!b) {
			twMessage_Delete(msg);
			return NULL;
		} 
		twMessage_SetBody(msg, b);
	}
	return msg;
}

twMessage * twMessage_CreateAuthMsg(char * claimName, char * claimValue)  {
	twAuthBody * b = NULL;
	twMessage * msg = twMessage_Create(TWX_AUTH, 0);
	if (msg) {
		msg->type = TW_AUTH;
		b = twAuthBody_Create();
		if (!b) {
			twMessage_Delete(msg);
			return NULL;
		} 
		twMessage_SetBody(msg, b);
		twAuthBody_SetClaim(b, claimName, claimValue);
	}
	return msg;
}

twMessage * twMessage_CreateFromStream(twStream * s) {
	twMessage * msg;
	unsigned char code;
	if (!s) {
		TW_LOG(TW_ERROR, "twMessage_CreateFromStream: NULL stream pointer"); 
		return 0; 
	}
	msg = (twMessage *)TW_CALLOC(sizeof(twMessage), 1);
	if (!msg) { TW_LOG(TW_ERROR, "twMessage_CreateFromStream: Error allocating msg storage"); return 0; }
	twStream_Reset(s);
	twStream_GetBytes(s, (char *)&msg->version, 1);
	twStream_GetBytes(s, &code, 1);
	msg->code = (enum msgCodeEnum)code;
	twStream_GetBytes(s, &msg->requestId, 4);
	swap4bytes((char *)&msg->requestId);
	twStream_GetBytes(s, &msg->endpointId, 4);
	swap4bytes((char *)&msg->endpointId);
	twStream_GetBytes(s, &msg->sessionId, 4);
	swap4bytes((char *)&msg->sessionId);
	twStream_GetBytes(s, &msg->multipartMarker, 1);
	msg->length = MSG_HEADER_SIZE;
	if (msg->code == TWX_GET || msg->code == TWX_PUT || msg->code == TWX_POST || msg->code == TWX_DEL) {
		msg->type = TW_REQUEST;
		if (!msg->multipartMarker) {
			msg->body = twRequestBody_CreateFromStream(s);
			msg->length += ((twRequestBody *)(msg->body))->length;
		} else {
			msg->type = TW_MULTIPART_REQ;
			msg->body = twMultipartBody_CreateFromStream(s, TRUE);
			msg->length += ((twMultipartBody *)(msg->body))->length;
		}
	} else if (msg->code == TWX_AUTH) {
		msg->type = TW_AUTH;
		msg->body = twAuthBody_CreateFromStream(s);
		msg->length += ((twRequestBody *)(msg->body))->length;
	} else if ((msg->code == TWX_BIND) || (msg->code == TWX_UNBIND)) {
		msg->type = TW_BIND;
		msg->body = twBindBody_CreateFromStream(s);
		msg->length += ((twRequestBody *)(msg->body))->length;
	} else if (msg->code >= TWX_SUCCESS) {
		msg->type = TW_RESPONSE;
		if (!msg->multipartMarker) {
			msg->body = twResponseBody_CreateFromStream(s);
			msg->length += ((twResponseBody *)(msg->body))->length;
		} else {
			msg->type = TW_MULTIPART_RESP;
			msg->body = twMultipartBody_CreateFromStream(s, FALSE);
			msg->length += ((twMultipartBody *)(msg->body))->length;
		}
	} else {
		TW_LOG(TW_ERROR,"twMessage_CreateFromStream: Unhandled message code: %d", msg->code);
		twMessage_Delete(msg);
		return NULL;
	}
	return msg;
}

void twMessage_Delete(void * input) {
	char * type;
	twMessage * msg = (twMessage *) input;
	if (!msg) { TW_LOG(TW_ERROR, "twMessage_Delete: NULL msg pointer"); return; }
	/* Clean up */
	/* if there already is a body, free it up */
	if (msg->type == TW_REQUEST) {
		if (!msg->multipartMarker) {
			type = "REQUEST";
			twRequestBody_Delete((twRequestBody *)msg->body);
		} else {
			twMultipartBody_Delete(msg->body);
			type = "MULTIPART REQUEST";
		}
	} else if (msg->type == TW_BIND) {
		twBindBody_Delete((twBindBody *)msg->body);
		type = "BIND";
	} else if (msg->type == TW_AUTH) {
		twAuthBody_Delete((twAuthBody *)msg->body);
		type = "AUTH";
	} else if (msg->type >= TW_RESPONSE) {
		if (!msg->multipartMarker) {
			type = "RESPONSE";
			twResponseBody_Delete((twResponseBody *)msg->body);
		} else {
			twMultipartBody_Delete(msg->body);
			type = "MULTIPART RESPONSE";
		}
	} else {
		TW_LOG(TW_ERROR,"twMessage_Delete: Unknown message code: %d", msg->code);
		TW_FREE(msg->body);
		type = "UNKNOWN";
	}
	TW_LOG(TW_DEBUG, "twMessage_Delete:  Deleting %s Message: %d", type, msg->requestId);
	TW_FREE(msg);
	return;
}

int twMessage_SetBody(struct twMessage * msg, void * body) {
	if (!msg) { TW_LOG(TW_ERROR, "twMessage_SetBody: NULL msg pointer"); return -1; }
	/* if there already is a body, free it up */
	if (msg->body) {
		if (msg->type == TW_REQUEST) {
			twRequestBody_Delete((twRequestBody *)msg->body);
		} else if (msg->type == TW_BIND) {
			twBindBody_Delete((twBindBody *)msg->body);
		} else if (msg->type == TW_AUTH) {
			twAuthBody_Delete((twAuthBody *)msg->body);
		} else if (msg->type == TW_RESPONSE) {
			twResponseBody_Delete((twResponseBody *)msg->body);
		} else {
			TW_LOG(TW_ERROR,"twMessage_SetBody: Unknown message code: %d", msg->code);
			TW_FREE(body);
			return TW_INVALID_MSG_CODE;
		}
	}
	msg->body = body;
	return TW_OK;
}

extern twApi * tw_api;
#define PERSISTED_MSG_SEPERATOR "!twMsg!"

int twMessage_Send(struct twMessage * msg, struct twWs * ws) {
	char byte;
	char header[MSG_HEADER_SIZE + MULTIPART_MSG_HEADER_SIZE];
	twStream * s = NULL;
	twStream * bodyStream = NULL;
	uint32_t length = 0;
	uint32_t bodyBytesRemaining;
	uint32_t tmp;
	uint16_t numChunks = 0;
	uint16_t chunkNumber = 1;
	int res = 0;
	char headerSize = MSG_HEADER_SIZE;
	uint16_t effectiveChunkSize = MESSAGE_CHUNK_SIZE - MSG_HEADER_SIZE - MULTIPART_MSG_HEADER_SIZE;

	/* Check to see if offline message store is enabled, we have some queued up messages, and we are online */
	if (tw_api->offlineMsgEnabled && tw_api->offlineMsgSize && twApi_isConnected()) {
		int res = 0;
		/* We are going to have to insert the current session ID into the message */
		uint32_t session = ws->sessionId;
		swap4bytes((char *)&session);
	
	#if (OFFLINE_MSG_STORE == 1) 
		/* Memory resident offline message store */
		if (tw_api->offlineMsgList) {
			ListEntry * le = NULL;
			le = twList_Next(tw_api->offlineMsgList, NULL);
			while (le && le->value) {
				ListEntry * tmp = le;
				twStream * ps = (twStream *)le->value;
				/* Insert our session ID */
				memcpy(&ps->data[10], (char *)&session, 4);
				res = twWs_SendMessage(ws, twStream_GetData(ps), twStream_GetLength(ps), 0);
				tw_api->offlineMsgSize -= ps->length;
				/* if we have disconnected again stop processing the list, otherwise delete the message */
				if (res && res == TW_WEBSOCKET_NOT_CONNECTED) break;
				le = twList_Next(tw_api->offlineMsgList, le);
				twList_Remove(tw_api->offlineMsgList, tmp, TRUE);
			}
		}
	#endif
	#if (OFFLINE_MSG_STORE == 2)
		/* Persisted offline message store */
		if (tw_api->offlineMsgFile) {
			size_t persistMsgLen = MESSAGE_CHUNK_SIZE + strlen(PERSISTED_MSG_SEPERATOR) + sizeof(s->length);
			char * buf = (char *) TW_CALLOC(persistMsgLen,1);
			int32_t bytesRead = 0;
			int res = 0;
			char foundMsg = FALSE;
			uint32_t msgLength = 0;
			int64_t curPos = 0;
			TW_FILE_HANDLE f = 0;
			if (buf) {
				/* Find the first offline message seperator in the file */
				f = TW_FOPEN(tw_api->offlineMsgFile, "rb");
				while (TW_FREAD(buf + curPos, 1, 1, f) == 1) {
					if (!strstr(buf, PERSISTED_MSG_SEPERATOR)) {
						if (curPos++ < persistMsgLen) continue;
						else {
							TW_LOG(TW_WARN,"twMessage_Send: Couldn't find a persisted message seperator in the offline msg store file");
							break;
						}
					}
					/* If we are here we have found the separator */
					foundMsg = TRUE;
					/* Read in the length */
					msgLength = 0;
					if (TW_FREAD(&msgLength, 1, sizeof(uint32_t), f) != sizeof(uint32_t)) {
						TW_LOG(TW_ERROR,"twMessage_Send: Error reading persisted message length from file");
						break;
					}
					bytesRead = TW_FREAD(buf, 1, msgLength, f);
					if (bytesRead != msgLength) {
						TW_LOG(TW_ERROR,"twMessage_Send: Error reading persisted message from file");
						break;
					}
					curPos = TW_FTELL(f);
					/* Insert our session ID */
					memcpy(&buf[10], (char *)&session, 4);
					/* we now have the msg in the buffer.  Send it off */
					res = twWs_SendMessage(ws, buf, msgLength, 0);
					if (res && res == TW_WEBSOCKET_NOT_CONNECTED) {
						/* Back off our current position to point to the beginning of this msg again */
						curPos = curPos - msgLength - strlen(PERSISTED_MSG_SEPERATOR) + sizeof(s->length);
						break;
					}
					tw_api->offlineMsgSize -= msgLength;
					/* Rest curPos to check for another message */
					curPos = 0;
				}
				/* Need to clean up our file now */
				if (tw_api->offlineMsgSize <= 0 || curPos <= 1) {
					TW_FCLOSE(f);
					f = TW_FOPEN(tw_api->offlineMsgFile,"w");
					if (f) TW_FCLOSE(f);
					tw_api->offlineMsgSize = 0;
				} else {
					char * tmpFile = (char *)TW_CALLOC(strlen(tw_api->offlineMsgFile) + 8,1);
					TW_FILE_HANDLE h = 0;
					if (tmpFile) {
						strcpy(tmpFile, tw_api->offlineMsgFile);
						strcat(tmpFile, ".tmp");
						h = TW_FOPEN(tmpFile,"wb");
						if (h) {
							TW_FSEEK(f, curPos, SEEK_SET);
							while (bytesRead = TW_FREAD(buf, 1, persistMsgLen, f) > 0) {
								TW_FWRITE(buf, 1, bytesRead, h);
							}
							TW_FCLOSE(f);
							TW_FCLOSE(h);
							/* Rename the file */
							twDirectory_MoveFile(tmpFile, tw_api->offlineMsgFile);
						} else {
							TW_LOG(TW_ERROR,"twMessage_Send: Opening file %s", tmpFile);
						}
						TW_FREE(tmpFile);
					} else {
						TW_LOG(TW_ERROR,"twMessage_Send: Error allocating temp filename");
						TW_FCLOSE(f);
					}
				}
				TW_FREE(buf);
			}
		}
	#endif
	}

	if (!msg || !msg->body || !ws) { TW_LOG(TW_ERROR, "twMessage_Send: NULL msg pointer"); return -1; }
	/* Get the length */
	if (msg->type == TW_REQUEST) {
		length = ((twRequestBody *)(msg->body))->length;
		effectiveChunkSize -= 2 + strlen(((twRequestBody *)msg->body)->entityName);
	} else if (msg->type == TW_BIND) {
		length = ((twBindBody *)(msg->body))->length;
		if (ws->gatewayName) length += (strlen(ws->gatewayName) + 1 +strlen("SDKGateway") + 1);
	} else if (msg->type == TW_AUTH) {
		length = ((twAuthBody *)(msg->body))->length;
	} else if (msg->type == TW_RESPONSE) {
		length = ((twResponseBody *)(msg->body))->length;
	} else {
		TW_LOG(TW_ERROR,"twMessage_Send: Unknown message code: %d", msg->code);
		return TW_INVALID_MSG_TYPE;
	}
	/* Create a new stream for the body */
	bodyStream = twStream_Create();
	if (!bodyStream) {
		TW_LOG(TW_ERROR, "twMessage_Send: Error allocating stream"); 
		return TW_ERROR_ALLOCATING_MEMORY; 
	}
	/* Create the header binary representation */
	header[0] = msg->version;
	header[1] = (char)msg->code;
	tmp = msg->requestId;
	swap4bytes((char *)&tmp);
	memcpy(&header[2], (char *)&tmp, 4);
	tmp = msg->endpointId;
	swap4bytes((char *)&tmp);
	memcpy(&header[6], (char *)&tmp, 4);
	if (ws->sessionId) msg->sessionId = ws->sessionId;
	tmp = msg->sessionId;
	swap4bytes((char *)&tmp);
	memcpy(&header[10], (char *)&tmp, 4);
	/* Check our message size */
	if (length + MSG_HEADER_SIZE > MESSAGE_CHUNK_SIZE) msg->multipartMarker = TRUE;
	header[14] = msg->multipartMarker;
	/* Log this message before we chunk it up */
	TW_LOG_MSG(msg, "Sending Msg >>>>>>>>>");
	/* Add the beginning of the body */
	numChunks = length/effectiveChunkSize + 1;
	if (msg->multipartMarker) {
		unsigned char chunkInfo[6];
		chunkInfo[0] = (unsigned char)(chunkNumber / 256);
		chunkInfo[1] = (unsigned char)(chunkNumber % 256);
		chunkInfo[2] = (unsigned char)(numChunks / 256);
		chunkInfo[3] = (unsigned char)(numChunks % 256);
		chunkInfo[4] = (unsigned char)(MESSAGE_CHUNK_SIZE / 256);
		chunkInfo[5] = (unsigned char)(MESSAGE_CHUNK_SIZE % 256);
		memcpy(&header[MSG_HEADER_SIZE], chunkInfo, 6);
		headerSize += MULTIPART_MSG_HEADER_SIZE;
	}
	if (msg->type == TW_REQUEST) {
		twRequestBody_ToStream((twRequestBody *)msg->body, bodyStream);
	} else if (msg->type == TW_BIND) {
		twBindBody_ToStream((twBindBody *)msg->body, bodyStream, ws->gatewayName);
	} else if (msg->type == TW_AUTH) {
		twAuthBody_ToStream((twAuthBody *)msg->body, bodyStream);
	} else if (msg->type >= TW_RESPONSE) {
		twResponseBody_ToStream((twResponseBody *)msg->body, bodyStream);
	} 
	/* Start sending the message */
	bodyBytesRemaining = length;
	while (chunkNumber <= numChunks) {
		/* Create a new stream for the body */
		uint16_t size = effectiveChunkSize;
		s = twStream_Create();
		if (!s) {
			TW_LOG(TW_ERROR, "twMessage_Send: Error allocating stream"); 
			twStream_Delete(bodyStream);
			return TW_ERROR_ALLOCATING_MEMORY; 
		}
		twStream_AddBytes(s, header, headerSize);
		if (bodyBytesRemaining <= effectiveChunkSize) size = bodyBytesRemaining;
		if (msg->multipartMarker) {
			/* Adjust the chunk number */
			s->data[MSG_HEADER_SIZE] = (unsigned char)(chunkNumber/256);
			s->data[MSG_HEADER_SIZE + 1] = (unsigned char)(chunkNumber%256);
			/* If this is a request we also need to add the entity info unless this is the first chunk which already has it */
			if ((msg->code == TWX_GET || msg->code == TWX_PUT || msg->code == TWX_POST || msg->code == TWX_DEL) && chunkNumber != 1) {
				byte = (char)((twRequestBody *)msg->body)->entityType;
				twStream_AddBytes(s, &byte, 1);
				stringToStream(((twRequestBody *)msg->body)->entityName, s);
			}
		}
		/* Add the data */
		twStream_AddBytes(s,&bodyStream->data[length - bodyBytesRemaining], size);
		/* Send both streams off to the websocket */
		res = twWs_SendMessage(ws, twStream_GetData(s), twStream_GetLength(s), 0);
		if (res) {
			if (res == TW_WEBSOCKET_NOT_CONNECTED && msg->type == TW_REQUEST) {
				/* Check to see if offline message store is enabled and we don't exceed its max size */
				if (tw_api->offlineMsgEnabled && tw_api->offlineMsgSize + twStream_GetLength(s) < OFFLINE_MSG_QUEUE_SIZE) {
				#if (OFFLINE_MSG_STORE == 1) 
					/* Memory resident offline message store */
					if (tw_api->offlineMsgList) {
						if (twList_Add(tw_api->offlineMsgList, s)) {
							TW_LOG(TW_ERROR,"twMessage_Send: Error storing message in offline msg queue. RequestId %d", msg->requestId);
							twStream_Delete(s);
							return TW_ERROR_WRITING_OFFLINE_MSG_STORE;
						} else {
							TW_LOG(TW_DEBUG,"twMessage_Send: Stored message in offline msg queue. RequestId %d", msg->requestId);
							tw_api->offlineMsgSize += twStream_GetLength(s);
							return TW_OK;
						}
					}
				#endif
				#if (OFFLINE_MSG_STORE == 2)
					/* Persisted offline message store */
					if (tw_api->offlineMsgFile) {
						size_t sepLength = strlen(PERSISTED_MSG_SEPERATOR);
						TW_FILE_HANDLE f = TW_FOPEN(tw_api->offlineMsgFile, "a+b");
						if (!f) {
							TW_LOG(TW_ERROR,"twMessage_Send: Error opening offline msg file %s", tw_api->offlineMsgFile);
							twStream_Delete(s);
							return TW_ERROR_WRITING_OFFLINE_MSG_STORE;
						}
						/* Messages are delimited by <PERSISTED_MSG_SEPERATOR><stream length> */
						/* Write the separator */
						if (TW_FWRITE(PERSISTED_MSG_SEPERATOR, 1, sepLength, f) != sepLength) {
							TW_LOG(TW_ERROR,"twMessage_Send: Error storing message in offline msg file. RequestId %d", msg->requestId);
							TW_FCLOSE(f);
							twStream_Delete(s);
							return TW_ERROR_WRITING_OFFLINE_MSG_STORE;
						}
						/* Write the length */
						if (TW_FWRITE(&s->length, 1, sizeof(s->length), f) != sizeof(s->length)) {
							TW_LOG(TW_ERROR,"twMessage_Send: Error storing message in offline msg file. RequestId %d", msg->requestId);
							TW_FCLOSE(f);
							twStream_Delete(s);
							return TW_ERROR_WRITING_OFFLINE_MSG_STORE;
						} else {
							if (TW_FWRITE(s->data, 1, s->length, f) != s->length) {
								TW_LOG(TW_DEBUG,"twMessage_Send: Error storing message in offline msg file. RequestId %d", msg->requestId);
							} else {
								TW_LOG(TW_DEBUG,"twMessage_Send: Stored message in offline msg file. RequestId %d", msg->requestId);
								tw_api->offlineMsgSize += twStream_GetLength(s);
							}
							TW_FCLOSE(f);
							return TW_OK;
						}
					}
				#endif
				}
			}
			if (msg->multipartMarker) TW_LOG(TW_ERROR,"twMessage_Send: Error sending Chunk %d of %d with RequestId %d", 
				chunkNumber, numChunks, msg->requestId);
			else  TW_LOG(TW_ERROR,"twMessage_Send: Error sending Message with RequestId %d", msg->requestId);
			twStream_Delete(s);
			twStream_Delete(bodyStream);
			return TW_ERROR_SENDING_MSG; 
		} else {
			if (msg->multipartMarker) TW_LOG(TW_TRACE,"twMessage_Send: Chunk %d of %d with RequestId %d sent successfully", 
				chunkNumber, numChunks, msg->requestId);
			else  TW_LOG(TW_TRACE,"twMessage_Send: Message with RequestId %d sent successfully", msg->requestId);
			twStream_Delete(s);
			bodyBytesRemaining -= size;
		}
		chunkNumber++;
	}
	twStream_Delete(bodyStream);
	/* Reset the multipart marker so deleteting the message doesn't get confused */
	msg->multipartMarker = FALSE;
	return TW_OK;
}



void twHeader_Delete(void * value) {
	twHeader * hdr = (twHeader *) value;
	if (hdr) {
		TW_FREE(hdr->name);
		TW_FREE(hdr->value);
	}
}

int twHeader_toStream(twHeader * hdr, twStream * s) {
	int res = TW_UNKNOWN;
	if (hdr && s) {
		res = stringToStream(hdr->name, s);
		res |= stringToStream(hdr->value, s);
	}
	return res;
}

twHeader * twHeader_fromStream(twStream * s) {
	twHeader * hdr;
	int cnt = 0;
	int stringSize = 0;
	if (!s) return 0;
	hdr = (twHeader *)TW_CALLOC(sizeof(twHeader), 1);
	if (!hdr) return 0;
	
	while (cnt < 2) {
		unsigned char size[4];
		/* Get the first byte to check the size */
		twStream_GetBytes(s, &size[0], 1);
		if (size[0] > 127) {
			/* Need the full 4 bytes */
			twStream_GetBytes(s, &size[1], 3);
			stringSize = size[0] * 0x1000000 + size[1] * 0x10000 + size[2] * 0x100 + size[3];
		} else {
			stringSize = size[0];
		}
		if (cnt) {
			hdr->value = (char *)TW_CALLOC(stringSize + 1, 1);
			if (hdr->value) twStream_GetBytes(s, hdr->value, stringSize);
		} else {
			hdr->name = (char *)TW_CALLOC(stringSize + 1, 1);
			if (hdr->name) twStream_GetBytes(s, hdr->name, stringSize);
		}
		cnt++;
	}
	if (!hdr->name || !hdr->value) {
		TW_LOG(TW_ERROR,"twHeader_fromStream: Error allocating header name or value");
		twHeader_Delete(hdr);
	}
	return NULL;
}

twRequestBody * twRequestBody_Create() {
	twRequestBody * body = (twRequestBody *)TW_CALLOC(sizeof(twRequestBody), 1);
	if (!body) {
		TW_LOG(TW_ERROR, "twRequestBody: Error allocating body");
		return NULL;
	}
	body->headers = twList_Create(twHeader_Delete);
	if (!body->headers) {
		TW_LOG(TW_ERROR, "twRequestBody: Error allocating header list");
		twRequestBody_Delete(body);
		return NULL;
	}
	body->length = 4; /* Ent type + Charateristic TYpe + header count + params type */
	return body;
}

twRequestBody * twRequestBody_CreateFromStream(twStream * s) {
	twRequestBody * body = NULL;
	twPrimitive * prim = NULL;
	char * start = NULL;
	char tmp, i;
	if (!s) {
		TW_LOG(TW_ERROR, "twMessage_CreateFromStream: NULL stream pointer"); 
		return NULL; 
	}
	start = s->ptr;
	body = (twRequestBody *)TW_CALLOC(sizeof(twRequestBody), 1);
	if (!body) {
		TW_LOG(TW_ERROR, "twRequestBody: Error allocating body");
	}
	/* Get the entity type and name */
	twStream_GetBytes(s, &tmp, 1);
	body->entityType = (enum entityTypeEnum)tmp;
	prim = twPrimitive_CreateFromStreamTyped(s, TW_STRING);
	if (prim && prim->val.bytes.data) {
		body->entityName = twPrimitive_DecoupleStringAndDelete(prim);
	}
	/* Get the characteristic type and name */
	twStream_GetBytes(s, &tmp, 1);
	body->characteristicType = (enum characteristicEnum)tmp;
	prim = twPrimitive_CreateFromStreamTyped(s, TW_STRING);
	if (prim && prim->val.bytes.data) {
		body->characteristicName = twPrimitive_DecoupleStringAndDelete(prim);
	}
	/* Get the headers */
	twStream_GetBytes(s, &body->numHeaders, 1);
	for (i = 0; i < body->numHeaders; i++) {
		twHeader * hdr = twHeader_fromStream(s);
		if (hdr) twList_Add(body->headers, hdr);
	}
	/* If this has an infotable then parse it */
	twStream_GetBytes(s, &tmp, 1);
	if ((enum BaseType)tmp != TW_INFOTABLE) return body;
	body->params = twInfoTable_CreateFromStream(s);
	body->length = s->ptr - start;
	return body;
}

int twRequestBody_Delete(struct twRequestBody * body) {
	if (!body) {
		return TW_INVALID_PARAM; 
	}
	TW_FREE(body->entityName);
	TW_FREE(body->characteristicName);
	twList_Delete(body->headers);
	twInfoTable_Delete(body->params);
	TW_FREE(body);
	return TW_OK;
}

int twRequestBody_SetParams(struct twRequestBody * body, twInfoTable * params) {
	if (!body || body->params) {
		TW_LOG(TW_ERROR, "twRequestBody_SetParams: NULL body pointer or body already has params"); 
		return TW_INVALID_PARAM; 
	}
	body->params = params; /* We own this pointer now */
	if (params) {
		body->length += params->length;
	} 
	return TW_OK;
}

int twRequestBody_SetEntity(struct twRequestBody * body, enum entityTypeEnum entityType, char * entityName) {
	if (!body || body->entityName || !entityName) {
		TW_LOG(TW_ERROR, "twRequestBody_SetEntity: NULL pointer or body already has an entity"); 
		return TW_INVALID_PARAM; 
	}
	body->entityType = entityType;
	body->entityName = duplicateString(entityName); /* We own this pointer now */
	body->length += strlen(entityName) + 1;
	return TW_OK;
}

int twRequestBody_SetCharateristic(struct twRequestBody * body, enum characteristicEnum characteristicType, char * characteristicName) {
	if (!body || body->characteristicName || !characteristicName) {
		TW_LOG(TW_ERROR, "twRequestBody_SetCharateristic: NULL pointer or body already has a charateristic"); 
		return TW_INVALID_PARAM; 
	}
	body->characteristicType = characteristicType;
	body->characteristicName = duplicateString(characteristicName); /* We own this pointer now */
	body->length += strlen(characteristicName) + 1;
	return TW_OK;
}

int twRequestBody_AddHeader(struct twRequestBody * body, char * name, char * value) {
	twHeader * hdr = 0;
	if (!body || !body->headers || !name || !value) {
		TW_LOG(TW_ERROR, "twRequestBody_SetCharateristic: NULL body, headers pointer, name or value"); 
		return TW_INVALID_PARAM; 
	}
	hdr = (twHeader *)TW_CALLOC(sizeof(twHeader), 1);
	hdr->name = duplicateString(name);
	hdr->value = duplicateString(value);
	body->length = strlen(name) + 1 + strlen(value) + 1;
	twList_Add(body->headers, hdr);
	body->numHeaders++;
	return TW_OK;
}

int twRequestBody_ToStream(struct twRequestBody * body, twStream * s) {
	char byte;
	if (!body || !s) {
		TW_LOG(TW_ERROR, "twRequestBody_ToStream: NULL body or stream pointer"); 
		return TW_INVALID_PARAM; 
	}
	byte = (char)body->entityType;
	twStream_AddBytes(s, &byte, 1);
	stringToStream(body->entityName, s);
	byte = (char)body->characteristicType;
	twStream_AddBytes(s, &byte, 1);
	stringToStream(body->characteristicName, s);
	twStream_AddBytes(s, &body->numHeaders, 1);
	if (body->headers) {
		ListEntry * le = twList_Next(body->headers, NULL);
		while (le) {
			twHeader_toStream((twHeader *)le, s);
			le = twList_Next(body->headers, le);
		}
	}
	if (body->params) {
		byte = (char)TW_INFOTABLE;
		twStream_AddBytes(s, &byte, 1);
		twInfoTable_ToStream(body->params, s);
	} else {
		byte = (char)TW_NOTHING;
		twStream_AddBytes(s, &byte, 1);
	}
	return TW_OK;
}

/**
*	Response Body
**/
twResponseBody * twResponseBody_Create() {
	twResponseBody * body = (twResponseBody *)TW_CALLOC(sizeof(twResponseBody), 1);
	if (!body) {
		TW_LOG(TW_ERROR, "twResponseBody_Create: Error allocating body");
		return NULL;
	}
	/* Default to NOTHING */
	body->length = 2; /* Reason marker + Content base type */
	twResponseBody_SetContent(body, NULL);
	return body;
}

twResponseBody * twResponseBody_CreateFromStream(twStream * s) {
	twResponseBody * body = NULL;
	twPrimitive * prim = NULL;
	char * start;
	char tmp;
	if (!s) {
		TW_LOG(TW_ERROR, "twResponseBody_CreateFromStream: NULL stream pointer"); 
		return 0; 
	}
	start = s->ptr;
	body = (twResponseBody *)TW_CALLOC(sizeof(twResponseBody), 1);
	if (!body) { TW_LOG(TW_ERROR, "twResponseBody_CreateFromStream: Error allocating body storage"); return NULL; }
	twStream_GetBytes(s, &body->reasonMarker, 1);
	if (body->reasonMarker) {
		prim = twPrimitive_CreateFromStreamTyped(s, TW_STRING);
		if (prim && prim->val.bytes.data) {
			body->reason = twPrimitive_DecoupleStringAndDelete(prim);
		}
	}
	twStream_GetBytes(s, &tmp, 1);
	body->contentType = (enum BaseType)tmp;
	if (body->contentType != TW_INFOTABLE) return body;
	body->content = twInfoTable_CreateFromStream(s);
	body->length = s->ptr - start;
	return body;
}

int twResponseBody_Delete(struct twResponseBody * body) {
	if (!body) {
		return TW_INVALID_PARAM; 
	}
	if (body->reason) TW_FREE(body->reason);
	twInfoTable_Delete(body->content);
	TW_FREE(body);
	return TW_OK;
}

int twResponseBody_SetContent(struct twResponseBody * body, twInfoTable * content) {
	if (!body || body->content) {
		TW_LOG(TW_ERROR, "twResponseBody_SetContent: NULL body pointeror body already has content"); 
		return TW_INVALID_PARAM; 
	}
	body->content = content;  /* We own this pointer now */
	if (content) {
		body->contentType = TW_INFOTABLE;
		body->length += content->length;
	} else {
		body->contentType = TW_NOTHING;
	}
	return TW_OK;
}

int twResponseBody_SetReason(struct twResponseBody * body, char * reason) {
	if (!body) {
		TW_LOG(TW_ERROR, "twResponseBody_SetReason: NULL body pointer"); 
		return TW_INVALID_PARAM; 
	}
	if (!reason) {
		body->reasonMarker = FALSE;
		return TW_OK;
	}
	TW_FREE(body->reason);
	body->reason = duplicateString(reason);  /* We own this pointer now */
	body->length += strlen(reason) + 1;
	return TW_OK;
}

int twResponseBody_ToStream(struct twResponseBody * body, twStream * s) {
	if (!body || !s) {
		TW_LOG(TW_ERROR, "twResponseBody_ToStream: NULL body or stream pointer"); 
		return TW_INVALID_PARAM; 
	}
	twStream_AddBytes(s, &body->reasonMarker, 1);
	if (body->reasonMarker) stringToStream(body->reason, s);
	twStream_AddBytes(s, (char *)&body->contentType, 1);
	if (body->contentType == TW_INFOTABLE) twInfoTable_ToStream(body->content, s);
	return TW_OK;
}


/**
*	Auth Body
**/
twAuthBody * twAuthBody_Create() {
	twAuthBody * body = (twAuthBody *)TW_CALLOC(sizeof(twAuthBody), 1);
	if (!body) {
		TW_LOG(TW_ERROR, "twAuthBody_Create: Error allocating body");
		return NULL;
	}
	body->length = 1;
	return body;
}

twAuthBody * twAuthBody_CreateFromStream(twStream * s) {
	twAuthBody * body = NULL;
	twPrimitive * prim = NULL;
	char tmp;
	if (!s) {
		TW_LOG(TW_ERROR, "twAuthBody_CreateFromStream: NULL stream pointer"); 
		return 0; 
	}
	body = (twAuthBody *)TW_CALLOC(sizeof(twAuthBody), 1);
	if (!body) { TW_LOG(TW_ERROR, "twAuthBody_CreateFromStream: Error allocating body storage"); return NULL; }
	twStream_GetBytes(s, &tmp, 1);
	/* We are only supporting one claim */
	if (tmp) {
		prim = twPrimitive_CreateFromStreamTyped(s, TW_STRING);
		body->name = twPrimitive_DecoupleStringAndDelete(prim);
		prim = twPrimitive_CreateFromStreamTyped(s, TW_STRING);
		body->value = twPrimitive_DecoupleStringAndDelete(prim);
	}
	body->length = s->ptr - s->data;
	return body;
}

int twAuthBody_Delete(struct twAuthBody * body) {
	if (!body) {
		TW_LOG(TW_ERROR, "twAuthBody_Delete: NULL body or stream pointer"); 
		return TW_INVALID_PARAM; 
	}
	TW_FREE(body->name);
	TW_FREE(body->value);
	TW_FREE(body);
	return TW_OK;
}

int twAuthBody_SetClaim(struct twAuthBody * body, char * name, char * value) {
	if (!body || !name || !value) {
		TW_LOG(TW_ERROR, "twAuthBody_SetClaim: NULL body or name/value pointer"); 
		return TW_INVALID_PARAM; 
	}
	body->name = duplicateString(name); /* We own this pointer now */
	body->value = duplicateString(value); /* We own this pointer now */
	body->length += strlen(name) + 1 + strlen(value) + 1;
	return TW_OK;
}

int twAuthBody_ToStream(struct twAuthBody * body, twStream * s) {
	char count = 1;
	if (!body || !s) {
		TW_LOG(TW_ERROR, "twAuthBody_ToStream: NULL body or stream pointer"); 
		return TW_INVALID_PARAM; 
	}
	twStream_AddBytes(s, &count, 1);
	stringToStream(body->name, s);
	stringToStream(body->value, s);
	return TW_OK;
}

/**
*	Bind Body
**/
twBindBody * twBindBody_Create(char * name) {
	twBindBody * body = (twBindBody *)TW_CALLOC(sizeof(twBindBody), 1);
	if (!body) {
		TW_LOG(TW_ERROR, "twBindBody_Create: Error allocating body");
		return NULL;
	}
	body->names = twList_Create(0);
	if (!body->names) {
		TW_LOG(TW_ERROR, "twBindBody_Create: Error allocating list");
		twBindBody_Delete(body);
		return NULL;
	}
	body->length = 3; /* Marker for gateway name + count */
	if (name) {
		twList_Add(body->names, duplicateString(name));
		body->count++;
		body->length += strlen(name) + 1;
	}
	return body;
}

twBindBody * twBindBody_CreateFromStream(twStream * s) {
	twBindBody * body = NULL;
	twPrimitive * prim = NULL;
	char tmp;
	uint16_t count = 0;
	if (!s) {
		TW_LOG(TW_ERROR, "twBindBody_CreateFromStream: NULL stream pointer"); 
		return NULL; 
	}
	body = (twBindBody *)TW_CALLOC(sizeof(twBindBody), 1);
	if (!body) { TW_LOG(TW_ERROR, "twBindBody_CreateFromStream: Error allocating body storage"); return NULL; }
	body->names = twList_Create(0);
	if (!body->names) {
		twBindBody_Delete(body);
		TW_LOG(TW_ERROR, "twBindBody_CreateFromStream: Error allocating list");
	}
	twStream_GetBytes(s, &tmp, 1);
	/* Check for a gateway */
	if (tmp) {
		prim = twPrimitive_CreateFromStreamTyped(s, TW_STRING);
		body->gatewayName = twPrimitive_DecoupleStringAndDelete(prim);
		prim = twPrimitive_CreateFromStreamTyped(s, TW_STRING);
		body->gatewayType = twPrimitive_DecoupleStringAndDelete(prim);
	}
	/* Get the count */
	twStream_GetBytes(s, &tmp, 1);
	count = tmp * 0x100;
	twStream_GetBytes(s, &tmp, 1);
	count += tmp;
	body->count = count;
	while (count) {
		prim = twPrimitive_CreateFromStreamTyped(s, TW_STRING);
		if (prim) twList_Add(body->names, twPrimitive_DecoupleStringAndDelete(prim));
		count--;
	}
	body->length = s->ptr - s->data;
	return body;
}

int twBindBody_Delete(struct twBindBody * body) {
	if (!body) {
		TW_LOG(TW_ERROR, "twBindBody_Delete: NULL body or stream pointer"); 
		return TW_INVALID_PARAM; 
	}
	if (body->gatewayName) TW_FREE(body->gatewayName);
	if (body->gatewayType) TW_FREE(body->gatewayType);
	twList_Delete(body->names);
	TW_FREE(body);
	return TW_OK;
}

int twBindBody_AddName(struct twBindBody * body, char * name) {
	if (!body) {
		TW_LOG(TW_ERROR, "twBindBody_Create: NULL body pointer");
		return TW_INVALID_PARAM;
	}
	if (!body->names) {
		TW_LOG(TW_ERROR, "twBindBody_Create: NULL list point");
		return TW_INVALID_PARAM;
	}
	if (name) {
		twList_Add(body->names, duplicateString(name));
		body->count++;
		body->length += strlen(name) + 1;
	}
	return TW_OK;
}

int twBindBody_ToStream(struct twBindBody * body, twStream * s, char * gatewayName) {
	unsigned char tmp = 0;
	ListEntry * entry = NULL;
	char * utf8 = NULL;
	if (!body || !s || !body->names) {
		TW_LOG(TW_ERROR, "twBindBody_ToStream: NULL body or stream pointer"); 
		return TW_INVALID_PARAM; 
	}
	if (gatewayName) {
		/* Add the gateway information */
		tmp = 0x01;
		twStream_AddBytes(s, (char *)&tmp, 1);
		stringToStream(gatewayName, s);
		stringToStream("SDKGateway", s);

	} else twStream_AddBytes(s, (char *)&tmp, 1);
	tmp = (unsigned char)(body->count / 0x100);
	twStream_AddBytes(s, (char *)&tmp, 1);
	tmp = (unsigned char)(body->count % 0x100);
	twStream_AddBytes(s, (char *)&tmp, 1);
	entry = twList_Next(body->names, NULL);
	while (entry) {
		utf8 = (char *)entry->value;
		if (utf8) stringToStream(utf8, s);
		entry = twList_Next(body->names, entry);
	}
	return TW_OK;
}

/* Multipart Body */
twMultipartBody * twMultipartBody_CreateFromStream(twStream * s, char isRequest) {
	twMultipartBody * body = (twMultipartBody *)TW_CALLOC(sizeof(twMultipartBody), 1);
	char tmp[2];
	if (!body || !s) {
		TW_LOG(TW_ERROR, "twMultipartBody_CreateFromStream: Error allocating body or missing stream input");
		return NULL;
	}
	twStream_GetBytes(s, tmp, 2);
	body->chunkId = tmp[0] * 0x100 + tmp[1];
	twStream_GetBytes(s, tmp, 2);
	body->chunkCount = tmp[0] * 0x100 + tmp[1];
	twStream_GetBytes(s, tmp, 2);
	body->chunkSize = tmp[0] * 0x100 + tmp[1];
	if (isRequest) {
		twStream_GetBytes(s, tmp, 1);
		body->entityType = (enum entityTypeEnum)tmp[0];
		body->entityName = streamToString(s);
	}
	/* Allocate storage for the rest of the data in the stream */
	body->length = s->length - (s->ptr - s->data);
	body->data = (char *) TW_CALLOC(body->length, 1);
	if (!body->data) {
		TW_LOG(TW_ERROR,"twMultipartBody_CreateFromStream: Error allocating storage for data");
		twMultipartBody_Delete(body);
		return NULL;
	}
	twStream_GetBytes(s, body->data, body->length);
	return body;
}

void twMultipartBody_Delete(void * body) {
	twMultipartBody * tmp = (twMultipartBody *)body;
	if (!body) return;
	if (tmp->entityName) TW_FREE(tmp->entityName);
	if (tmp->data) TW_FREE(tmp->data);
	TW_FREE(tmp);
}

/**
* Multipart message cache - this is a singleton 
**/
twMultipartMessageStore * mpStore = NULL;

mulitpartMessageStoreEntry * mulitpartMessageStoreEntry_Create(twMessage * msg) {
	mulitpartMessageStoreEntry * tmp = (mulitpartMessageStoreEntry *)TW_CALLOC(sizeof(mulitpartMessageStoreEntry), 1);
	twMultipartBody * mpBody = NULL;
	if (!msg || !msg->body) return NULL;
	mpBody = (twMultipartBody *)msg->body;
	/* Make sure the size of this message doesn't exceed our max size */
	if (mpBody->chunkCount * mpBody->chunkSize > MAX_MESSAGE_SIZE) {
		TW_LOG(TW_ERROR,"mulitpartMessageStoreEntry_Create: Multipart message would exceed maximum message size");
		return NULL;
	}
	/* We want to store the messages in an ordered array for easy reasembly */
	tmp->msgs = (twMessage **)TW_CALLOC(sizeof(twMessage *) * mpBody->chunkCount, 1);
	if (!tmp->msgs) {
		mulitpartMessageStoreEntry_Delete(tmp);
		return NULL;
	}
	tmp->expirationTime = twGetSystemMillisecondCount() + STALE_MSG_CLEANUP_RATE;
	tmp->id = msg->requestId;
	tmp->chunksExpected = mpBody->chunkCount;
	tmp->chunksReceived = 1;
	tmp->msgs[mpBody->chunkId - 1] = msg; /* CHunk IDs are not zero based */
	return tmp;
}

void mulitpartMessageStoreEntry_Delete(void * entry) {
	uint16_t i = 0;
	mulitpartMessageStoreEntry * tmp = (mulitpartMessageStoreEntry *) entry;
	if (!tmp) return;
	for (i = 0; i < tmp->chunksExpected; i++) {
		if (tmp->msgs[i]) twMessage_Delete(tmp->msgs[i]);
	}
	if (tmp->msgs) TW_FREE(tmp->msgs);
	TW_FREE(tmp);
}

twMultipartMessageStore * twMultipartMessageStore_Instance() {
	/* Check to see if it already exists */
	twMutex_Lock(twInitMutex);
	if (mpStore) {
		twMutex_Unlock(twInitMutex);
		return mpStore;
	}
	mpStore = (twMultipartMessageStore *)TW_CALLOC(sizeof(twMultipartMessageStore), 1);
	if (!mpStore) {
		TW_LOG(TW_ERROR,"twMultipartMessageStore_Instance: Error allocating multipart message store");
	} else {
		mpStore->mtx = twMutex_Create();
		mpStore->multipartMessageList = twList_Create(mulitpartMessageStoreEntry_Delete);
		if (!mpStore->mtx || !mpStore->multipartMessageList) {
			TW_LOG(TW_ERROR, "twMultipartMessageStore_Instance: Error allocating memory");
			twMutex_Delete(mpStore->mtx);
			twList_Delete(mpStore->multipartMessageList);
			TW_FREE(mpStore);
			mpStore = 0;
		}
	}
	twMutex_Unlock(twInitMutex);
	return mpStore;
}

void twMultipartMessageStore_Delete(void * input) {
	twMultipartMessageStore * tmp = mpStore;
	if (!mpStore) return;
	twMutex_Lock(tmp->mtx);
	/*twMutex_Lock(twInitMutex);*/
	mpStore = NULL;
	/*twMutex_Unlock(twInitMutex);*/
	twMutex_Unlock(tmp->mtx);
	twMutex_Delete(tmp->mtx);
	twList_Delete(tmp->multipartMessageList);
	TW_FREE(tmp);
}

twMessage * twMultipartMessageStore_AddMessage(twMessage * msg) {
	/* Returns the complete message if all chunks have been received */
	ListEntry * le = NULL;
	uint16_t i = 0;
	twMultipartBody * mp = NULL;
	twStream * s = NULL;
	if (!mpStore || !msg || !msg->body){
		TW_LOG(TW_ERROR,"twMultipartMessageStore_AddMessage: No message or message store found");
		return NULL;
	}
	mp = (twMultipartBody *)msg->body;
	twMutex_Lock(mpStore->mtx);
	le = twList_Next(mpStore->multipartMessageList, NULL);
	while (le) {
		twMessage * m = NULL;
		mulitpartMessageStoreEntry * tmp = (mulitpartMessageStoreEntry *)le->value;
		if (tmp && tmp->id == msg->requestId) {
			/* The entry already exists in the list, just add the mesg */
			if (mp->chunkId > tmp->chunksExpected) {
				TW_LOG(TW_ERROR,"twMultipartMessageStore_AddMessage: Chunk Id %d is greater that expected chunks of %d",
					mp->chunkId, tmp->chunksExpected);
				twMessage_Delete(msg);
				twMutex_Unlock(mpStore->mtx);
				return NULL;
			}
			/* Put this message where it belongs */
			tmp->msgs[mp->chunkId - 1] = msg;
			/* Check to see if we have received all the chunks */
			for (i = 0; i < tmp->chunksExpected; i++ ) {
				if (!tmp->msgs[i]) {
					/* We aren't complete, return a NULL */
					twMutex_Unlock(mpStore->mtx);
					return NULL;
				}
			}
			/* If we are here then we have recevied the entire message */
			m = twMessage_Create(msg->code, msg->requestId);
			s = twStream_Create();
			if (!m || !s) {
				TW_LOG(TW_ERROR,"twMultipartMessageStore_AddMessage: Error allocating memory for complete message");
				twList_Remove(mpStore->multipartMessageList, le, TRUE);
				twMutex_Unlock(mpStore->mtx);
				return NULL;
			}
			m->requestId = msg->requestId;
			m->type = msg->type;
			m->endpointId = msg->endpointId;
			m->sessionId = msg->sessionId;
			/* Reassemble all the chunks */
			for (i = 0; i < tmp->chunksExpected; i++ ) {
				/* Need to add the entity info back in for the first chunk */
				twMultipartBody * b = (twMultipartBody *)tmp->msgs[i]->body;
				if (m->type == TW_MULTIPART_REQ && i == 0) {
					char et = b->entityType;
					twStream_AddBytes(s, &et, 1);
					stringToStream(b->entityName, s);
				}
				twStream_AddBytes(s, b->data, b->length);
				/* Delete the chunk here to preserve RAM  */
				TW_LOG(TW_TRACE,"twMultipartMessageStore_AddMessage: Deleting Multipart Chunk %d", i + 1);
				twMessage_Delete(tmp->msgs[i]);
				tmp->msgs[i] = 0;
			}
			twStream_Reset(s);
			if (m->type == TW_MULTIPART_REQ) {
				m->type = TW_REQUEST;
				m->body = twRequestBody_CreateFromStream(s);
			} else if (m->type == TW_MULTIPART_RESP) {
				m->type = TW_RESPONSE;
				m->body = twResponseBody_CreateFromStream(s);
			}
			twMutex_Unlock(mpStore->mtx);
			twStream_Delete(s);
			/* Remove this entry from the message store */
			twList_Remove(mpStore->multipartMessageList, le, TRUE);
			return m;
		}
		le = twList_Next(mpStore->multipartMessageList, le);
	}
	/* If we are here we didn't find an entry with this request id */
	twList_Add(mpStore->multipartMessageList, mulitpartMessageStoreEntry_Create(msg));
	twMutex_Unlock(mpStore->mtx);
	return NULL;
}

void twMultipartMessageStore_RemoveStaleMessages() {
	ListEntry * le = NULL;
	uint64_t now = twGetSystemMillisecondCount();
	if (!mpStore) return;
	twMutex_Lock(mpStore->mtx);
	le = twList_Next(mpStore->multipartMessageList, NULL);
	while (le) {
		mulitpartMessageStoreEntry * tmp = (mulitpartMessageStoreEntry *)le->value;
		if (twTimeGreaterThan(now, tmp->expirationTime)) {
			TW_LOG(TW_INFO,"Removing stale message with Request Id %d", tmp->id);
			twList_Remove(mpStore->multipartMessageList, le, TRUE);
		}
		le = twList_Next(mpStore->multipartMessageList, le);
	}
	twMutex_Unlock(mpStore->mtx);
}
