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

#define PRINTF              printf




/*
* This example demonstrates HTTP POST, chunked POST and POST requests with status code expectation.
* PUT requests are similar.
* 
* The target URL is Henry's HTTP Post Dumping Server (HTTP test endpoint)
* (http://www.posttestserver.com/)
*/

#define SERVER_IP           "67.205.27.203"
#define SERVER_HOSTNAME     "posttestserver.com"
#define SERVER_PORT         80

/*
* Defines the size of the virtual file to be posted
*/
#define FILE_SZ             (15 * 1024)

/*
* Define this to do a chunked POST
*/
#define DO_CHUNKED_POST

/*
* Define this to add an "Expect" header to the request
*/
#define USE_EXPECT_100_CONTINUE



mangoErr_t mangoApp_handler(mangoArg_t* mangoArgs, void* userArgs){
	mangoErr_t err;
    
	switch(mangoArgs->argType){
		case MANGO_ARG_TYPE_HTTP_REQUEST_READY:
        {
            /*
            * This is the HTTP request that mango is going to send to the Server.
            */
            PRINTF("\r\n");
			PRINTF("-----------------------------------------------------------------\r\n");
			PRINTF("HTTP REQUEST:\r\n");
			PRINTF("-----------------------------------------------------------------\r\n");
			PRINTF("%s", mangoArgs->buf);
			PRINTF("-----------------------------------------------------------------\r\n");
            break;
        }
        case MANGO_ARG_TYPE_HTTP_RESP_RECEIVED:
        {
            /*
            * HTTP response headers received.
            */
            PRINTF("\r\n");
			PRINTF("-----------------------------------------------------------------\r\n");
			PRINTF("HTTP RESPONSE [status code %u]:\r\n", mangoArgs->statusCode);
			PRINTF("-----------------------------------------------------------------\r\n");
			PRINTF("%s", mangoArgs->buf);
			PRINTF("-----------------------------------------------------------------\r\n");
            
#if 0
            /*
            * If a header value is needed it can be extracted as follows:
            */
            char headerValue[64];
            err = mango_httpHeaderGet((char*) mangoArgs->buf, MANGO_HDR__CONTENT_LENGTH, headerValue, sizeof(headerValue));
            if(err == MANGO_OK){
                PRINTF("The value of the specified header is '%s'\r\n", headerValue);
            }else{
                PRINTF("Header not found or temporary buffer was small!\r\n");
            }
#endif
            
            break;
        }
        case MANGO_ARG_TYPE_HTTP_DATA_RECEIVED:
        {
            /*
            * Data were received from the Server (GET request)
            */
            PRINTF("\r\n");
			PRINTF("-----------------------------------------------------------------\r\n");
			PRINTF("HTTP DATA RECEIVED: [%u bytes]\r\n", mangoArgs->buflen);
			PRINTF("-----------------------------------------------------------------\r\n");
			PRINTF("%s\r\n", mangoArgs->buf);
			PRINTF("-----------------------------------------------------------------\r\n");
			
            break;
        }
        case MANGO_ARG_TYPE_WEBSOCKET_DATA_RECEIVED:
        {
            /*
            * Data were received from the websocket
            */
            PRINTF("\r\n");
			PRINTF("-----------------------------------------------------------------\r\n");
			PRINTF("WEBSOCKET DATA RECEIVED: [%u bytes, Frame ID %u]\r\n", mangoArgs->buflen, mangoArgs->frameID);
			PRINTF("-----------------------------------------------------------------\r\n");
			PRINTF("%s\r\n", mangoArgs->buf);
			PRINTF("-----------------------------------------------------------------\r\n");
			
			break;
        }
        case MANGO_ARG_TYPE_WEBSOCKET_CLOSE:
        {
			/*
            * Server requested to close the websocket, a closed frame was sent automatically
			* in response. Poll will return with an error.
            */
            PRINTF("\r\n");
			PRINTF("-----------------------------------------------------------------\r\n");
			PRINTF("WEBSOCKET CLOSE FRAME RECEIVED!\r\n");
			PRINTF("-----------------------------------------------------------------\r\n");
            break;
        }
        case MANGO_ARG_TYPE_WEBSOCKET_PING:
        {
			/*
            * Server sent a Ping frame and a pong frame was sent automatically.
            */
            PRINTF("\r\n");
			PRINTF("-----------------------------------------------------------------\r\n");
			PRINTF("WEBSOCKET PING FRAME RECEIVED!\r\n");
			PRINTF("-----------------------------------------------------------------\r\n");
            break;
        }
	};
	
    return MANGO_OK;
};


mangoErr_t httpPost(mangoHttpClient_t* httpClient){
    mangoErr_t err;
    static uint8_t buffer[128];
    char fileSz[11];
    uint32_t sent, sendnow;
    

    err = mango_httpRequestNew(httpClient, "/post.php",  MANGO_HTTP_METHOD_POST);
    if(err != MANGO_OK){ return MANGO_ERR; }
    
    /*
    * The "host: xxxx" header is required by almost all servers so we should add it
    */
    err = mango_httpHeaderSet(httpClient, MANGO_HDR__HOST, SERVER_HOSTNAME);
    if(err != MANGO_OK){ return MANGO_ERR; }
    
    /*
    * Add this if it is desirable to continue with other HTTP requests after
    * the current one without closing the existing connection.
    */
    err = mango_httpHeaderSet(httpClient, MANGO_HDR__CONNECTION, "keep-alive");
    if(err != MANGO_OK){ return MANGO_ERR; }
    
#ifdef DO_CHUNKED_POST
    /*
    * The "Transfer-Encoding: chunked" HTTP header will notify mango that this is 
    * a Chunked HTTP POST. The actual size of the POST is not yet known. When the 
    * whole file has been sent a final mango_httpHeaderSet() call with
    * buffer NULL and buflen 0 should be made. This will notify the internal State machine
    * that all data have been sent and the application is ready to accept the HTTP response.
    */
    err = mango_httpHeaderSet(httpClient, MANGO_HDR__TRANSFER_ENCODING, "chunked");
    if(err != MANGO_OK){ return MANGO_ERR; }
#else
    /*
    * The "Content-Length: xxxx" HTTP header will notify mango that this is 
    * a normal (Non-Chunked) HTTP POST. This header specifies actual size of the POST.  
    * When the whole file has been sent a final mango_httpHeaderSet() call with
    * buffer NULL and buflen 0 should be made. This will notify the internal State machine
    * that all data have been sent and the application is ready to accept the HTTP response.
    */
    mangoHelper_dec2decstr(FILE_SZ, &fileSz);
    err = mango_httpHeaderSet(httpClient, MANGO_HDR__CONTENT_LENGTH, fileSz);
    if(err != MANGO_OK){ return MANGO_ERR; }
#endif
    
#ifdef USE_EXPECT_100_CONTINUE
    /*
    * To verify that the server is going to accept the POST data without
    * first sending them, use the "Expect: 100-Continue" header. If
    * the server is not willing to accept the POST/PUT request, it will reply
    * with a 417 Epectation failed status code. This may happen if for example
    * the HTTP headers/URL are not correct. If the HTTP request headers are corrent
    * a "100 Continue" HTTP response will be sent from the server and the application 
    * can start sending the actual POST data.
    *
    * Note: The "Expect" header may not be supported from the target server, so
    * if any strange behaviour is observed it should be disabled.
    */
    err = mango_httpHeaderSet(httpClient, MANGO_HDR__EXPECT, "100-Continue");
    if(err != MANGO_OK){ return MANGO_ERR; }
#endif

    /*
    * Send the HTTP request
    */
    err = mango_httpRequestProcess(httpClient, mangoApp_handler, NULL);
    if(err >= MANGO_ERR_HTTP_100 && err <= MANGO_ERR_HTTP_599){
        if(err == MANGO_ERR_HTTP_100){
            /*
            * Server is ready to accept the HTTP body, send it
            */
            sent = 0;
            while(sent != FILE_SZ){
                sendnow = FILE_SZ - sent > sizeof(buffer) ? sizeof(buffer) : FILE_SZ - sent;

                err = mango_httpDataSend(httpClient, buffer, sendnow);
                if(err != MANGO_OK){ return MANGO_ERR; } /* If we sent more data than specified in the "Content-Length" field.. */
                
                sent += sendnow;
                PRINTF("POSTED %u/%u bytes..\r\n",sent, FILE_SZ);
                //mangoPort_sleep(500);
            };
            
            /*
            * Notify that all POST data have been sent
            */
            err = mango_httpDataSend(httpClient, NULL, 0);
            if(err >= MANGO_ERR_HTTP_100 && err <= MANGO_ERR_HTTP_599){
                /*
                * HTTP POST finished, err stores the actual HTTP status code
                */
                return err; 
            }else{ 
                /* If we sent less data than specified in the  "Content-Length" field.. */
                return MANGO_ERR; 
            } 

        }else{
            /*
            * We got a non-success status code from the server before executing the
            * actual POST. For example server may responded with a "417 Expectation Failed" if
            * the request headers or target URL were invalid.
            */
            return err;
        }
    }
    
    return MANGO_ERR;
}



int main(){
    mangoHttpClient_t* httpClient;
    mangoErr_t err;
    
    /*
    * Connect to server
    */
    httpClient = mango_connect(SERVER_IP, SERVER_PORT);
    if(!httpClient){
        PRINTF("mangoHttpClient_connect() FAILED!");
        return MANGO_ERR;
    }
    
    /*
    * Do the HTTP POST 
    */
    err = httpPost(httpClient);
    if(err >= MANGO_ERR_HTTP_100 && err <= MANGO_ERR_HTTP_599){ 
        /*
        * HTTP request recognized by the server, err stores the 
        * final HTTP response status code. It may be a 200 or
        * any other status code. 
        * 
        * NOTE: At this point the status of the HTTP session is healthy
        * and we can continue sending other HTTP request without closing 
        * the HTTP connection (persistent connection). It is possible to call 
        * httpPost() again or  continue with a new POST/PUT/GET/HEAD request.
        */
        PRINTF("HTTP response code was %d\r\n", err);
    }
    else {
        /*
        * Fatal error during the POST operation (application sent more or less data than 
        * needed, or a socket Tx failed due to socket disconnection, or the 
        * working buffer was small..). At this point mango_disconnect() should be called.
        */
        PRINTF("HTTP request failed!\r\n");
    }
    
    /*
    * Disconnect from server
    */
    mango_disconnect(httpClient);
    
    return 0;
}
