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

#include "mango.h"


#define mangoSM_ENTER(state, hc)  \
    MANGO_ENSURE(hc->curState != state, ("?") ); \
    hc->nxtState = state; \
    return;

mangoErr_t mangoSM_INIT(mangoHttpClient_t* hc){
    
    hc->curState = mangoSM__HTTP_CONNECTED;
    hc->nxtState = mangoSM__HTTP_CONNECTED;
    
	hc->subscribedEvent = EVENT_NONE;
	hc->smTimeout		= MANGO_TIMEOUT_INFINITE;
	hc->smEntryTimestamp= mangoPort_timeNow();
	
    mangoSM_THROW(EVENT_ENTRY, hc);
    
    return MANGO_OK;
}

mangoErr_t mangoSM_PROCESS(mangoHttpClient_t* hc, mangoEvent_e event){
    uint32_t elapsedTime;
    uint8_t forever = 1;

    /*
    * Reset error code
    */
    mangoSM_EXITERR(MANGO_OK, hc);
    
    /*
    * Trigger event subscription
    */
    mangoSM_THROW(event, hc);

    /*
    * Start processing the SM
    */
    while(forever){

		/*
        * Check for state timeout
        */
		if(hc->smTimeout != MANGO_TIMEOUT_INFINITE){
			elapsedTime = mangoHelper_elapsedTime(hc->smEntryTimestamp);
			if(elapsedTime >= hc->smTimeout){
				mangoSM_THROW(EVENT_TIMEOUT, hc);
			}else{
				hc->smEventTimeout = hc->smTimeout - elapsedTime;
			}
		}
		
        switch(hc->subscribedEvent){
            case EVENT_READ:
                mangoSM_THROW(EVENT_READ, hc);
                break;
            case EVENT_WRITE:
                mangoSM_THROW(EVENT_WRITE, hc);
                break;
            case EVENT_PROCESS:
                mangoSM_THROW(EVENT_PROCESS, hc);
                break;
            case EVENT_NONE:
                /*
                * No subscribed event, exit
                */
                forever = 0;
                break;
            default:
                MANGO_ENSURE(0, ("?") );
                break;
        }
    }
    
    return hc->smExitError;
}



void mangoSM_EXITERR(mangoErr_t err, mangoHttpClient_t* hc){
    hc->smExitError = err;
}

void mangoSM_SUBSCRIBE(mangoEvent_e event, mangoHttpClient_t* hc){
    switch(event){
        case EVENT_PROCESS:
        case EVENT_READ:
        case EVENT_WRITE:
		case EVENT_APICALL_wsFrameSend:
            hc->subscribedEvent = event;
            break;
        default:
            MANGO_ENSURE(0, ("?") );
            break;
    }
}

void mangoSM_THROW(mangoEvent_e event, mangoHttpClient_t* hc){
    hc->curState(event, hc);
    while(hc->nxtState != hc->curState){
        hc->curState        = hc->nxtState;
        hc->subscribedEvent = EVENT_NONE;
		hc->smTimeout		= MANGO_TIMEOUT_INFINITE;
		hc->smEntryTimestamp= mangoPort_timeNow();
        hc->curState(EVENT_ENTRY, hc);
    }
}


void mangoSM_TIMEOUT(uint32_t timeout, mangoHttpClient_t* hc){
	hc->smEntryTimestamp = mangoPort_timeNow();
    hc->smTimeout = timeout;
	
}

/* -----------------------------------------------------------------------------------------------------------------
| STATE MACHINE
----------------------------------------------------------------------------------------------------------------- */
void mangoSM__ABORTED(mangoEvent_e event, mangoHttpClient_t* hc){
    MANGO_DBG(MANGO_DBG_LEVEL_SM, ("STATE %s, EVENT %u\r\n", __func__, event) );
    
	switch(event){
		case EVENT_ENTRY:
            /*
            * State machine is going to exit as we do not subsribe to any events
            */
			break;
        case EVENT_APICALL_httpRequestProcess:
		case EVENT_APICALL_httpDataSend:
		case EVENT_APICALL_wsPoll:
		case EVENT_APICALL_wsFrameSend:
		case EVENT_APICALL_wsClose:
            /*
            * State machine is going to exit as we do not subsribe to any events
            */
            mangoSM_EXITERR(MANGO_ERR_ABORTED, hc);
			break;
		default:
            MANGO_ENSURE(0, ("?") );
			break;	
	}
}

void mangoSM__DISCONNECTED(mangoEvent_e event, mangoHttpClient_t* hc){
    MANGO_DBG(MANGO_DBG_LEVEL_SM, ("STATE %s, EVENT %u\r\n", __func__, event) );
    
	switch(event){
		case EVENT_ENTRY:
            /*
            * State machine is going to exit as we do not subsribe to any event
            */
			break;
        case EVENT_APICALL_httpRequestProcess:
		case EVENT_APICALL_httpDataSend:
		case EVENT_APICALL_wsPoll:
		case EVENT_APICALL_wsFrameSend:
		case EVENT_APICALL_wsClose:
            /*
            * State machine is going to exit as we do not subsribe to any event
            */
            mangoSM_EXITERR(MANGO_ERR_CONNECTION, hc);
			break;
		default:
            MANGO_ENSURE(0, ("?") );
			break;	
	}
}

void mangoSM__HTTP_CONNECTED(mangoEvent_e event, mangoHttpClient_t* hc){
    MANGO_DBG(MANGO_DBG_LEVEL_SM, ("STATE %s, EVENT %u\r\n", __func__, event) );
    
	switch(event){
		case EVENT_ENTRY:
            /*
            * State machine is going to exit as we do not subsribe to any event.
			* We also clear the working buffer so we can execute new HTTP requests
            */
            hc->workingBufferIndexLeft = 0;
            hc->workingBufferIndexRight = 0;
			
			hc->stats.time = mangoHelper_elapsedTime(hc->stats.time);
			MANGO_DBG(MANGO_DBG_LEVEL_SM, ("-----------------------------------\r\n") );
			MANGO_DBG(MANGO_DBG_LEVEL_SM, ("| Tx   = %u bytes\r\n", hc->stats.txBytes) );
			MANGO_DBG(MANGO_DBG_LEVEL_SM, ("| Rx   = %u bytes\r\n", hc->stats.rxBytes) );
			MANGO_DBG(MANGO_DBG_LEVEL_SM, ("| Time = %u ms\r\n", hc->stats.time) );
			MANGO_DBG(MANGO_DBG_LEVEL_SM, ("-----------------------------------\r\n") );
			
			break;
        case EVENT_APICALL_httpRequestProcess:
            mangoSM_ENTER(mangoSM__HTTP_SENDING_HEADERS, hc);
			break;
		case EVENT_APICALL_httpDataSend:
		case EVENT_APICALL_wsPoll:
		case EVENT_APICALL_wsFrameSend:
		case EVENT_APICALL_wsClose:
			/*
			* Connection has not been upgraded to websockets
			* so we cannot execute the websocket API..
			*/
			mangoSM_EXITERR(MANGO_ERR_APICALLNOTSUPPORTED, hc);
			break;
        default:
            MANGO_ENSURE(0, ("?") );
			break;	
	}
}

void mangoSM__HTTP_SENDING_HEADERS(mangoEvent_e event, mangoHttpClient_t* hc){
	char headerValueBuf[10 + 1];
	int retval;
	
    MANGO_DBG(MANGO_DBG_LEVEL_SM, ("STATE %s, EVENT %u\r\n", __func__, event) );
    
	switch(event){
		case EVENT_ENTRY:
        {
            MANGO_ENSURE(hc->workingBufferIndexLeft == 0, ("?") );
            MANGO_ENSURE(hc->workingBufferIndexRight > 0, ("?") );
            
            mangoSM_SUBSCRIBE(EVENT_WRITE, hc);
            
			break;
        }
		case EVENT_WRITE:
        {
            /* 
            * Send HTTP request headers 
            */
            int retval;
            
            retval = mangoSocket_write(hc, &hc->workingBuffer[hc->workingBufferIndexLeft], hc->workingBufferIndexRight - hc->workingBufferIndexLeft, MANGO_SOCKET_WRITE_TIMEOUT_MS); 
            if(retval < 0){
                /* Connection error */
                mangoSM_EXITERR(MANGO_ERR_CONNECTION, hc);
                mangoSM_ENTER(mangoSM__DISCONNECTED, hc);
                break;
            }else{
                hc->workingBufferIndexLeft += retval;
                
                MANGO_ENSURE(hc->workingBufferIndexLeft <= hc->workingBufferIndexRight, ("?") );
                
                if(hc->workingBufferIndexLeft == hc->workingBufferIndexRight){
                   mangoSM_SUBSCRIBE(EVENT_PROCESS, hc);
                   return;
                }
            }
			break;
        }
		case EVENT_PROCESS:
        {
			/* 
            * HTTP request headers sent, inspect user's request to get the next state.
			* The HTTP request is stored into the WB at range [0, hc->workingBufferIndexLeft - 1]
            */
			switch(hc->httpMethod){
				case MANGO_HTTP_METHOD_HEAD:
				case MANGO_HTTP_METHOD_GET:
				{
					mangoSM_ENTER(mangoSM__HTTP_RECVING_HEADERS, hc);
					
					break;
				}
				case MANGO_HTTP_METHOD_PUT:
				case MANGO_HTTP_METHOD_POST:
				{	
					hc->outputDataProcessor = NULL;
					hc->dataProcessorArgs = NULL;
					
					/* 
					* Check if MANGO_HDR__CONTENT_LENGTH header is used and if so assign the RAW ODP
					*/
					retval = mangoHelper_httpHeaderGet((char*)MANGO_WB_PTR(hc), MANGO_HDR__CONTENT_LENGTH, headerValueBuf, sizeof(headerValueBuf));
					if(retval < 0){
					}else{
						/* Normal PUT/POST */
						hc->outputDataProcessor = mangoODP_raw;
                        if(mangoHelper_decstr2dec(headerValueBuf, &hc->ODPArgsRaw.fileSz)){
                            mangoSM_EXITERR(MANGO_ERR_INVALIDREQHEADERS, hc);
							mangoSM_ENTER(mangoSM__ABORTED, hc);
                        }
                        
                        MANGO_DBG(MANGO_DBG_LEVEL_SM, ("Attaching Raw ODP\r\n") );
                        
						hc->ODPArgsRaw.fileSzProcessed = 0;
						hc->dataProcessorArgs = &hc->ODPArgsRaw;
						
						if(hc->ODPArgsRaw.fileSz == 0){
							mangoSM_ENTER(mangoSM__HTTP_RECVING_HEADERS, hc);
						}
					}
					
					/* 
					* Check if MANGO_HDR__TRANSFER_ENCODING header is used and if so assign the CHUNKED ODP
					*
					* Messages MUST NOT include both a Content-Length header field and a non-identity transfer-coding. 
					* If the message does include a non-identity transfer-coding, the Content-Length MUST be ignored.
					*/
					retval = mangoHelper_httpHeaderGet((char*)MANGO_WB_PTR(hc), MANGO_HDR__TRANSFER_ENCODING, NULL, 0);
					if(retval < 0){
					}else{
						/* Chunked PUT/POST */
						if(hc->outputDataProcessor){
							/* User has provided both "Content-Length" & "Transfer-Encoding" HTTP Headers */
							mangoSM_EXITERR(MANGO_ERR_INVALIDREQHEADERS, hc);
							mangoSM_ENTER(mangoSM__ABORTED, hc);
						}
						
                        MANGO_DBG(MANGO_DBG_LEVEL_SM, ("Attaching Chunked ODP\r\n") );
                        
						hc->outputDataProcessor = mangoODP_chunked;
						hc->ODPArgsChunked.workingBuffer = MANGO_WB_PTR(hc);
						hc->ODPArgsChunked.workingBufferSz = MANGO_WB_TOT_SZ(hc);
						hc->dataProcessorArgs = &hc->ODPArgsChunked;
					}
					
					/* 
					* Verify that request's headers resulted to one ODP
					*/
					if(!hc->outputDataProcessor){
						/* HTTP request has not the "Content-Length" or the "Transfer-Encoding" HTTP Header */
						mangoSM_EXITERR(MANGO_ERR_INVALIDREQHEADERS, hc);
						mangoSM_ENTER(mangoSM__ABORTED, hc);
					}
					
					/* 
					* Check if MANGO_HDR__EXPECT header is used and if so move to the mangoSM__HTTP_RECVING_HEADERS state
					*/
					retval = mangoHelper_httpHeaderGet((char*)MANGO_WB_PTR(hc), MANGO_HDR__EXPECT, NULL, 0);
					if(retval < 0){
						/* Not used. Set a virtual  MANGO_ERR_HTTP_100 so user can handle it the same way that expect-100 is handled */
						mangoSM_EXITERR(MANGO_ERR_HTTP_100, hc);
						mangoSM_ENTER(mangoSM__HTTP_SENDING_DATA, hc);
					}else{
						/* Used */
						mangoSM_ENTER(mangoSM__HTTP_RECVING_HEADERS, hc);
					}
					
					break;
				}
				default:
				{
					MANGO_ENSURE(0, ("?") );
					break;
				}
			}
		}
        default:
        {
            MANGO_ENSURE(0, ("?") );
			break;
        }
	}
}



void mangoSM__HTTP_RECVING_HEADERS(mangoEvent_e event, mangoHttpClient_t* hc){
	char headerValueBuf[32];
	mangoArg_t funcArgs;
	int retval;
	
    MANGO_DBG(MANGO_DBG_LEVEL_SM, ("STATE %s, EVENT %u\r\n", __func__, event) );
    
	switch(event){
		case EVENT_ENTRY:
        {
			mangoSM_TIMEOUT(MANGO_HTTP_RESPONSE_TIMEOUT_MS, hc);
            mangoSM_SUBSCRIBE(EVENT_READ, hc);
            hc->workingBufferIndexLeft  = 0;
            hc->workingBufferIndexRight = 0;
			break;
        }
        case EVENT_READ:
        {
            retval = mangoSocket_read(hc, MANGO_WB_FREE_PTR(hc), MANGO_WB_FREE_SZ(hc), hc->smEventTimeout); 
            if(retval < 0){
                /* Connection error */
                mangoSM_EXITERR(MANGO_ERR_CONNECTION, hc);
                mangoSM_ENTER(mangoSM__DISCONNECTED, hc);
            }else if(retval == 0){
			}else{
                hc->workingBufferIndexRight += retval;
                MANGO_WB_NULLTERMINATE();
                
                retval = mangoHelper_httpReponseVerify((char*) hc->workingBuffer);
                if(retval < 0){
                    /* Malformed/not supported HTTP Response format */
                    mangoSM_EXITERR(MANGO_ERR_RESPFORMAT, hc);
                    mangoSM_ENTER(mangoSM__ABORTED, hc);
                }else if(retval == 0){
                    /* We haven't received the whole response yet, we should read more data to get it */
                    if(MANGO_WB_FREE_SZ(hc) == 0){
                        /* ..but we have no space anyway */
                        mangoSM_EXITERR(MANGO_ERR_WORKBUFSMALL, hc);
                        mangoSM_ENTER(mangoSM__ABORTED, hc);
                    }
                }else{
					/* The whole HTTP response received */
					hc->httpResponseStatusCode = retval;
					
					/* 
					* NOTE: Update the left index of the working buffer to be ready to enter
					* other states that are not aware that we have trimmed 2 bytes
					* from the working buffer (last CRLF). For this reason we will not be
					* able to use the MANGO_WB_xxx during the EVENT_PROCESS event.
					*/
					hc->workingBufferIndexLeft = strlen((char*) hc->workingBuffer) + 2;
					 
					mangoSM_SUBSCRIBE(EVENT_PROCESS, hc);
					return;
				}
			}
			
			break;
		}
		case EVENT_PROCESS:
		{
			/*
			*	Pass the HTTP response to the application
			*/
			funcArgs.buf = MANGO_WB_PTR(hc);
			funcArgs.buflen = hc->workingBufferIndexLeft;
			funcArgs.statusCode = hc->httpResponseStatusCode;
			funcArgs.argType = MANGO_ARG_TYPE_HTTP_RESP_RECEIVED;
			hc->userFunc(&funcArgs, hc->userArgs);

			MANGO_DBG(MANGO_DBG_LEVEL_SM, ("CONTENT LENGTH SEARCH\r\n") );
			
			/*
			* Locate Content-Length
			*/
			retval = mangoHelper_httpHeaderGet((char*) MANGO_WB_PTR(hc), MANGO_HDR__CONTENT_LENGTH, headerValueBuf, sizeof(headerValueBuf));
			if(retval < 0){
				/* Content-Length not found */
				MANGO_DBG(MANGO_DBG_LEVEL_SM, ("CONTENT LENGTH NOT FOUND!\r\n") );
			}else if(retval == 0){
				/* Content-Length found, but buffer size is not enough to get the length */
                MANGO_DBG(MANGO_DBG_LEVEL_SM, ("CONTENT LENGTH TOO BIG!\r\n") );
				mangoSM_EXITERR(MANGO_ERR_TEMPBUFSMALL, hc);
				mangoSM_ENTER(mangoSM__ABORTED, hc);
			}else{
				/* HTTP response with CONTENT-LENGTH */
                
                MANGO_DBG(MANGO_DBG_LEVEL_SM, ("Content-Length is %s bytes\r\n", headerValueBuf) );
                
				if(hc->httpMethod != MANGO_HTTP_METHOD_HEAD){
					hc->inputDataProcessor = mangoIDP_raw;
                    if(mangoHelper_decstr2dec(headerValueBuf, &hc->IDPArgsRaw.fileSz)){
                        MANGO_DBG(MANGO_DBG_LEVEL_SM, ("CONTENT LENGTH ERROR!\r\n") );
                        mangoSM_EXITERR(MANGO_ERR_RESPFORMAT, hc);
                        mangoSM_ENTER(mangoSM__ABORTED, hc);
                    }
                    
                    MANGO_DBG(MANGO_DBG_LEVEL_SM, ("Content-Length is %u bytes\r\n", hc->IDPArgsRaw.fileSz) );
                    
					hc->IDPArgsRaw.fileSzProcessed = 0;
					hc->dataProcessorArgs = &hc->IDPArgsRaw;
					mangoSM_ENTER(mangoSM__HTTP_RECVING_DATA, hc);
				}else{
					/* HTTP request completed */
					mangoSM_EXITERR(hc->httpResponseStatusCode, hc);
					mangoSM_ENTER(mangoSM__HTTP_CONNECTED, hc);
				}
			}
			
			/*
			* Content-Length not found, maybe this is a CHUNKED transfer-coding ?
			*/
			retval = mangoHelper_httpHeaderGet((char*) MANGO_WB_PTR(hc), MANGO_HDR__TRANSFER_ENCODING, headerValueBuf, sizeof(headerValueBuf));
			if(retval < 0){
				MANGO_DBG(MANGO_DBG_LEVEL_SM, ("NOT A CHUNKED RESPONSE!\r\n") );
			}else if(retval == 0){
				mangoSM_EXITERR(MANGO_ERR_TEMPBUFSMALL, hc);
				mangoSM_ENTER(mangoSM__ABORTED, hc);
				break;
			}else{
				/* CHUNKED HTTP response */
				
				/*
				* TODO: verify it is indeed chunked encoding..
				*/
				
				MANGO_DBG(MANGO_DBG_LEVEL_SM, ("This is a CHUNKED RESPONSE!\r\n") );

				if(hc->httpMethod != MANGO_HTTP_METHOD_HEAD){
					hc->inputDataProcessor = mangoIDP_chunked;
					hc->IDPArgsChunked.state 			= 1;
					hc->IDPArgsChunked.chunkSz 			= 0;
					hc->IDPArgsChunked.chunkSzProcessed	= 0;
					hc->dataProcessorArgs = &hc->IDPArgsChunked;
					mangoSM_ENTER(mangoSM__HTTP_RECVING_DATA, hc);
				}else{
					/* HTTP request completed */
					mangoSM_EXITERR(hc->httpResponseStatusCode, hc);
					mangoSM_ENTER(mangoSM__HTTP_CONNECTED, hc);
				}
			}
			
			/*
			* According to RFC HTTP Responses with the status codes below should not have a HTTP Body:
			*	1xx: 100 Continue and 101 Switching Protocols 
			*	204: No Content
			*	205: Reset Content
			*	304: 304 Not Modified
			*/
			
			/* 
			* Special case: Shoutcast [ICY 200 responses have no content-length] 
			*/
			if(hc->httpResponseStatusCode == MANGO_ERR_HTTP_200 && memcmp(MANGO_WB_PTR(hc), "ICY ", 4) == 0){
				hc->inputDataProcessor = mangoIDP_raw;
				hc->IDPArgsRaw.fileSz = MANGO_FILE_SZ_INFINITE;
				hc->IDPArgsRaw.fileSzProcessed = 0;
				hc->dataProcessorArgs = &hc->IDPArgsRaw;
				mangoSM_ENTER(mangoSM__HTTP_RECVING_DATA, hc);
			}
			
			/* 
			* Special case: Websockets [They have no content-length] 
			*/
			if(hc->httpResponseStatusCode == MANGO_ERR_HTTP_101){
				MANGO_DBG(MANGO_DBG_LEVEL_SM, ("SWITCHING TO WEB SOCKETS!\r\n") );
				
				hc->inputDataProcessor = mangoIDP_websocket;
				hc->IDPArgsWebsocket.state = 1;
                hc->IDPArgsWebsocket.frameID = 0;
				hc->dataProcessorArgs = &hc->IDPArgsWebsocket;

                mangoSM_EXITERR(MANGO_ERR_HTTP_101, hc);
				mangoSM_ENTER(mangoSM__WS_CONNECTED, hc);
			}
			
			/* 
			* Special case: Received "Expect: 100-continue" for a POST/PUT request with expectation
			*/
			if(hc->httpResponseStatusCode == MANGO_ERR_HTTP_100 && (hc->httpMethod == MANGO_HTTP_METHOD_POST || hc->httpMethod == MANGO_HTTP_METHOD_PUT)){
				mangoSM_EXITERR(MANGO_ERR_HTTP_100, hc);
				mangoSM_ENTER(mangoSM__HTTP_SENDING_DATA, hc);
			}
			
			/* 
			* Special case: Unknown case, we are going to suppose it provided a content-length: 0 header
			* and let the IDP to handle it as such
			*/
			hc->inputDataProcessor = mangoIDP_raw;
			hc->IDPArgsRaw.fileSz = 0;
			hc->IDPArgsRaw.fileSzProcessed = 0;
			hc->dataProcessorArgs = &hc->IDPArgsRaw;
			mangoSM_ENTER(mangoSM__HTTP_RECVING_DATA, hc);

			break;
        }
		case EVENT_TIMEOUT:
        {
			mangoSM_EXITERR(MANGO_ERR_RESPTIMEOUT, hc);
			mangoSM_ENTER(mangoSM__ABORTED, hc);
			break;
        }
        default:
		{
            MANGO_ENSURE(0, ("?") );
			break;
		}
	}
}



void mangoSM__HTTP_RECVING_DATA(mangoEvent_e event, mangoHttpClient_t* hc){
	uint32_t processed;
    mangoErr_t err;
    int retval;
    
    MANGO_DBG(MANGO_DBG_LEVEL_SM, ("STATE %s, EVENT %u\r\n", __func__, event) );
    
	switch(event){
		case EVENT_ENTRY:
        {
			/* This timeout will be reset every time we receive new data */
			mangoSM_TIMEOUT(MANGO_HTTP_RESPONSE_TIMEOUT_MS, hc);
			
            mangoSM_SUBSCRIBE(EVENT_PROCESS, hc);
            
			/* 
			* Shrink working buffer. HTTP data may have been received 
			* during the period where HTTP response was read.
			*/
            mangoWB_shrink(hc);

			break;
        }
        case EVENT_READ:
        {
            /* Ask new data from the socket */
            retval = mangoSocket_read(hc, MANGO_WB_FREE_PTR(hc), MANGO_WB_FREE_SZ(hc), hc->smEventTimeout); 
            if(retval < 0){
                /* Connection error */
                mangoSM_EXITERR(MANGO_ERR_CONNECTION, hc);
                mangoSM_ENTER(mangoSM__DISCONNECTED, hc);
            }else if(retval == 0){
                /* No data received */
            }else{
                /* New data received */
                hc->workingBufferIndexRight += retval;
                MANGO_WB_NULLTERMINATE();
				
                MANGO_ENSURE(hc->workingBufferIndexRight <= MANGO_WB_TOT_SZ(hc), ("?") );
                
				/* Reset timeout */
				mangoSM_TIMEOUT(MANGO_HTTP_RESPONSE_TIMEOUT_MS, hc);
				
                /* Process the data */
                mangoSM_SUBSCRIBE(EVENT_PROCESS, hc);
                return;
            }  

            break;
        }
		case EVENT_PROCESS:
        {
			/*
			* NOTE: Input data processors should deal with the case were content-length is 0
			*/
            hc->dataProcessorCompleted = 0;
            err = hc->inputDataProcessor(hc, MANGO_WB_USED_PTR(hc), MANGO_WB_USED_SZ(hc), hc->dataProcessorArgs, &processed, &hc->dataProcessorCompleted);
            if(err != MANGO_OK){
                switch(err){
                    case MANGO_ERR_MOREDATANEEDED:
                    {
                        /* The processor needs more data to continue.. */
                        if(MANGO_WB_FREE_SZ(hc) == 0){
                            /* ..but we have no space anyway */
                            mangoSM_EXITERR(MANGO_ERR_WORKBUFSMALL, hc);
                            mangoSM_ENTER(mangoSM__ABORTED, hc);
                        }else{
                            mangoSM_SUBSCRIBE(EVENT_READ, hc);
                            return;
                        }
                        break;
                    }
                    default:
                    {
                        /* Other generic non-recoverable processing error */
                        mangoSM_EXITERR(err, hc);
                        mangoSM_ENTER(mangoSM__ABORTED, hc);
                        break;
                    }
                };
            }else{
                MANGO_DBG(MANGO_DBG_LEVEL_SM, ("[%u processed]\r\n", processed) );
                /* Zero or more data were processed */
                if(hc->dataProcessorCompleted){
                    /* Processing completed succesfully */
					mangoSM_EXITERR(hc->httpResponseStatusCode, hc);
                    mangoSM_ENTER(mangoSM__HTTP_CONNECTED, hc);
                }else{
                    /* Some data were processed so we can shrink the buffer */
                    hc->workingBufferIndexLeft += processed;
                    mangoWB_shrink(hc);

                    /* 
                    * We are going to move to the READ state only if: 
                    * [1] The WB is empty. In this case we certainly have to read new data [else completed would be 1].
                    * [2] The processor needs more data to continue (already handled)
                    */ 
                    if(MANGO_WB_USED_SZ(hc) == 0){
                        mangoSM_SUBSCRIBE(EVENT_READ, hc);
                        return;
                    }
                }
            }
            
            break;
        }
		case EVENT_TIMEOUT:
        {
			mangoSM_EXITERR(MANGO_ERR_RESPTIMEOUT, hc);
			mangoSM_ENTER(mangoSM__ABORTED, hc);
			break;
        }
		default:
        {
            MANGO_ENSURE(0, ("?") );
			break;
        }
	}
}


void mangoSM__HTTP_SENDING_DATA(mangoEvent_e event, mangoHttpClient_t* hc){
    
    MANGO_DBG(MANGO_DBG_LEVEL_SM, ("STATE %s, EVENT %u\r\n", __func__, event) );
    
	switch(event){
		case EVENT_ENTRY:
            /*
            * State machine is going to exit as we do not subsribe to any event.
			* User should continue through the 'httpDataSend' API call
            */
			hc->workingBufferIndexLeft  = 0;
            hc->workingBufferIndexRight = 0;
			break;
		case EVENT_APICALL_httpDataSend:
			mangoSM_ENTER(mangoSM__HTTP_SENDING_PACKET, hc);
			break;
        case EVENT_APICALL_httpRequestProcess:
		case EVENT_APICALL_wsPoll:
		case EVENT_APICALL_wsFrameSend:
		case EVENT_APICALL_wsClose:
            mangoSM_EXITERR(MANGO_ERR_APICALLNOTSUPPORTED, hc);
			break;
        default:
            MANGO_ENSURE(0, ("?") );
			break;	
	}
	
}


void mangoSM__HTTP_SENDING_PACKET(mangoEvent_e event, mangoHttpClient_t* hc){
	uint32_t processed;
    mangoErr_t err;

    MANGO_DBG(MANGO_DBG_LEVEL_SM, ("STATE %s, EVENT %u\r\n", __func__, event) );
    
	switch(event){
		case EVENT_ENTRY:
        {
			mangoSM_SUBSCRIBE(EVENT_WRITE, hc);

			break;
        }
		case EVENT_WRITE:
        {
            MANGO_DBG(MANGO_DBG_LEVEL_SM, ("EVENT EVENT_APICALL_httpDataSend !!!!!!!\r\n") );
			
			mangoHTTPDataSendArgs_t* HTTPDataSendArgs = (mangoHTTPDataSendArgs_t*) hc->smAPICallArgs;
            
			err = hc->outputDataProcessor(hc, HTTPDataSendArgs->buf, HTTPDataSendArgs->buflen, hc->dataProcessorArgs, &processed, &hc->dataProcessorCompleted);
			if(err != MANGO_OK){
				mangoSM_EXITERR(err, hc);
				mangoSM_ENTER(mangoSM__ABORTED, hc);
            }else{
				if(hc->dataProcessorCompleted){
					/* ODP completed, all data were sent */
					mangoSM_ENTER(mangoSM__HTTP_RECVING_HEADERS, hc);
				}else{
					/* We have not finished yet, wait for more data from the application */
					mangoSM_ENTER(mangoSM__HTTP_SENDING_DATA, hc);
				}
            }
			break;
        }
        case EVENT_TIMEOUT:
        {
            MANGO_DBG(MANGO_DBG_LEVEL_SM, ("EVENT TIMEOUT !!!!!!!\r\n") );
			mangoSM_ENTER(mangoSM__ABORTED, hc);
			break;
        }
        default:
        {
            MANGO_ENSURE(0, ("?") );
			break;
        }
	};
}


void mangoSM__WS_CONNECTED(mangoEvent_e event, mangoHttpClient_t* hc){
    
    MANGO_DBG(MANGO_DBG_LEVEL_SM, ("STATE %s, EVENT %u\r\n", __func__, event) );
    
	switch(event){
		case EVENT_ENTRY:
            /*
            * State machine is going to exit as we do not subsribe to any event
            */

			/* 
			* Shrink working buffer. Websocket data may have been received 
			* during the period where HTTP response was read.
			*/
            mangoWB_shrink(hc);
			
			break;
        case EVENT_APICALL_httpRequestProcess:
		case EVENT_APICALL_httpDataSend:
			/*
			* We cannot start a new HTTP request/data if we have
			* upgraded the connection..
			*/
			mangoSM_EXITERR(MANGO_ERR_APICALLNOTSUPPORTED, hc);
			break;
		case EVENT_APICALL_wsPoll:
			mangoSM_ENTER(mangoSM__WS_POLLING, hc);
			break;
		case EVENT_APICALL_wsFrameSend:
            mangoSM_ENTER(mangoSM__WS_SENDING_FRAME, hc);
			break;
		case EVENT_APICALL_wsClose:
			mangoSM_ENTER(mangoSM__WS_CLOSING, hc);
			break;
        default:
            MANGO_ENSURE(0 , ("?") );
			break;	
	}
}


void mangoSM__WS_POLLING(mangoEvent_e event, mangoHttpClient_t* hc){
    uint32_t processed;
    mangoErr_t err;
    int retval;
    
    MANGO_DBG(MANGO_DBG_LEVEL_SM, ("STATE %s, EVENT %u\r\n", __func__, event) );
    
	switch(event){
		case EVENT_ENTRY:
        {
			mangoWSPollArgs_t* WSPollArgs = (mangoWSPollArgs_t*) hc->smAPICallArgs;

			mangoSM_TIMEOUT(WSPollArgs->timeout, hc); 
			
			mangoSM_SUBSCRIBE(EVENT_PROCESS, hc);
		}
        case EVENT_PROCESS:
        {
            hc->dataProcessorCompleted = 0;
            err = hc->inputDataProcessor(hc, MANGO_WB_USED_PTR(hc), MANGO_WB_USED_SZ(hc), hc->dataProcessorArgs, &processed, &hc->dataProcessorCompleted);
            if(err != MANGO_OK){
                switch(err){
                    case MANGO_ERR_MOREDATANEEDED:
                    {
                        /* The processor needs more data to continue.. */
                        if(MANGO_WB_FREE_SZ(hc) == 0){
                            /* ..but we have no space anyway */
                            mangoSM_EXITERR(MANGO_ERR_WORKBUFSMALL, hc);
                            mangoSM_ENTER(mangoSM__ABORTED, hc);
                        }else{
                            mangoSM_SUBSCRIBE(EVENT_READ, hc);
                            return;
                        }
                        break;
                    }
                    case MANGO_ERR_WEBSOCKETCLOSED:
                    {
                        /* Close packet received */
                        mangoSM_EXITERR(MANGO_ERR_WEBSOCKETCLOSED, hc);
                        mangoSM_ENTER(mangoSM__DISCONNECTED, hc);
                        break;
                    }
                    default:
                    {
                        /* Other generic non-recoverable processing error */
                        mangoSM_EXITERR(err, hc);
                        mangoSM_ENTER(mangoSM__ABORTED, hc);
                        break;
                    }
                };
                mangoSM_EXITERR(MANGO_ERR_CONNECTION, hc);
                mangoSM_ENTER(mangoSM__DISCONNECTED, hc);
            }else{
                /* Some data were processed so we can shrink the buffer */
                hc->workingBufferIndexLeft += processed;
                mangoWB_shrink(hc);
                
                /* 
                * We are going to move to the READ state only if: 
                * [1] The WB is empty. In this case we certainly have to read new data [else completed would be 1].
                * [2] The processor needs more data to continue (already handled)
                */ 
                if(MANGO_WB_USED_SZ(hc) == 0){
                    mangoSM_SUBSCRIBE(EVENT_READ, hc);
                    return;
                }else{
                    
                }
            }
            
            break;
        }
        case EVENT_READ:
        {
            
            /* Ask new data from the socket */
            retval = mangoSocket_read(hc, MANGO_WB_FREE_PTR(hc), MANGO_WB_FREE_SZ(hc), hc->smEventTimeout); 
            if(retval < 0){
                /* Connection error */
                mangoSM_EXITERR(MANGO_ERR_CONNECTION, hc);
                mangoSM_ENTER(mangoSM__DISCONNECTED, hc);
            }else if(retval == 0){
                /* No data received */
            }else{
                /* New data received */
                hc->workingBufferIndexRight += retval;
                MANGO_WB_NULLTERMINATE();
				
                MANGO_ENSURE(hc->workingBufferIndexRight <= MANGO_WB_TOT_SZ(hc), ("?") );
                
                /* Process the data */
                mangoSM_SUBSCRIBE(EVENT_PROCESS, hc);
                return;
            }  
            
            break;
        }
        case EVENT_TIMEOUT:
        {
            MANGO_DBG(MANGO_DBG_LEVEL_SM, ("EVENT TIMEOUT !!!!!!!\r\n") );
			mangoSM_ENTER(mangoSM__WS_CONNECTED, hc);
			break;
        }
        default:
        {
            MANGO_ENSURE(0, ("?") );
			break;
        }
	};
}

void mangoSM__WS_SENDING_FRAME(mangoEvent_e event, mangoHttpClient_t* hc){
    mangoErr_t err;

    MANGO_DBG(MANGO_DBG_LEVEL_SM, ("STATE %s, EVENT %u\r\n", __func__, event) );
    
	switch(event){
		case EVENT_ENTRY:
        {
			mangoSM_SUBSCRIBE(EVENT_WRITE, hc);

			break;
        }
		case EVENT_WRITE:
        {
           MANGO_DBG(MANGO_DBG_LEVEL_SM, ("EVENT EVENT_APICALL_wsFrameSend !!!!!!!\r\n") );
			
			mangoWSFrameSendArgs_t* WSFrameSendArgs = (mangoWSFrameSendArgs_t*) hc->smAPICallArgs;

			err =  mangoWS_frameSend(hc->socketfd, WSFrameSendArgs->buf, WSFrameSendArgs->buflen, WSFrameSendArgs->type);
			if(err != MANGO_OK){
				mangoSM_ENTER(mangoSM__ABORTED, hc);
			}else{
				mangoSM_ENTER(mangoSM__WS_CONNECTED, hc);
			}

			break;
        }
        case EVENT_TIMEOUT:
        {
            MANGO_DBG(MANGO_DBG_LEVEL_SM, ("EVENT TIMEOUT !!!!!!!\r\n") );
			mangoSM_ENTER(mangoSM__ABORTED, hc);
			break;
        }
        default:
        {
            MANGO_ENSURE(0, ("?") );
			break;
        }
	};
}

void mangoSM__WS_CLOSING(mangoEvent_e event, mangoHttpClient_t* hc){
    mangoErr_t err;

    MANGO_DBG(MANGO_DBG_LEVEL_SM, ("STATE %s, EVENT %u\r\n", __func__, event) );
    
	switch(event){
		case EVENT_ENTRY:
        {
			mangoSM_SUBSCRIBE(EVENT_WRITE, hc);

			break;
        }
		case EVENT_WRITE:
        {
            MANGO_DBG(MANGO_DBG_LEVEL_SM, ("EVENT EVENT_APICALL_wsClose !!!!!!!\r\n") );
			
			err =  mangoWS_close(hc->socketfd);
			if(err != MANGO_OK){
				mangoSM_ENTER(mangoSM__ABORTED, hc);
			}else{
				mangoSM_ENTER(mangoSM__DISCONNECTED, hc);
			}

			break;
        }
        case EVENT_TIMEOUT:
        {
            MANGO_DBG(MANGO_DBG_LEVEL_SM, ("EVENT TIMEOUT !!!!!!!\r\n") );
			mangoSM_ENTER(mangoSM__ABORTED, hc);
			break;
        }
        default:
        {
            MANGO_ENSURE(0, ("?") );
			break;
        }
	};
}

/* -----------------------------------------------------------------------------------------------------------------
| HELPER FUNCTIONS
----------------------------------------------------------------------------------------------------------------- */

void mangoWB_shrink(mangoHttpClient_t* hc){
    
    /* The last byte of working buffer is used for string termination */
    MANGO_ENSURE(hc->workingBufferIndexRight <= MANGO_WB_TOT_SZ(hc), ("?") );
    MANGO_ENSURE(hc->workingBufferIndexLeft <= hc->workingBufferIndexRight, ("?") );
    
    if(hc->workingBufferIndexLeft == 0){
        /* Cannot shrink */
    }else{
        if(hc->workingBufferIndexLeft == hc->workingBufferIndexRight){
            hc->workingBufferIndexLeft  = 0;
            hc->workingBufferIndexRight = 0;
        }else{
            uint16_t index = 0;
            
            while(hc->workingBufferIndexLeft < hc->workingBufferIndexRight){
                hc->workingBuffer[index++] = hc->workingBuffer[hc->workingBufferIndexLeft++];
            };
            
            hc->workingBufferIndexLeft  = 0;
            hc->workingBufferIndexRight = index;
        }
    }
	
    MANGO_WB_NULLTERMINATE();
}


/* -----------------------------------------------------------------------------------------------------------------
| HELP FUNCTIONS
----------------------------------------------------------------------------------------------------------------- */

int mangoSocket_read(mangoHttpClient_t* hc, uint8_t* data, uint16_t datalen, uint32_t timeout){
    int retval;
    
    if(!timeout) {timeout = 1;}
    
    retval = mangoPort_read(hc->socketfd, data, datalen, timeout);
    if(retval <= 0){
        
    }else{
		hc->stats.rxBytes += retval;
    }
	
	return retval;
}


int mangoSocket_write(mangoHttpClient_t* hc, uint8_t* data, uint16_t datalen, uint32_t timeout){
    int retval;
    
    if(!timeout) {timeout = 1;}
    
    retval = mangoPort_write(hc->socketfd, data, datalen, timeout);
    if(retval <= 0){
        
    }else{
		hc->stats.txBytes += retval;
    }
    
    return retval;
}
