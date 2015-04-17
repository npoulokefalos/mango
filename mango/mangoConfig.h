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

#ifndef __MANGO_CONFIG_H__
#define __MANGO_CONFIG_H__

/*
* Select OS printf function.
* To enable/disable debug messages check "mangoDebug.h" header file
*/
#define MANGO_PRINTF(msg)           do {printf msg;} while(0)


/*
* Defines the size of the internal working buffer. This needs to be at least
* the maximum size of the HTTP request or HTTP reponse that the application
* is able to handle. For example if the maximum size of any HTTP request
* issued by the application is ~300 bytes and the maximum size of any
* recieved HTTP response is ~400 bytes, set this value to 400. With around
* 768 Bytes - 1Kb you will able to process almost any HTTP request. 
*
* It also defines the size of the data read/written through sockets so
* if you are expecting to achieve high throuputs set this to the
* approprietary value.
*
* If it is set too low and a request is built or a reponse is received
* that needs a bigger WB size to be handled, mango is going to exit with
* an error code MANGO_ERR_WORKBUFSMALL.
*/
#define MANGO_WORKING_BUFFER_SZ     		(768)

/*
* Defines the maximum period of time (in miliseconds) that mango is going to wait 
* until the HTTP response of the executed HTTP request is received.
* 
* During HTTP GET operations, it also defines that if no TCP packets have been
* received for the specified time period, the connection should be considered
* closed and the server unreachable.
*/
#define MANGO_HTTP_RESPONSE_TIMEOUT_MS      (10000)

/*
* Defines the maximum period of time that mango is going to wait until the
* connection with the remote server has been established. If the timeout
* is exceeded the server is considered unreachable.
*/
#define MANGO_SOCKET_CONNECT_TIMEOUT_MS     (5000)

/*
* Defines the maximum timeout (in milliseconds) of any TCP write operation.
* If the timeout is exceeded and no (or only some) bytes of the packet
* were sent, the connection will be considered closed and mango will move
* to the disconnected/aborted state. 
*/
#define MANGO_SOCKET_WRITE_TIMEOUT_MS       (10000)


/*
* Define the OS enviroment
*/
#define MANGO_OS_ENV__UNIX

/*
* Define the IP enviroment
*/
#define MANGO_IP_ENV__UNIX
#define MANGO_IP_ENV__LWIP

#endif
