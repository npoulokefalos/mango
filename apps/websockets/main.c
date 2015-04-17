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
* This example demonstrates Websockets.
* 
* The target URL is websocket.org (Websocket test endpoint, echoes back whatever we send)
* (http://www.posttestserver.com/)
*/

#define SERVER_IP           "174.129.224.73"
#define SERVER_HOSTNAME     "echo.websocket.org"
#define SERVER_PORT         80



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
            * Server sent us a Ping frame and a pong frame was sent automatically.
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


mangoErr_t testWebsocket(mangoHttpClient_t* httpClient){
    mangoErr_t err;
    char* msg;
 
    /*
	* Specify HTTP request's basic headers
	*/
	err = mango_httpRequestNew(httpClient, "/",  MANGO_HTTP_METHOD_GET);
	if(err != MANGO_OK){ return MANGO_ERR; }
	
	/*
	* If Basic Access Authentication is needed enable this..
	*/
	//err = mango_httpAuthSet(httpClient, MANGO_HTTP_AUTH__BASIC, "admin", "admin");
	if(err != MANGO_OK){ return MANGO_ERR; }
	
	err = mango_httpHeaderSet(httpClient, MANGO_HDR__HOST, SERVER_HOSTNAME);
	if(err != MANGO_OK){ return MANGO_ERR; }	

	err = mango_httpHeaderSet(httpClient, MANGO_HDR__ORIGIN, "http://www.websocket.org");
	if(err != MANGO_OK){ return MANGO_ERR; }
	
	err = mango_httpHeaderSet(httpClient, MANGO_HDR__WEB_SOCKET_VERSION, "13");
	if(err != MANGO_OK){ return MANGO_ERR; }
	
	err = mango_httpHeaderSet(httpClient, MANGO_HDR__CONNECTION, "Keep-alive, Upgrade");
	if(err != MANGO_OK){ return MANGO_ERR; }
	
	err = mango_httpHeaderSet(httpClient, MANGO_HDR__WEB_SOCKET_PROTOCOL, "wamp");
	if(err != MANGO_OK){ return MANGO_ERR; }	
	
	err = mango_httpHeaderSet(httpClient, MANGO_HDR__WEB_SOCKET_KEY, "v6H3B7uxxRf1NfPeeaDHiQ==");
	if(err != MANGO_OK){ return MANGO_ERR; }
	
	err = mango_httpHeaderSet(httpClient, MANGO_HDR__UPGRADE, "websocket");
	if(err != MANGO_OK){ return MANGO_ERR; }
	

	/*
	* Send HTTP request and receive the response
	*/
	err = mango_httpRequestProcess(httpClient, mangoApp_handler, NULL);
	if(err >= MANGO_ERR_HTTP_100 && err <= MANGO_ERR_HTTP_599){
		if(err != MANGO_ERR_HTTP_101){
			/*
			* HTTP upgrade to websockets failed, possibly due to wrong headers
            * or because the server does not support them
			*/
			return err;
		}else{
			/*
			* HTTP upgrade to websockets succeed. Enter an infinite loop
			* waiting for new frames and sending new ones.
			*/
			while(1){
				/*
				* Block for the specified amount of time and Poll for received data 
                * or control (Ping, Close) frames.
				*/
                PRINTF("Polling..\r\n");
				err = mango_wsPoll(httpClient, 2000);
				if(err != MANGO_OK){ return MANGO_ERR; } /* Connection closed by remote peer / socket disconnected / we got an invalid frame.. */
				
				/*
				* Send some frames to the server. They should be echoed back to us through the 
                * mangoApp_handler() callback.
				*/
                msg = "Hello from mango!";
				err = mango_wsFrameSend(httpClient, (uint8_t*) msg, strlen(msg), MANGO_WS_FRAME_TYPE_TEXT);
				if(err != MANGO_OK){ return MANGO_ERR; } /* Connection error, abort */
                
                msg = "This is a test message";
				err = mango_wsFrameSend(httpClient, (uint8_t*) msg, strlen(msg), MANGO_WS_FRAME_TYPE_TEXT);
				if(err != MANGO_OK){ return MANGO_ERR; } /* Connection error, abort */
			}
			
			/*
			* When we finish close the connection
			*/
			mango_wsClose(httpClient);
			
			return MANGO_OK;
		}
	}else{
		/*
		* Fatal request error (Connection closed or working buffer was small
		* and we couldn't to process the HTPP request/response.
		*/
		return MANGO_ERR;
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
    * Do the HTTP GET 
    */
    err = testWebsocket(httpClient);
    if(err >= MANGO_ERR_HTTP_100 && err <= MANGO_ERR_HTTP_599){

        PRINTF("HTTP response code was %d\r\n", err);
    }
    else {
        /*
        * Fatal error during the GET request (working buffer was small / connection was closed). 
        * At this point mango_disconnect() should be called.
        */
        PRINTF("HTTP request failed!\r\n");
    }
    
    /*
    * Disconnect from server
    */
    mango_disconnect(httpClient);
    
    return 0;
}
