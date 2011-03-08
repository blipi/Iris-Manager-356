/*
 * LibReadINI by D_Skywalk
 *
 * Copyright (c) 2006 David Colmenero Aka D_Skywalk
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 *
 *
**/

/*
** Internal global functions
*/

#ifndef LIBREADINI_GLOBAL_H
#define LIBREADINI_GLOBAL_H



/*
** local function prototypes
*/

int    openConfFile   (const char *filename, const int style);
void   closeConfFile  ();
int    parseConfFile  (int (*pfunc)(const char *,const char *));
void   setUserSection (const char * sec_name);
void   rewindConfFile ();
char  *getUserSection ();

typedef enum {
        FALSE = 0,
        TRUE  = 1
} bool;

#include "LibReadINI.h"


#endif

