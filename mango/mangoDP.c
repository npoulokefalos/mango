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

/*
 * Pass-through input data processor for non-chunked input data
*/
mangoErr_t mangoIDP_raw(mangoHttpClient_t* hc, uint8_t* buf, uint16_t buflen, void* vargs, uint32_t* processed, uint8_t* completed){
    mangoIDPArgsRaw_t* args = (mangoIDPArgsRaw_t*) vargs;
    mangoArg_t funcArgs;
    mangoErr_t err;
	uint8_t oldByte;
    
	MANGO_DBG(MANGO_DBG_LEVEL_DP, ("mangoIDP_raw() %u bytes...............\r\n", buflen) );
    
    *completed = 0;
    *processed = 0;
    
	/*
	* "Content-length: 0" case
	*/
	if(args->fileSz == 0){
		*completed = 1;
		return MANGO_OK; 
	}
	
    /*
    * IDP may be called with buflen 0 and should return
    * MANGO_ERR_MOREDATANEEDED to trigger socket reading
    */
    if(buflen == 0){
        return MANGO_ERR_MOREDATANEEDED;
    }else{
		buf[buflen] = '\0';
	}
    
	args->fileSzProcessed += buflen;
	*processed = buflen;
	if(args->fileSzProcessed == args->fileSz){
		*completed = 1;
	}
	
	MANGO_DBG(MANGO_DBG_LEVEL_DP, ("IDP %u/%u received!\r\n", args->fileSzProcessed, args->fileSz) );
	 
    /*
    *   Pass the data to application layer
    */
    funcArgs.buf = buf;
    funcArgs.buflen = buflen;
    funcArgs.argType = MANGO_ARG_TYPE_HTTP_DATA_RECEIVED;

    oldByte = funcArgs.buf[funcArgs.buflen];
    funcArgs.buf[funcArgs.buflen] = '\0';
    
    err = hc->userFunc(&funcArgs, hc->userArgs);
    
    funcArgs.buf[funcArgs.buflen] = oldByte;
    
	if(err != MANGO_OK){
		return MANGO_ERR_APPABORTED;
	}else{
		 return MANGO_OK;
	}

}



/*
 * Input data processor for chunked input data
 *
 *	------------------------------------------------------------------
 *	RFC2616
 *	------------------------------------------------------------------
 *	Chunked-Body   = *chunk
 *					last-chunk
 *					trailer
 *					CRLF
 *
 *	chunk          = chunk-size [ chunk-extension ] CRLF
 *					chunk-data CRLF
 *	chunk-size     = 1*HEX
 *	last-chunk     = 1*("0") [ chunk-extension ] CRLF
 *
 *	chunk-extension= *( ";" chunk-ext-name [ "=" chunk-ext-val ] )
 *	chunk-ext-name = token
 *	chunk-ext-val  = token | quoted-string
 *	chunk-data     = chunk-size(OCTET)
 *	trailer        = *(entity-header CRLF)
 *	------------------------------------------------------------------
*/
mangoErr_t mangoIDP_chunked(mangoHttpClient_t* hc, uint8_t* buf, uint16_t buflen, void* vargs, uint32_t* processed, uint8_t* completed){
	mangoIDPArgsChunked_t* args = (mangoIDPArgsChunked_t*) vargs;
	uint8_t* buf0;
	uint8_t sz;
	uint32_t maxReadSz;
	mangoArg_t funcArgs;
    mangoErr_t err;
	uint8_t oldByte;
    
    *completed = 0;
    *processed = 0;
    
#define RETURN()						if(*completed){return MANGO_OK;}else{if(*processed){return MANGO_OK;}else{return MANGO_ERR_MOREDATANEEDED;}}
#define MOVETO(newState, processSz)		args->state = (newState); *processed += (processSz); buf += (processSz); buflen -= (processSz); break;	

	while(1){
		if(!buflen){ RETURN(); }
		buf0 = buf;
		switch(args->state){
			case 1: /* Chunk size */
			{
				while(*buf != '\n' && *buf){ buf++; }
				
				if(!*buf){ RETURN(); }

				if(mangoHelper_hexstr2dec((char*)buf0, &args->chunkSz)){
					/* Fatal error [chunk size parsing error] */
					return MANGO_ERR_DATAPROCESSING;
				}
				
				args->chunkSzProcessed = 0;

				sz = ++buf - buf0;
				
				buf -= sz; /* buf is already increased, so we have to decrease it */
				
				if(args->chunkSz){
					MOVETO(2, sz);
				}else{
					MOVETO(4, sz);
				}
			}
			case 2:
			{
				maxReadSz = buflen > args->chunkSz - args->chunkSzProcessed ? args->chunkSz - args->chunkSzProcessed : buflen;
				
				if(maxReadSz){
					/*
					*  Pass the data to application layer
					*/
					funcArgs.buf = buf;
					funcArgs.buflen = maxReadSz;
					funcArgs.argType = MANGO_ARG_TYPE_HTTP_DATA_RECEIVED;

                    oldByte = funcArgs.buf[funcArgs.buflen];
                    funcArgs.buf[funcArgs.buflen] = '\0';
                    
                    err = hc->userFunc(&funcArgs, hc->userArgs);
                    
                    funcArgs.buf[funcArgs.buflen] = oldByte;
                    
					if(err != MANGO_OK){
						/* Application wants to abort */
						return MANGO_ERR_APPABORTED;
					}

					args->chunkSzProcessed += maxReadSz;
					if(args->chunkSz == args->chunkSzProcessed){ 
						MOVETO(3, maxReadSz); 
					}else{
						/* Same sate */
						MOVETO(2, maxReadSz); 
					}
				}
				
				/* Never reach here */
				MANGO_ENSURE(0, ("?") );
				return MANGO_ERR_DATAPROCESSING;
			};	
			case 3:
			{
				if(buflen < 2){ 
					RETURN(); 
				}
				
				if(buf[0] == '\r' && buf[1] == '\n'){
					MOVETO(1, 2);
				}else{
					/* Fatal error [expect CRLF] */
					return MANGO_ERR_DATAPROCESSING;
				}
			}	
			case 4:
			{
				/*
				*	RFC 2616: Trailing headers are acceptable:
				*	4\r\n
				*	Test\r\n 
				*	0\r\n 
				*	User-Agent: Bar\r\n
				*	Evil-header: foobar\r\n
				*	\r\n
				*/
				
				if(buflen < 2){ 
					RETURN(); 
				}
				
				if(buflen == 2 && buf[0] == '\r' && buf[1] == '\n'){
					/* Completed, no extra HTTP headers */
					*completed = 1;
					return MANGO_OK;
				}else if(buflen > 3 && buf[buflen - 4] == '\r' && buf[buflen - 3] == '\n' && buf[buflen - 2] == '\r' && buf[buflen - 1] == '\n'){
					/*
					* Completed, extra HTTP headers found.
					* Pass the HTTP headers to the application
					*/
					funcArgs.buf = buf;
					funcArgs.buflen = buflen;
					funcArgs.statusCode = 0;
					funcArgs.argType = MANGO_ARG_TYPE_HTTP_RESP_RECEIVED;
					hc->userFunc(&funcArgs, hc->userArgs);
					
					*completed = 1;
					return MANGO_OK;
				}else{
					RETURN(); 
				}
			}
		}; // switch()
	}; // while(1)
	
#undef RETURN
#undef MOVETO

	/* Never reach here */
	MANGO_ENSURE(0, ("?") );
	
	return MANGO_ERR_DATAPROCESSING;
}

/*
 * Pass-through output data processor for non-chunked input data
*/
mangoErr_t mangoODP_raw(mangoHttpClient_t* hc, uint8_t* buf, uint16_t buflen, void* vargs, uint32_t* processed, uint8_t* completed){
    mangoODPArgsRaw_t* args = (mangoODPArgsRaw_t*) vargs;
    int retval;
	
    MANGO_DBG(MANGO_DBG_LEVEL_DP, ("ODP DATA %u bytes:\r\n[%s]\r\n", buflen, buf) );
    
    *completed = 0;
    *processed = 0;
    
    
    if(buflen == 0){
        /* Application sent all data */
        MANGO_DBG(MANGO_DBG_LEVEL_DP, ("Alla data sent indication\r\n") );
        
        if(args->fileSzProcessed + buflen > args->fileSz){
            /* Application sent less data than expected */
            MANGO_DBG(MANGO_DBG_LEVEL_DP, ("Content-Length was wrong: More data expected\r\n") );
            return MANGO_ERR_CONTENTLENGTH;
        }
        
        *completed = 1;
        return MANGO_OK;
    }
    
	if(args->fileSzProcessed + buflen > args->fileSz){
		/* Application sent more data than expected */
		MANGO_DBG(MANGO_DBG_LEVEL_DP, ("Content-Length was wrong: Less data expected\r\n") );
		return MANGO_ERR_CONTENTLENGTH;
	}
	
	
	//retval = mangoPort_write(hc->socketfd, buf, buflen, MANGO_SOCKET_WRITE_TIMEOUT_MS);
    retval = mangoSocket_write(hc, buf, buflen, MANGO_SOCKET_WRITE_TIMEOUT_MS);
    
	if(retval == buflen){
		*processed = buflen;
		args->fileSzProcessed += buflen;
		return MANGO_OK;
	}else{
		return MANGO_ERR_WRITETIMEOUT;
	}
}

/*
 * Output data processor for chunked output data
*/
mangoErr_t mangoODP_chunked(mangoHttpClient_t* hc, uint8_t* buf, uint16_t buflen, void* vargs, uint32_t* processed, uint8_t* completed){
    mangoODPArgsChunked_t* args = (mangoODPArgsChunked_t*) vargs;
    uint8_t extraChunkSz;
	uint16_t chunkLen;
	uint16_t sendSz;
	int sent;
    int retval;
    
    MANGO_DBG(MANGO_DBG_LEVEL_DP, ("CHUNKED HTTP ODP DATA %u bytes:\r\n[%s]\r\n", buflen, buf) );
    
    *completed = 0;
	*processed = 0;
	
	if(buflen == 0){
        /* This is the last chunk, application sent all data */
        MANGO_DBG(MANGO_DBG_LEVEL_DP, ("All data sent indication\r\n") );
        
		chunkLen = strlen("0\r\n\r\n");
		
		if(chunkLen > args->workingBufferSz){
			return MANGO_ERR_WORKBUFSMALL;
		}
		
		memcpy(args->workingBuffer, "0\r\n\r\n", chunkLen);
        retval = mangoSocket_write(hc, args->workingBuffer, chunkLen, MANGO_SOCKET_WRITE_TIMEOUT_MS);

		if(retval == chunkLen){
			*completed = 1;
			*processed = chunkLen;
			return MANGO_OK;
		}else{
			return MANGO_ERR_WRITETIMEOUT;
		}

	}else{
		extraChunkSz = (8 /* Strlen for chunk size in Hex */) + (2 /* CRLF before data */);
		extraChunkSz += (2 /* CRLF after data */) + (1 /* string termination */);
		
		if(extraChunkSz >= args->workingBufferSz){
			return MANGO_ERR_WORKBUFSMALL;
		}
		
		sent = 0;
		while(sent < buflen){
			sendSz = (buflen - sent > args->workingBufferSz - extraChunkSz) ? args->workingBufferSz - extraChunkSz : buflen - sent;
			
			/* Hex chunk size */
			mangoHelper_dec2hexstr(sendSz, (char*) args->workingBuffer);
			chunkLen = strlen((char*)args->workingBuffer);
			
			/* CRLF */
			memcpy(&args->workingBuffer[chunkLen], "\r\n", strlen("\r\n"));		
			chunkLen += 2;
			
			/* Data */
			memcpy(&args->workingBuffer[chunkLen], &buf[sent], sendSz); 		
			chunkLen += sendSz; 
			
			/* CRLF */
			memcpy(&args->workingBuffer[chunkLen], "\r\n", strlen("\r\n"));		
			chunkLen += 2;
			
			/* Send the chunk */
			//retval = mangoPort_write(hc->socketfd, args->workingBuffer, chunkLen, MANGO_SOCKET_WRITE_TIMEOUT_MS);
            retval = mangoSocket_write(hc, args->workingBuffer, chunkLen, MANGO_SOCKET_WRITE_TIMEOUT_MS);
			if(retval == chunkLen){
				*processed += sendSz;
				sent += sendSz;
			}else{
				return MANGO_ERR_WRITETIMEOUT;
			}
		}
		
		return MANGO_OK;
	}
}

/*
 * Output data processor for websocket output data
*/
mangoErr_t mangoODP_websocket(mangoHttpClient_t* hc, uint8_t* buf, uint16_t buflen, void* vargs, uint32_t* processed, uint8_t* completed){
    mangoWSFrameSendArgs_t* WSFrameSendArgs = (mangoWSFrameSendArgs_t*) vargs;
    mangoErr_t err;
	
    
    MANGO_DBG(MANGO_DBG_LEVEL_DP, ("ODP DATA %u bytes:\r\n[%s]\r\n", buflen, buf) );
    
    *completed = 0;
    *processed = 0;
    
	err =  mangoWS_frameSend(hc->socketfd, WSFrameSendArgs->buf, WSFrameSendArgs->buflen, WSFrameSendArgs->type);
	if(err != MANGO_OK){
		return MANGO_ERR_DATAPROCESSING;
	}else{
		*completed = 1;
		*processed = buflen;
		return MANGO_OK;
	}
}

/*
 * Input data processor for websocket input data
*/
mangoErr_t mangoIDP_websocket(mangoHttpClient_t* hc, uint8_t* buf, uint16_t buflen, void* vargs, uint32_t* processed, uint8_t* completed){
	mangoIDPArgsWebsocket_t* args = (mangoIDPArgsWebsocket_t*) vargs;
    int forever = 1;
    uint16_t maxReadSz;
    mangoArg_t funcArgs;
    uint8_t oldByte;
    
    MANGO_DBG(MANGO_DBG_LEVEL_DP, ("IDP WEBSOCKET %u bytes:\r\n[%s]\r\n", buflen, buf) );
    
    *processed = 0;
    *completed = 0;
    
#define RETURN()						if(*completed){return MANGO_OK;}else{if(*processed){return MANGO_OK;}else{return MANGO_ERR_MOREDATANEEDED;}}
#define MOVETO(newState, processSz)		args->state = (newState); *processed += (processSz); buf += (processSz); buflen -= (processSz); break;	
	
#define FRAME_FIN						(args->header[0] & 0x80)
#define FRAME_OPCODE					(args->header[0] & 0x0F)
#define FRAME_MASK						(args->header[1] & 0x80)
#define FRAME_PAYLOADLEN				(args->header[1] & 0x7F)
	
    while(forever){
		if(!buflen){ RETURN(); }
        switch(args->state){
            case 1:
            {
                MANGO_DBG(MANGO_DBG_LEVEL_DP, ("[CASE 1] buf '%s'\r\n", buf) );
                
                if(buflen < 2){
                    RETURN(); 
                }
				
				args->header[0] = buf[0];
				args->header[1] = buf[1];
					
				args->frameSzProcessed = 0;
				
				if(FRAME_MASK){
					/* Masked frame from server! Not allowed */
					MANGO_DBG(MANGO_DBG_LEVEL_DP, ("!!!!!!!!!!!!!!!!!!! abort due to masking!\r\n") );
					return MANGO_ERR_DATAPROCESSING;
				}
				
				MANGO_DBG(MANGO_DBG_LEVEL_DP, ("FRAME SZ  is '%u' [0x%x, 0x%x]\r\n", FRAME_PAYLOADLEN, args->header[0], args->header[1]) );
				if(FRAME_PAYLOADLEN < 126){
					/* This is the frame len  */
					args->frameSz = FRAME_PAYLOADLEN;
					MOVETO(2, 2);
				}else if(FRAME_PAYLOADLEN == 126){
					/* The following 2 bytes are the payload len : bytes 3 & 4 */
					if(buflen < 4){
						RETURN();
					}else{
						args->frameSz = (buf[2] << 8) | buf[3];
						MOVETO(2, 4);
					}
				}else{
					if(buflen < 10){
						/* The following 8 bytes are the payload len : 3,4,5,6,7,8,9,10 */
						RETURN();
					}else{
						if(buf[2] | buf[3] | buf[4] | buf[5]){
							/* Huge frame */
							MANGO_DBG(MANGO_DBG_LEVEL_DP, ("!!!!!!!!!!!!!!!!!!! abort due to huge!\r\n") );
							return MANGO_ERR_DATAPROCESSING;
						}else{
							args->frameSz = (buf[6] << 24) | (buf[7] << 16) | (buf[8] << 8) | buf[9];
							MOVETO(2, 10);
						}
					}
				}
				
				/* Never reach here */
				MANGO_ENSURE(0, ("?") );
				return MANGO_ERR_DATAPROCESSING;
            }
            case 2:
            {
                MANGO_DBG(MANGO_DBG_LEVEL_DP, ("[CASE 3] buf '%s', len %u\r\n", buf, buflen) );
				
				if(args->frameSz <= MANGO_WB_TOT_SZ(hc)){
					/* We are able to buffer the whole frame */
					if(args->frameSz > buflen){
						/* But we haven't received it all */
						RETURN();
					}else{
						maxReadSz = args->frameSz;
					}
				}else{
					/* We cant't buffer the whole frame, pass to the application whatever we got now */
					maxReadSz = buflen >= args->frameSz - args->frameSzProcessed ? args->frameSz - args->frameSzProcessed : buflen;
				}
				
                if(FRAME_OPCODE & 0x08){ /* Control frame */
                    switch(FRAME_OPCODE){
                        case MANGO_WS_FRAME_TYPE_CLOSE:
                        {
                            funcArgs.buf = NULL;
                            funcArgs.buflen = 0;
                            funcArgs.argType = MANGO_ARG_TYPE_WEBSOCKET_CLOSE;
                            hc->userFunc(&funcArgs, hc->userArgs);
                            
                            mangoWS_close(hc->socketfd);
                            return MANGO_ERR_WEBSOCKETCLOSED;
                            break;
                        }
                        case MANGO_WS_FRAME_TYPE_PING:
                        {
                            funcArgs.buf = NULL;
                            funcArgs.buflen = 0;
                            funcArgs.argType = MANGO_ARG_TYPE_WEBSOCKET_PING;
                            hc->userFunc(&funcArgs, hc->userArgs);
                            
                            mangoWS_pong(hc->socketfd);
                            break;
                        }
                        default:
                        {
                            /* Not supported control frame received */
                            MANGO_DBG(MANGO_DBG_LEVEL_DP, ("Unknown control frame received: %d\r\n", FRAME_OPCODE) );
                            return MANGO_ERR_DATAPROCESSING;
                        };
                    };
                }else{ /* Non-control frame */
                    
                    /*
                    * Pass the data to the application layer
                    * TODO: Null terminate this?
                    */
                    funcArgs.buf = buf;
                    funcArgs.buflen = maxReadSz;
                    funcArgs.argType = MANGO_ARG_TYPE_WEBSOCKET_DATA_RECEIVED;
                    funcArgs.frameID = args->frameID;
                    
                    oldByte = funcArgs.buf[funcArgs.buflen];
                    funcArgs.buf[funcArgs.buflen] = '\0';
                    
                    hc->userFunc(&funcArgs, hc->userArgs);
                    
                    funcArgs.buf[funcArgs.buflen] = oldByte;
                    
                }
                
				args->frameSzProcessed  += maxReadSz;

				if(args->frameSzProcessed == args->frameSz){
					if(FRAME_FIN){
                        if(FRAME_OPCODE & 0x08){
                            /* 
                            * Control frame 
                            */
                        }else{
                            /* 
                            * Non-control frame. 
                            * We increase the frameID here as we may receive control frames 
                            * in between fragmented data frames.
                            */
                            args->frameID++;
                        }
					}
					
					MOVETO(1, maxReadSz);
				}else{
					/* Same state */
					MOVETO(2, maxReadSz);
				}
            }
		}; // switch()
	}; // while(1)
	
#undef RETURN
#undef MOVETO 
#undef FRAME_FIN
#undef FRAME_OPCODE
#undef FRAME_MASK
#undef FRAME_PAYLOADLEN
    
	/* Never reach here */
	MANGO_ENSURE(0, ("?") );
	
	return MANGO_ERR_DATAPROCESSING;
}


