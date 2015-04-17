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

#ifndef __MANGO_DEBUG_H__
#define __MANGO_DEBUG_H__ 

#define MANGO_DBG_ON                (0x01)
#define MANGO_DBG_OFF               (0x00)

/*
*   Select logging mask
*/
#define MANGO_DBG_LEVEL_SM          MANGO_DBG_ON
#define MANGO_DBG_LEVEL_DP          MANGO_DBG_ON
#define MANGO_DBG_LEVEL_APP         MANGO_DBG_ON
#define MANGO_DBG_LEVEL_WS          MANGO_DBG_ON
#define MANGO_DBG_LEVEL_PORT        MANGO_DBG_OFF


#define MANGO_DBG(level, msg)       if(level & MANGO_DBG_ON){ MANGO_PRINTF(msg); };
#define MANGO_ENSURE(cond, msg)     if(!(cond)){MANGO_PRINTF( ("[MANGO ENSURE] %s(), line %d\r\n", __func__, __LINE__) ); MANGO_PRINTF(msg); while(1){};}

#endif
