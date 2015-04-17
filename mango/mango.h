/*
 * mango HTTP client
 *
 * Copyright (C) 2015,  Nikos Poulokefalos
 *
 * This file is part of mango HTTP client.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * npoulokefalos@gmail.com
*/

#ifndef __MANGO_H__
#define __MANGO_H__

#include "mangoTypes.h"
#include "mangoConfig.h"
#include "mangoInternal.h"
#include "mangoDebug.h"

#define MANGO_HDR__CONTENT_LENGTH       "Content-Length"
#define MANGO_HDR__TRANSFER_ENCODING    "Transfer-Encoding"
#define MANGO_HDR__HOST    				"Host"
#define MANGO_HDR__EXPECT    			"Expect"
#define MANGO_HDR__WEB_SOCKET_PROTOCOL 	"Sec-WebSocket-Protocol"
#define MANGO_HDR__WEB_SOCKET_KEY 		"Sec-WebSocket-Key"
#define MANGO_HDR__UPGRADE 				"Upgrade"
#define MANGO_HDR__ORIGIN 				"Origin"
#define MANGO_HDR__WEB_SOCKET_VERSION 	"Sec-WebSocket-Version"
#define MANGO_HDR__CONNECTION 			"Connection"
#define MANGO_HDR__WWW_AUTHENTICATE 	"WWW-Authenticate"
#define MANGO_HDR__AUTHORIZATION 		"Authorization"



mangoHttpClient_t*  mango_connect(char* serverIP, uint16_t serverPort);

mangoErr_t          mango_httpRequestNew(mangoHttpClient_t* hc, char* URI, mangoHttpMethod_e method);
mangoErr_t 			mango_httpAuthSet(mangoHttpClient_t* hc, mangoHttpAuth_t auth, char* username, char* password);
mangoErr_t          mango_httpHeaderSet(mangoHttpClient_t* hc, char* headerName, char* headerValue);
mangoErr_t          mango_httpHeaderGet(char* response, char* headerName, char* headerValue, uint16_t headerValueLen);
mangoErr_t          mango_httpRequestProcess(mangoHttpClient_t* hc, mangoErr_t (*userFunc)(mangoArg_t* mangoArgs, void* userArgs), void* userArgs);
mangoErr_t 			mango_httpDataSend(mangoHttpClient_t* hc, uint8_t* buf, uint32_t buflen);

mangoErr_t 			mango_wsPoll(mangoHttpClient_t* hc, uint32_t timeout);
mangoErr_t 			mango_wsFrameSend(mangoHttpClient_t* hc, uint8_t* buf, uint32_t buflen, mangoWsFrameType_t type);
mangoErr_t 			mango_wsClose(mangoHttpClient_t* hc);

void                mango_disconnect(mangoHttpClient_t* hc);





#endif
