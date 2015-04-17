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

#ifndef __MANGO_INTERNAL_H__
#define __MANGO_INTERNAL_H__

#include <stdio.h>          /* printf, ... */
#include <string.h>          /* strlen, ... */

#include "mangoTypes.h"
#include "mangoConfig.h"

/* **********************************************************************************************************************
* Definitions
*************************************************************************************************************************/
#define MANGO_FILE_SZ_INFINITE      (-1)
#define MANGO_TIMEOUT_INFINITE      (-1)

#define MANGO_WB_TOT_SZ(hc)         (sizeof(hc->workingBuffer) - 1)  /* Last byte is for string termination */
#define MANGO_WB_USED_SZ(hc)        (hc->workingBufferIndexRight - hc->workingBufferIndexLeft)
#define MANGO_WB_FREE_SZ(hc)        (MANGO_WORKING_BUFFER_SZ - hc->workingBufferIndexRight - 1) /* Last byte is for string termination */

#define MANGO_WB_PTR(hc)		    &hc->workingBuffer[0]
#define MANGO_WB_USED_PTR(hc)       &hc->workingBuffer[hc->workingBufferIndexLeft]
#define MANGO_WB_FREE_PTR(hc)       &hc->workingBuffer[hc->workingBufferIndexRight]

#define MANGO_WB_NULLTERMINATE()    hc->workingBuffer[hc->workingBufferIndexRight]	= '\0';

/* **********************************************************************************************************************
* Port layer function declarations
*************************************************************************************************************************/

void*       mangoPort_malloc(uint32_t sz);
void        mangoPort_free(void* ptr);
int         mangoPort_read(int socketfd, uint8_t* data, uint16_t datalen, uint32_t timeout);
int         mangoPort_write(int socketfd, uint8_t* data, uint16_t datalen, uint32_t timeout);
void        mangoPort_disconnect(int socketfd);
int         mangoPort_connect(char* serverIP, uint16_t serverPort, uint32_t timeout);
uint32_t    mangoPort_timeNow(void);
void        mangoPort_sleep(uint32_t ms);

/* **********************************************************************************************************************
* Helper function declarations
*************************************************************************************************************************/
int         mangoHelper_httpReponseVerify(char* response);
int         mangoHelper_httpHeaderGet(char* response, char* headerName, char* headerValue, uint16_t headerValueLen);
uint32_t    mangoHelper_elapsedTime(uint32_t starttime);
void        mangoHelper_dec2hexstr(uint32_t dec, char hexbuf[9]);
int         mangoHelper_hexstr2dec(char* hexstr, uint32_t* dec);
int         mangoHelper_decstr2dec(char* decstr, uint32_t* dec);
void        mangoHelper_dec2decstr(uint32_t dec, char decbuf[11]);

/* **********************************************************************************************************************
* Crypto function declarations
*************************************************************************************************************************/
int 		mangoCrypto_base64Encode(const void* data_buf, size_t dataLength, char* result, size_t resultSize);

/* **********************************************************************************************************************
* Websocket function declarations
*************************************************************************************************************************/
mangoErr_t  mangoWS_close(int socketfd);
mangoErr_t  mangoWS_pong(int socketfd);
mangoErr_t  mangoWS_frameSend(int socketfd, uint8_t* buf, uint32_t buflen, mangoWsFrameType_t type);

/* **********************************************************************************************************************
* Working buffer function declarations
*************************************************************************************************************************/
void        mangoWB_shrink(mangoHttpClient_t* hc);

/* **********************************************************************************************************************
* State machine function declarations
*************************************************************************************************************************/
mangoErr_t  mangoSM_INIT(mangoHttpClient_t* hc);
mangoErr_t  mangoSM_PROCESS(mangoHttpClient_t* hc, mangoEvent_e event);
void        mangoSM_EXITERR(mangoErr_t err, mangoHttpClient_t* hc);
void        mangoSM_THROW(mangoEvent_e event, mangoHttpClient_t* hc);
void        mangoSM_SUBSCRIBE(mangoEvent_e event, mangoHttpClient_t* hc);

void        mangoSM__ABORTED(mangoEvent_e event, mangoHttpClient_t* hc);
void        mangoSM__DISCONNECTED(mangoEvent_e event, mangoHttpClient_t* hc);

void        mangoSM__HTTP_CONNECTED(mangoEvent_e event, mangoHttpClient_t* hc);
void        mangoSM__HTTP_SENDING_HEADERS(mangoEvent_e event, mangoHttpClient_t* hc);
void        mangoSM__HTTP_RECVING_HEADERS(mangoEvent_e event, mangoHttpClient_t* hc);
void        mangoSM__HTTP_RECVING_DATA(mangoEvent_e event, mangoHttpClient_t* hc);
void        mangoSM__HTTP_SENDING_DATA(mangoEvent_e event, mangoHttpClient_t* hc);
void        mangoSM__HTTP_SENDING_PACKET(mangoEvent_e event, mangoHttpClient_t* hc);

void        mangoSM__WS_CONNECTED(mangoEvent_e event, mangoHttpClient_t* hc);
void        mangoSM__WS_POLLING(mangoEvent_e event, mangoHttpClient_t* hc);
void        mangoSM__WS_SENDING_FRAME(mangoEvent_e event, mangoHttpClient_t* hc);
void        mangoSM__WS_CLOSING(mangoEvent_e event, mangoHttpClient_t* hc);

/* **********************************************************************************************************************
* Input data processor (IDP) function declarations
*************************************************************************************************************************/
mangoErr_t  mangoIDP_raw(mangoHttpClient_t* hc, uint8_t* buf, uint16_t buflen, void* vargs, uint32_t* processed, uint8_t* completed);
mangoErr_t  mangoIDP_chunked(mangoHttpClient_t* hc, uint8_t* buf, uint16_t buflen, void* vargs, uint32_t* processed, uint8_t* completed);
mangoErr_t  mangoIDP_websocket(mangoHttpClient_t* hc, uint8_t* buf, uint16_t buflen, void* vargs, uint32_t* processed, uint8_t* completed);

/* **********************************************************************************************************************
* Output data processor (ODP) function declarations
*************************************************************************************************************************/
mangoErr_t  mangoODP_raw(mangoHttpClient_t* hc, uint8_t* buf, uint16_t buflen, void* vargs, uint32_t* processed, uint8_t* completed);
mangoErr_t  mangoODP_chunked(mangoHttpClient_t* hc, uint8_t* buf, uint16_t buflen, void* vargs, uint32_t* processed, uint8_t* completed);

/* **********************************************************************************************************************
* Socket IO hook function declarations
*************************************************************************************************************************/
int         mangoSocket_read(mangoHttpClient_t* hc, uint8_t* data, uint16_t datalen, uint32_t timeout);
int         mangoSocket_write(mangoHttpClient_t* hc, uint8_t* data, uint16_t datalen, uint32_t timeout);



#endif
