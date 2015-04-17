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

#include "../mango.h"

int mangoCrypto_base64Encode(const void* data_buf, size_t dataLength, char* result, size_t resultSize)
{
	const char base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	const uint8_t *data = (const uint8_t *)data_buf;
	size_t resultIndex = 0;
	size_t x;
	uint32_t n = 0;
	int padCount = dataLength % 3;
	uint8_t n0, n1, n2, n3;
	
	/* increment over the length of the string, three characters at a time */
	for (x = 0; x < dataLength; x += 3) 
	{
		/* these three 8-bit (ASCII) characters become one 24-bit number */
		n = ((uint32_t)data[x]) << 16; //parenthesis needed, compiler depending on flags can do the shifting before conversion to uint32_t, resulting to 0
		
		if((x+1) < dataLength)
			n += ((uint32_t)data[x+1]) << 8;//parenthesis needed, compiler depending on flags can do the shifting before conversion to uint32_t, resulting to 0
		
		if((x+2) < dataLength)
			n += data[x+2];
		
		/* this 24-bit number gets separated into four 6-bit numbers */
		n0 = (uint8_t)(n >> 18) & 63;
		n1 = (uint8_t)(n >> 12) & 63;
		n2 = (uint8_t)(n >> 6) & 63;
		n3 = (uint8_t)n & 63;
		
		/*
		 * if we have one byte available, then its encoding is spread
		 * out over two characters
		 */
		if(resultIndex >= resultSize) return -1;   /* indicate failure: buffer too small */
		result[resultIndex++] = base64chars[n0];
		if(resultIndex >= resultSize) return -1;   /* indicate failure: buffer too small */
		result[resultIndex++] = base64chars[n1];
		
		/*
		 * if we have only two bytes available, then their encoding is
		 * spread out over three chars
		 */
		if((x+1) < dataLength)
		{
			if(resultIndex >= resultSize) return -1;   /* indicate failure: buffer too small */
			result[resultIndex++] = base64chars[n2];
		}
		
		/*
		 * if we have all three bytes available, then their encoding is spread
		 * out over four characters
		 */
		if((x+2) < dataLength)
		{
			if(resultIndex >= resultSize) return -1;   /* indicate failure: buffer too small */
			result[resultIndex++] = base64chars[n3];
		}
	}  
	
	/*
	 * create and add padding that is required if we did not have a multiple of 3
	 * number of characters available
	 */
	if (padCount > 0) 
	{ 
		for (; padCount < 3; padCount++) 
		{ 
			if(resultIndex >= resultSize) return -1;   /* indicate failure: buffer too small */
			result[resultIndex++] = '=';
		} 
	}
	if(resultIndex >= resultSize) return -1;   /* indicate failure: buffer too small */
	
	result[resultIndex] = 0;
	return resultIndex;   /* indicate success */
}
