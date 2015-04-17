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

#define _GNU_SOURCE // This fixes the issue with strcasstr protoype "warning: assignment makes pointer from integer without a cast"
#include "mango.h"

/**
 * @brief   Checks if we've received a complete HTTP response
 *          and that the format is correct. If the format
 *          is correct the HTTP status code is returned.
 *
 * @retval  <0  if the HTTP response is malformed
 * @retval  0   if the complete HTTP response has not yet received 
 * @retval  >0  if the complete HTTP reponse was received. The exact
 *              value represents the HTTP status code [100 -> 599].
 */
int mangoHelper_httpReponseVerify(char* response){
    char        strBuf[5];
    uint8_t     strBufIndex;
    char*       strPtr;
    uint32_t    statusCode;
    uint8_t     endCharFound;
    
    
    //printf("%s\r\n[%s]\r\n", __func__, response);
    
    /*
    * Check if the whole response was received
    */
	strPtr = strstr(response, "\r\n\r\n");
	if(strPtr == NULL){
		return 0;
	}
    
    /*
    * Remove the last \r\n
    * This is done so all our searches are performed only to headers
    */
    strPtr[2] = '\0';
    strPtr[3] = '\0';
    
    /*
    * Locate the status code
    */
    strPtr = strstr(response, " ");
	if(strPtr == NULL){
		return -1;
	}
    
    while((*strPtr) == ' '){strPtr++;}
    
    memset(strBuf, 0, sizeof(strBuf));
    
    endCharFound = 0;
    for(strBufIndex = 0; strBufIndex < sizeof(strBuf) - 1; strBufIndex++){
        if(( *strPtr >= '0') && (*strPtr <= '9')){
            strBuf[strBufIndex]     = *strPtr++;
        }else if( *strPtr == ' '){
            strBuf[strBufIndex] = '\0';
            endCharFound = 1;
            break;
        }else{
            return -1;
        }
    }
    
    if(!endCharFound){
        return -1;
    }
    
    if(mangoHelper_decstr2dec(strBuf, &statusCode)){
        return -1;
    }
    
    if( (statusCode < 100) || (statusCode >= 600) ){
        return -1;
    }
    
    return statusCode;
}

/**
 * @brief   Searches
 *          and that the format is correct. If the format
 *          is correct the HTTP status code is returned.
 *
 * @note    headerValueLen == 0 is permitted so we can check if
 *          a header exists without retrieving its value
 *
 * @retval  1   if header was found and copied to headerName, 
 * @retval  0   if header was found but headerValueLen was small
 * @retval < 0  if header was not found
 */
int mangoHelper_httpHeaderGet(char* response, char* headerName, char* headerValue, uint16_t headerValueLen){
    uint16_t    index;
    uint8_t     endCharFound;
    char*       strPtr;
    
    /*
    * Bypass the first reponse/request line and search for the
    * [CRLF]headerName pattern. This is done to avoid any
    * occurrences of "headerName" into  non-header locations 
    * (for example status lines or header values)
    */
    while(1){
        strPtr = strstr(response, "\r\n");
        if(strPtr == NULL){
            return -1;
        }
        
        strPtr += 2;
        response = strPtr;
        if(strncasecmp(strPtr, headerName, strlen(headerName)) == 0){
            /* Header found */
            break;
        }
    }
    
    strPtr  += strlen(headerName);
    
    while((*strPtr) == ' '){strPtr++;}
    
    if(*strPtr != ':'){
        return -1;
    }
    
    strPtr++;
    
    while((*strPtr) == ' '){strPtr++;}
    
    endCharFound = 0;
    for(index = 0; index < headerValueLen; index++){
        if( *strPtr != '\r'){
            headerValue[index] = *strPtr++;
			if(index == headerValueLen - 1){
				/* '\0' cannot fit into the provided buffer */
				break; 
			}
        }else{
            headerValue[index] = '\0';
            endCharFound = 1;
            break;
        }
    }

    if(!endCharFound){
        return 0;
    }

    /*
    * Remove spaces after header value
    */
    if(index){
        strPtr = &headerValue[index - 1];
        while( *strPtr == ' '){ *strPtr = '\0'; strPtr--;}
    }else{
        /* HTTP headers with empty value are acceptable */
    }
    
    return 1;
}

/**
 * @brief   Converts a DEC integer to HEX string
 * @return  0 on success, <0 on error
 */
void mangoHelper_dec2hexstr(uint32_t dec, char hexbuf[9]){
	uint8_t i;
	uint8_t k;
	char hex[9]; /* ffffffff */
	
	if(!dec){
		hexbuf[0] = '0';
		hexbuf[1] = '\0';
	}
	
	i = 7;
	hex[8] = '\0';
	hex[i--] = (dec & 0xF); dec >>= 4;
	hex[i--] = (dec & 0xF); dec >>= 4;
	hex[i--] = (dec & 0xF); dec >>= 4;
	hex[i--] = (dec & 0xF); dec >>= 4;
	hex[i--] = (dec & 0xF); dec >>= 4;
	hex[i--] = (dec & 0xF); dec >>= 4;
	hex[i--] = (dec & 0xF); dec >>= 4;
	hex[i--] = (dec & 0xF); dec >>= 4;
	
	i = 0;
	while(hex[i] == 0){i++;}
	
	k = 0;
	while(i < 8){hexbuf[k++] = (hex[i] <= 9) ? '0' + hex[i] : 'A' - 10 + hex[i]; i++;}
	
	hexbuf[k] = '\0';
}

/**
 * @brief   Converts a DEC integer to DEC string
 * @return  0 on success, <0 on error
 */
void mangoHelper_dec2decstr(uint32_t dec, char decbuf[9]){
	sprintf(decbuf,"%u", dec);
}

/**
 * @brief   Converts a HEX string to DEC integer
 * @return  0 on success, <0 on error
 */
int mangoHelper_hexstr2dec(char* hexstr, uint32_t* dec){
	uint8_t i;

	*dec = 0;
	
	if((hexstr[0] == '0') && ((hexstr[1] == 'x') || (hexstr[1] == 'X'))){ hexstr += 2;}
	
	i = 0;
	while(*hexstr){
		if(*hexstr > 47 && *hexstr < 58){ 
			/* 0 -> 9 */
			*dec += (*hexstr - 48);
		}else if(*hexstr > 64 && *hexstr < 71){ 
			/* A -> F */
			*dec += (*hexstr - 65 + 10);
		}else if(*hexstr > 96 && *hexstr < 103){
			/* a -> f */
			*dec += (*hexstr - 97 + 10);
		}else{
			break;
		}
		
		hexstr++;
		if((*hexstr > 47 && *hexstr < 58) || (*hexstr > 64 && *hexstr < 71) || (*hexstr > 96 && *hexstr < 103)){
			*dec <<= 4;
		}
		
		if(++i == 9) {
			/* Not a 32-bit integer */
			*dec = 0;
			return -1;
		}

	}
	
	if(!i) {
		/* No digits found */
		*dec = 0;
		return -1;
	}
	
	return 0;

}

/**
 * @brief   Converts a DEC string to DEC integer
 * @return  0 on success, < 0 on error
 */
int mangoHelper_decstr2dec(char* decstr, uint32_t* dec){
	uint8_t i;
	*dec = 0;
	
	i = 0;
	while(*decstr > 47 && *decstr < 58){
		/* 0 -> 9 */
		*dec += (*decstr - 48);
		
		decstr++;
		if(*decstr > 47 && *decstr < 58){
			*dec *= 10;
		}
		
		/* TODO: fix the check below */
		if(++i == 11) {
			/* Not a 32-bit integer */
			*dec = 0;
			return -1;
		}
	}

	if(!i) {
		/* No digits found */
		*dec = 0;
		return -1;
	}
	
    return 0;
}



uint32_t mangoHelper_elapsedTime(uint32_t starttime){
    uint32_t now;
    
    now = mangoPort_timeNow();
    
    return (now >= starttime) ? now - starttime : now + (0xffffffff - starttime);
}
