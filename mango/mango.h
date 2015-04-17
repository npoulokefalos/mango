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


/**
 * @brief  Connectes to the specified HTTP Server
 * @retval MANGO_OK     A new mangoHttpClient_t instance if the conenction was established
 * @retval NULL         If the conenction failed to be established
 */
mangoHttpClient_t*  mango_connect(char* serverIP, uint16_t serverPort);

/**
 * @brief  Creates an new HTTP request
 * @retval MANGO_OK    if the request created succesfully succesfully
 * @retval errorcode   If the request creation failed due to memory limitation or other factor 
 */
mangoErr_t          mango_httpRequestNew(mangoHttpClient_t* hc, char* URI, mangoHttpMethod_e method);

/**
 * @brief  Adds Basic Access Auhtentication specific headers to the new HTTP request
 */
mangoErr_t 			mango_httpAuthSet(mangoHttpClient_t* hc, mangoHttpAuth_t auth, char* username, char* password);

/**
 * @brief  Adds the specified header to the new HTTP request
 *
 * @retval MANGO_OK    if the header was added succesfully
 * @retval errorcode   If the header was not added due to memory limitation or other factor 
 */
mangoErr_t          mango_httpHeaderSet(mangoHttpClient_t* hc, char* headerName, char* headerValue);

/**
 * @brief Gets the value of the specific header from a received HTTP reponse.
 * @param response
 * @param headerName The name of the header
 * @param headerValue Temporary buffer to store the value of the header
 * @param headerValueLen The size of the provided headerValue buffer
 *
 * @retval MANGO_OK                 if the header value was copied to the "headerValue" buffer
 * @retval MANGO_ERR_TEMPBUFSMALL   if the header was found in the reponse, but the size of the "headerValue" buffer
 *                                   was not big enough to copy it
 * @retval MANGO_ERR                 If the header was not found                                             
 */
mangoErr_t          mango_httpHeaderGet(char* response, char* headerName, char* headerValue, uint16_t headerValueLen);

/**
 * @brief   Starts the processing of the HTTP request.
 *          
 * @return  [MANGO_ERR_HTTP_100, MANGO_ERR_HTTP_599] status codes indicate that a HTTP response was received
 *          and the HTTP conenction is healthy (meaning that a new HTTP request could be issued using the same
 *          open connection). Else an errorcode indicating the failure reason is returned.
 */
mangoErr_t          mango_httpRequestProcess(mangoHttpClient_t* hc, mangoErr_t (*userFunc)(mangoArg_t* mangoArgs, void* userArgs), void* userArgs);

/**
 * @brief   In case of POST/PUT HTTP requests this function sends the HTTP body of the request. When the 
 *          whole HTTP body has been sent this function should be called again with "buf" NULL and "buflen" 0 
 *          to notify the internal state machine that the whole HTTP body was provided by the application. 
 *          If the request was a CHUNKED one, the last chunk will be sent, and then mango is going to read the received
 *          HTTP response. If the request was not a CHUNKED one then mango is going to check if the
 *          data provided by the aplication was equal to the "Content-Length" header of the request, and
 *          if everything is OK then the internal State Machine is going to read the HTTP response.
 *
 * @retval MANGO_OK     Frame transmition succeed
 * @retval errorcode    Frame transmition failed, connection was closed.
 *                      In this case the application should call mango_disconnect().
 *
 *          When the last call has been made ("buf" is NULL and "buflen" is 0), then the following values
 *          are returned:
 *
 * @return  [MANGO_ERR_HTTP_100, MANGO_ERR_HTTP_599] status codes indicate that a HTTP response was received
 *          and the HTTP conenction is healthy (meaning that a new HTTP request could be issued using the same
 *          open connection). Else an errorcode indicating the failure reason is returned.           
 */
mangoErr_t 			mango_httpDataSend(mangoHttpClient_t* hc, uint8_t* buf, uint32_t buflen);

/**
 * @brief   In case of websockets, blocks for the specified amount of time (miliseconds) waiting for
 *          any received data and control (Ping, Close) frames. These frames are provided to the
 *          application through the callback function.
 * @retval MANGO_OK     Websocket connection is healthy
 * @retval errorcode    Websocket connection has been closed or encoutered an error. 
                        In this case the application should call mango_disconnect().
 */
mangoErr_t 			mango_wsPoll(mangoHttpClient_t* hc, uint32_t timeout);

/**
 * @brief   In case of websockets, this function is used to send a new data frame to the remote server
 *
 * @retval MANGO_OK     Frame transmition succeed
 * @retval errorcode    Frame transmition failed, connection might be closed.
 *                      In this case the application should call mango_disconnect().
 */
mangoErr_t 			mango_wsFrameSend(mangoHttpClient_t* hc, uint8_t* buf, uint32_t buflen, mangoWsFrameType_t type);

/**
 * @brief   In case of websockets, this function is used to close the websocket connection.
 *          After calling this function the application should call mango_disconnect().
 *
 * @retval MANGO_OK     Websocket connection close succeed
 * @retval errorcode    Websocket connection close failed (Close frame could not be sent
 *                      maybe because the connection is already closed)
 */
mangoErr_t 			mango_wsClose(mangoHttpClient_t* hc);


/**
 * @brief   Closes any active HTTP connection and releases any memory resources
 */
void                mango_disconnect(mangoHttpClient_t* hc);





#endif
