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
* Define this if you want to test chunked-encoded resource
*/
//#define CHUNKED_GET


#ifdef CHUNKED_GET
    /*
    * This example demonstrates HTTP GET requests where the HTTP body 
    * is encoded with chunkes. When mango detects the chuned encoding
    * it will strip the chunk metadata providing to the application
    * only the actual data. This is done automatically so chunked GET
    * does not require any special configuration compared to normal GET.
    * 
    * The target URL is an image hosted in http://www.httpwatch.com
    * (http://www.httpwatch.com/httpgallery/chunked/)
    */
    #define SERVER_IP           "191.236.16.125"
    #define SERVER_HOSTNAME     "www.httpwatch.com"
    #define SERVER_PORT         80
    #define RESOURCE_URL        "/httpgallery/chunked/chunkedimage.aspx?0.020209475534940458" 
#else
    /*
    * This example demonstrates non-chunked HTTP GET requests.
    * 
    * The target URL is the home page of stackoverflow (http://stackoverflow.com/)
    */
    #define SERVER_IP           "198.252.206.140"
    #define SERVER_HOSTNAME     "stackoverflow.com"
    #define SERVER_PORT         80
    #define RESOURCE_URL        "/" 
#endif



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
#ifdef CHUNKED_GET
            /* Don't print binary data */
#else
			PRINTF("%s\r\n", mangoArgs->buf);
#endif
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


mangoErr_t httpGet(mangoHttpClient_t* httpClient){
    mangoErr_t err;

    /*
    * Select if a GET or HEAD request will be sent
    */
    err = mango_httpRequestNew(httpClient, RESOURCE_URL,  MANGO_HTTP_METHOD_GET);
    //err = mango_httpRequestNew(httpClient, RESOURCE_URL,  MANGO_HTTP_METHOD_HEAD);
    if(err != MANGO_OK){ return MANGO_ERR; }
    
	/*
	* If Basic Access Authentication is needed enable this..
	*/
	//err = mango_httpAuthSet(httpClient, MANGO_HTTP_AUTH__BASIC, "admin", "admin");
	if(err != MANGO_OK){ return MANGO_ERR; }
	
    /*
    * The "host: xxxx" header is required by almost all servers so we need
    * to add it.
    */
    err = mango_httpHeaderSet(httpClient, MANGO_HDR__HOST, SERVER_HOSTNAME);
    if(err != MANGO_OK){ return MANGO_ERR; }
    
    /*
    * Add this if it is desirable to continue with other HTTP requests after
    * the current one without closing the existing connection.
    */
    err = mango_httpHeaderSet(httpClient, MANGO_HDR__CONNECTION, "keep-alive");
    if(err != MANGO_OK){ return MANGO_ERR; }
    
    /*
    * Send the HTTP request and receive the response
    */
    err = mango_httpRequestProcess(httpClient, mangoApp_handler, NULL);
    if(err >= MANGO_ERR_HTTP_100 && err <= MANGO_ERR_HTTP_599){
		/*
		* A valid HTTP response was received..
		*/
        return err;
    }else{
        /*
		* Fatal request error
        */
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
    err = httpGet(httpClient);
    if(err >= MANGO_ERR_HTTP_100 && err <= MANGO_ERR_HTTP_599){ 
        /*
        * HTTP request recognized by the server, err stores the 
        * final HTTP response status code. It may be a 200 or
        * any other status code. 
        *
        * NOTE: At this point the status of the HTTP session is healthy
        * and we can continue sending other HTTP request without
        * closing the HTTP connection. It is possible to call httpGet()
        * again or to continue with new POST/PUT/GET/HEAD requests.
        */
        PRINTF("HTTP response code was %d\r\n", err);
    }
    else {
        /*
        * Fatal error during the GET request (working buffer was small or connection was closed). 
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
