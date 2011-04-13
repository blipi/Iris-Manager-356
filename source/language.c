/*
 * Language IrisManager by D_Skywalk
 *
 * Copyright (c) 2011 David Colmenero Aka D_Skywalk
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
    // VIDEO - ADJUST
    { "VIDEOADJUST_POSITION"    , "Use LEFT (-X) / RIGHT (+X) / UP (-Y) / DOWN (+Y) to adjust the screen" },
    { "VIDEOADJUST_SCALEINFO"   , "Video Scale X: %i Y: %i" },
    { "VIDEOADJUST_EXITINFO"    , "Press 'X' to exit" },
    { "VIDEOADJUST_DEFAULTS"    , "Press 'O' to default values" },

    //SELECT - GAME FOLDER
    { "GAMEFOLDER_WANTUSE"      , "Want to use" },
    { "GAMEFOLDER_TOINSTALLNTR" , "to install the game?" },
    { "GAMEFOLDER_USING"        , "Using" },
    { "GAMEFOLDER_TOINSTALL"    , "to install the game" },
    
    //DRAW SCREEN1
    { "DRAWSCREEN_FAVSWAP"      , "Favourites Swap" },
    { "DRAWSCREEN_FAVINSERT"    , "Favourites Insert" },
    { "DRAWSCREEN_FAVORITES"    , "Favourites" },
    { "DRAWSCREEN_PAGE"         , "Page" },
    { "DRAWSCREEN_GAMES"        , "Games" },
    { "DRAWSCREEN_PLAY"         , "Play" },
    { "DRAWSCREEN_SOPTIONS"     , "SELECT: Game Options" },
    { "DRAWSCREEN_SDELETE"      , "SELECT: Delete Favourite" },
    { "DRAWSCREEN_STGLOPT"      , "START: Global Options" },
    { "DRAWSCREEN_EXITXMB"      , "Exit to XMB?" },
    { "DRAWSCREEN_CANRUNFAV"    , "Cannot run this favourite" },
    { "DRAWSCREEN_MARKNOTEXEC"  , "Marked as non executable. Trying to install in HDD0 cache" },
    { "DRAWSCREEN_MARKNOTEX4G"  ,"Marked as not executable - Contains splited files >= 4GB" },
    { "DRAWSCREEN_GAMEINOFMNT"  , "I cannot find one folder to mount /dev_hdd0/game from USB" },
    { "DRAWSCREEN_GAMEIASKDIR"  , "Want to create in /dev_usb00" },
    { "DRAWSCREEN_GAMEICANTFD"  , "I cannot find an USB device to mount /dev_hdd0/game from USB" },
    { "DRAWSCREEN_GAMEIWLAUNCH" , "Want to launch the Game?" },
    { "DRAWSCREEN_EXTEXENOTFND" , "external executable not found" },
    { "DRAWSCREEN_EXTEXENOTCPY" , "Use 'Copy EBOOT.BIN from USB' to import them." },
    { "DRAWSCREEN_REQBR"        , "Required BR-Disc, Retry?" },

    //DRAW OPTIONS
    { "DRAWGMOPT_OPTS"          , "Options" },
    { "DRAWGMOPT_CFGGAME"       , "Config. Game" },
    { "DRAWGMOPT_CPYGAME"       , "Copy Game" },
    { "DRAWGMOPT_DELGAME"       , "Delete Game" },
    { "DRAWGMOPT_FIXGAME"       , "Fix File Permissions" },
    { "DRAWGMOPT_TSTGAME"       , "Test Game" },
    { "DRAWGMOPT_CPYEBOOTGAME"  , "Copy EBOOT.BIN from USB" },
    { "DRAWGMOPT_CPYTOFAV"      , "Copy to Favourites" },
    { "DRAWGMOPT_DELFMFAV"      , "Delete from Favourites" },

    { "DRAWGMOPT_FIXCOMPLETE"   , "Fix Permissions Done!" },
    { "DRAWGMOPT_CPYOK"         , "copied successfully" },
    { "DRAWGMOPT_CPYERR"        , "Error copying" },
    { "DRAWGMOPT_CPYNOTFND"     , "not found" },

    //DRAW CONFIGS
    { "DRAWGMCFG_CFGS"          , "Config. Game" },
    { "DRAWGMCFG_DSK"           , "Requires Disc" },
    { "DRAWGMCFG_NO"            , "No" },
    { "DRAWGMCFG_YES"           , "Yes" },
    { "DRAWGMCFG_UPD"           , "Online Updates" },
    { "DRAWGMCFG_ON"            , "On" },
    { "DRAWGMCFG_OFF"           , "Off" },
    { "DRAWGMCFG_EXTBOOT"       , "Extern EBOOT.BIN" },
    { "DRAWGMCFG_BDEMU"         , "BD Emu (for USB)" },
    { "DRAWGMCFG_EXTHDD0GAME"   , "Ext /dev_hdd0/game" },
    { "DRAWGMCFG_SAVECFG"       , "Save Config" },

    //DRAW GLOBAL OPTIONS
    { "DRAWGLOPT_OPTS"          , "Global Options" },
    { "DRAWGLOPT_SCRADJUST"     , "Video Adjust" },
    { "DRAWGLOPT_CHANGEDIR"     , "Change Game Directory" },
    { "DRAWGLOPT_CHANGEBCK"     , "Change Background Color" },
    { "DRAWGLOPT_SWMUSICOFF"    , "Switch Music Off" },
    { "DRAWGLOPT_SWMUSICON"     , "Switch Music On" },
    { "DRAWGLOPT_INITFTP"       , "Initialize FTP server" },
    { "DRAWGLOPT_TOOLS"         , "Tools" },
    { "DRAWGLOPT_CREDITS"       , "Credits" },
    { "DRAWGLOPT_FTPINITED"     , "Server FTP initialized\nDo you want auto-enable FTP service on init?" },
    { "DRAWGLOPT_FTPARINITED"   , "Server FTP already initialized" },
    { "DRAWGLOPT_FTPSTOPED"     , "Server FTP Stoped\nRemoved FTP service on init." },

    //DRAW TOOLS
    { "DRAWTOOLS_TOOLS"         , "Tools" },
    { "DRAWTOOLS_DELCACHE"      , "Delete Cache Tool" },
    { "DRAWTOOLS_SECDISABLE"    , "Press To Disable Syscall Security" },
    { "DRAWTOOLS_SECENABLE"     , "Press To Enable Syscall Security" },
    { "DRAWTOOLS_LOADX"         , "Load PS3LoadX" },

    //GLOBAL
    { "GLOBAL_RETURN"           , "Return" },
    { "GLOBAL_SAVED"            , "File Saved" },

};

char * language[LANGSTRINGS_COUNT];

int open_language (char * filename) 
{

    int n, version;
    struct stat s;

    if(stat(filename, &s)<0)
    {
        //write it
        if(SaveFile(filename, (char *) &language_ini_bin, sizeof(language_ini_bin)) == 0)
        {
            if(stat(filename, &s)<0)
                return -1; //why?!
        }
        else
            return -2; //no perms?
    }

    version = getConfigValueInt(filename, "Language", "VERSION", -1);
    if( version < LANGFILE_VERSION )
    {
        //malformed lang file?
        return -3;
    }
    
    for (n = 0; n < LANGSTRINGS_COUNT; n++)
    {
        language[n] = (char*) malloc(1024);
        getConfigValueString(filename, "Language", lang_strings[n].strname, language[n], MAX_CFGLINE_LEN-1, "NOP!"); //lang_strings[n].strdefault);
    }

    return version;
}

void close_language(void)
{
    int n;
    //free memory
    for (n = 0; n < LANGSTRINGS_COUNT; n++)
        free(language[n]);
}

