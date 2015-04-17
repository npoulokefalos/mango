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

#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

uint32_t mangoPort_timeNow(){
    struct timeval  tv;
	gettimeofday(&tv, NULL);

    return  (tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
}

void mangoPort_sleep(uint32_t ms){
    usleep(ms * 1000);
}

void* mangoPort_malloc(uint32_t sz){
    return malloc(sz);
}

void mangoPort_free(void* ptr){
    free(ptr);
}

/*
* < 0 connection error
* >=0 connection OK, data were received
*/
int mangoPort_read(int socketfd, uint8_t* data, uint16_t datalen, uint32_t timeout){
    uint32_t received;
    uint32_t start;
    int socketerror;
    //socklen_t socketerrorlen;
    int retval;
	
    MANGO_DBG(MANGO_DBG_LEVEL_PORT, ("Trying to read %u bytes\r\n", datalen) );
    
    received = 0;
    start = mangoPort_timeNow();
	while(received < datalen){
    
        retval = recv(socketfd, &data[received], datalen - received, 0);
        //printf("READ %d\r\n", retval);
        if(retval < 0){
            //socketerrorlen = sizeof(socketerror);
            //retval = getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &socketerror, (socklen_t *) &socketerrorlen);
            socketerror = errno;
            
            MANGO_DBG(MANGO_DBG_LEVEL_PORT, ("READ SOCKET ERROR %d\r\n", socketerror) );
            
            if(socketerror == EWOULDBLOCK || socketerror == EAGAIN){
                mangoPort_sleep(64);
            }else{
                return -1;
            }
        }else{
            received += retval;
            return received;
        }
        
        if(mangoHelper_elapsedTime(start) > timeout){
            return received;
        }
	}
	
	MANGO_DBG(MANGO_DBG_LEVEL_PORT, ("%u bytes read\r\n", retval) );
	
	return retval;
}

/*
* < 0 connection error
* >=0 connection OK, data were sent
*/
int mangoPort_write(int socketfd, uint8_t* data, uint16_t datalen, uint32_t timeout){
    uint32_t sent;
    uint32_t start;
    int socketerror;
    //socklen_t socketerrorlen;
    int retval;
    
	MANGO_DBG(MANGO_DBG_LEVEL_PORT, ("Trying to write %u bytes [%s]\r\n", datalen, data) );
	
    sent = 0;
    start = mangoPort_timeNow();
    while(sent < datalen){
        retval = write(socketfd, &data[sent], datalen - sent);
        if(retval < 0){
            //socketerrorlen = sizeof(socketerror);
            //retval = getsockopt(socketfd, SOL_SOCKET, SO_ERROR, &socketerror, (socklen_t *) &socketerrorlen);
            socketerror = errno;
            
            MANGO_DBG(MANGO_DBG_LEVEL_PORT, ("WRITE SOCKET ERROR %d!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\r\n", socketerror) );
            
            if(socketerror == EWOULDBLOCK || socketerror == EAGAIN){
                mangoPort_sleep(64);
            }else{
                return -1;
            }
        }else{
            sent += retval;
        }
        
        if(mangoHelper_elapsedTime(start) > timeout){
            return sent;
        }
    };
    
    return sent;
}

void mangoPort_disconnect(int socketfd){
	close(socketfd);
}

int mangoPort_connect(char* serverIP, uint16_t serverPort, uint32_t timeout){
    int retval;
    struct sockaddr_in s_addr_in;
    int socketfd;
    
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    
    memset(&s_addr_in, 0, sizeof(s_addr_in));
    s_addr_in.sin_family      = AF_INET;
    s_addr_in.sin_port        = htons (serverPort);
    s_addr_in.sin_addr.s_addr = inet_addr(serverIP);

    retval = connect(socketfd, (struct sockaddr *) &s_addr_in, sizeof(s_addr_in));

    if(retval == 0){
        retval = fcntl(socketfd, F_SETFL, O_NONBLOCK);
        if(retval == 0){
            
        }
        
        return socketfd;
    }else{
        close(socketfd);
        return -1;
    }
}


