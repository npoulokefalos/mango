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

mangoErr_t mangoWS_close(int socketfd){
	return mangoWS_frameSend(socketfd, NULL, 0, MANGO_WS_FRAME_TYPE_CLOSE);
}

mangoErr_t mangoWS_pong(int socketfd){
	return mangoWS_frameSend(socketfd, NULL, 0, MANGO_WS_FRAME_TYPE_PONG);
}

mangoErr_t mangoWS_frameSend(int socketfd, uint8_t* buf, uint32_t buflen, mangoWsFrameType_t type){
	uint8_t frame0[8];
	uint8_t buf0[2];
    uint8_t maskingkey[4];
    uint32_t i, k;
    int retval;
	uint8_t* frame;
	uint32_t framelen;
	

	/* 
	* Randomize masking key 
	*/
	maskingkey[0] = mangoPort_timeNow();
	maskingkey[1] = mangoPort_timeNow() >> 8;
	maskingkey[2] = maskingkey[0] & maskingkey[1];
	maskingkey[3] = maskingkey[1] ^ maskingkey[2];	
	
						 
    switch(type){
        case MANGO_WS_FRAME_TYPE_CONT:
            /* Fragmentation on the Tx path currently not supported */
            MANGO_ENSURE(0, ("?") );
            break;
        case MANGO_WS_FRAME_TYPE_PING:
            /* Clients are not allowed to ping the server  */
            MANGO_ENSURE(0, ("?") );
            break;
        case MANGO_WS_FRAME_TYPE_PONG:
			MANGO_DBG(MANGO_DBG_LEVEL_WS, ("--------------> OPCODE: PONG!\r\n"));
			buflen = 0;
            break;
        case MANGO_WS_FRAME_TYPE_CLOSE:
			MANGO_DBG(MANGO_DBG_LEVEL_WS, ("--------------> OPCODE: CLOSE!\r\n"));
			buf = buf0;
			
			/* Status code 1000, normal closure */
			buf0[0] = (0x03e8 & 0xff00) >> 8;
			buf0[1] = (0x03e8 & 0x00ff);
			
            buflen = 2;
            break;
        case MANGO_WS_FRAME_TYPE_TEXT:
            MANGO_DBG(MANGO_DBG_LEVEL_WS, ("--------------> OPCODE: TEXT!\r\n"));
            break;
        case MANGO_WS_FRAME_TYPE_BINARY:
            MANGO_DBG(MANGO_DBG_LEVEL_WS, ("--------------> OPCODE: BINARY!\r\n"));
            break;
        default:
            MANGO_DBG(MANGO_DBG_LEVEL_WS, ("--------------> OPCODE: ???\r\n"));
            break;
    }
    
	/*
    * Allocate memory for the frame
    */
	if((type == MANGO_WS_FRAME_TYPE_PONG) || (type == MANGO_WS_FRAME_TYPE_CLOSE)){
		frame = frame0;
	}else{
		frame = mangoPort_malloc(buflen + 14 /* max frame header size */);
		if(!frame){
			return 0;
		}
	}
			
	
    /*
    * Build frame's header
    */
	i = 0;
	if(buflen <= 125){ 
        /* 0 -> 125 */
		/* Opcode */
        framelen = buflen + 6;
        frame[i++] = 0x80 | type;
        /* Payload len */
        frame[i++] = 0x80 | buflen;
        /* Masking */
        frame[i++] = maskingkey[0];
        frame[i++] = maskingkey[1];
        frame[i++] = maskingkey[2];
        frame[i++] = maskingkey[3];
    }else if(buflen <= 0xffff) { 
        /* 126 -> 65,535 */
		/* Opcode */
        framelen = buflen + 8;
        frame[i++] = 0x80 | type;
        /* Payload len */
        frame[i++] = 0x80 |126;
        frame[i++] = (buflen >> 8) & 0xff;
        frame[i++] = (buflen) & 0xff;
        /* Masking */
        frame[i++] = maskingkey[0];
        frame[i++] = maskingkey[1];
        frame[i++] = maskingkey[2];
        frame[i++] = maskingkey[3];
    }else{ 
		/* 65,536 -> 4,294,967,295 */
        /* Opcode */
        framelen = buflen + 14;
        frame[i++] = 0x80 | type;
        /* Payload len */
        frame[i++] = 0x80 | 127;
        frame[i++] = 0;
        frame[i++] = 0;
        frame[i++] = 0;
        frame[i++] = 0;
        frame[i++] = (buflen >> 24) & 0xff;
        frame[i++] = (buflen >> 16) & 0xff;
        frame[i++] = (buflen >> 8) & 0xff;
        frame[i++] = (buflen) & 0xff;
        /* Masking */
        frame[i++] = maskingkey[0];
        frame[i++] = maskingkey[1];
        frame[i++] = maskingkey[2];
        frame[i++] = maskingkey[3];
    }

	/*
    * Mask frame's payload
    */
	k = 0;
    while(buflen >= 8){
        frame[i++] = buf[k++] ^ maskingkey[0]; 
        frame[i++] = buf[k++] ^ maskingkey[1]; 
        frame[i++] = buf[k++] ^ maskingkey[2]; 
        frame[i++] = buf[k++] ^ maskingkey[3]; 
        frame[i++] = buf[k++] ^ maskingkey[0]; 
        frame[i++] = buf[k++] ^ maskingkey[1]; 
        frame[i++] = buf[k++] ^ maskingkey[2];
        frame[i++] = buf[k++] ^ maskingkey[3];
        buflen -= 8;
    }
	
    while(buflen > 0){
        frame[i++] = buf[k] ^ maskingkey[k % 4]; k++;
        buflen--;
    }

	/*
	* Send the frame.
	* It's a do or die here so we give a big enough timeout. 
	* If we don't manage to send the whole frame (but only part of it)
	* we have to abort the websocket connection.
	*/
	MANGO_DBG(MANGO_DBG_LEVEL_WS, ("Sending websocket frame with size %u bytes..\r\n", framelen));
              
    retval = mangoPort_write(socketfd, frame, framelen, MANGO_SOCKET_WRITE_TIMEOUT_MS);

	MANGO_DBG(MANGO_DBG_LEVEL_WS, ("%d/%d bytes sent..\r\n", retval, framelen));
	
	if(frame != frame0){
		mangoPort_free(frame);
	}
	
	if(retval < 0){
		/* Connection closed */
		return MANGO_ERR;
	}else{
		if(retval == framelen){
			/* The whole frame was transmitted */
			return MANGO_OK;
		}else{
			/* 
			* None all part of the frame was transmitted. 
			* Given the big timeout of mangoWS_frameSend() this is not normal
			* so we have to abort the connection.
			*/
			return MANGO_ERR;
		}
	}
}
