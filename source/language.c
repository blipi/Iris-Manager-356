/*
 * Language HManager by D_Skywalk
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *   Simple code for play with languages
 *    for ps3 scene ;)
 *
**/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "LibReadINI-global.h"
#include "LibReadINI.h"
#include "language.h"
#include "utils.h"
#include "language_ini.bin.h"

#define LANGFILE_VERSION 1

typedef struct lngstr
{
  char * strname;  
  char * strdefault; 
} t_lngstr;

t_lngstr lang_strings[] = 
{
    //VIDEO - ADJUST
    { "VIDEOADJUST_POSITION",       "Use LEFT (-X) / RIGHT (+X) / UP (-Y) / DOWN (+Y) to adjust the screen" },
    { "VIDEOADJUST_SCALEINFO",      "Video Scale X: %i Y: %i" },
    { "VIDEOADJUST_EXITINFO",       "Press 'X' to exit" },
    { "VIDEOADJUST_DEFAULTS",       "Press 'O' to default values" },
    { "VIDEOADJUST_SAVED",          "File Saved" },
    //SELECT - GAME FOLDER
    { "GAMEFOLDER_WANTUSE",         "Want to use" },
    { "GAMEFOLDER_TOINSTALLNTR",    "to install the game?" },
    { "GAMEFOLDER_USING",           "Using" },
    { "GAMEFOLDER_TOINSTALL",       "to install the game" },
    //MAIN
    { "MAIN_PAYLOADINVALID",        "Invalid payload or payload locked" },
    { "MAIN_PAYLOADINVALIDMAP",     "Error Loading Payload: map failed" },
    { "MAIN_PAYLOADOLD",            "Payload Resident Is Old" },
    { "MAIN_PAYLOADRESIDENT",       "Payload Is Resident" },
    //DRAW SCREEN1
    { "DRAWSCREEN_FAVSWAP",         "Favourites Swap" },
    { "DRAWSCREEN_FAVINSERT",       "Favourites Insert" },
    { "DRAWSCREEN_FAVORITES",       "Favourites" },
    { "DRAWSCREEN_PAGE",            "Page" },
    { "DRAWSCREEN_PRESS",           "Press" },
    { "DRAWSCREEN_FOPTIONS",        "for Options" },
    { "DRAWSCREEN_FDELETE",         "for Delete" },
    { "DRAWSCREEN_PSTARTG",         "Press START for Global Options" },
    { "DRAWSCREEN_EXITXMB",         "Exit to XMB?" },
    { "DRAWSCREEN_CANRUNFAV",       "Cannot run this favourite" },
    { "DRAWSCREEN_MARKNOTEXEC",     "Marked as not executable" },
    { "DRAWSCREEN_REQBR",           "Required BR-Disc" },
    { "DRAWSCREEN_EXTEXENOTFND",    "external executable not found" },


};


char * language[LANGSTRINGS_COUNT];

int open_language (char * filename) 
{

    int n;

    if(!cfgOpen(filename, MS_STYLE))
    {
        //write it
        if(SaveFile(filename, (char *) &language_ini_bin, sizeof(language_ini_bin)) == 0)
        {
            if(!cfgOpen(filename, MS_STYLE))
                return -1; //why?!
        }
        else
            return -2; //no perms?
    }


    if(cfgSelectSection("Language") != 1) /* ok if returns 1 */
    {
        //malformed lang file?
        return -3;
    }
    
    for (n = 0; n < LANGSTRINGS_COUNT; n++)
    	language[n] = cfgReadText(lang_strings[n].strname, lang_strings[n].strdefault);

    //close file
    cfgClose();

    return 1;
}

void close_language(void)
{
    //free memory
    cfgFree();
}

