#ifndef LANGUAGE_H
#define LANGUAGE_H

/* REMEMBER INCLUDE YOUR NEW STRIGNS
   IN language.c - lang_strings */

enum lang_codes 
{
    //VIDEO - ADJUST
    VIDEOADJUST_POSITION,
    VIDEOADJUST_SCALEINFO,
    VIDEOADJUST_EXITINFO,
    VIDEOADJUST_DEFAULTS,

    // SELECT - GAME FOLDER
    GAMEFOLDER_WANTUSE,
    GAMEFOLDER_TOINSTALLNTR,
    GAMEFOLDER_USING,
    GAMEFOLDER_TOINSTALL,
    
    //DRAW SCREEN1
    DRAWSCREEN_FAVSWAP,
    DRAWSCREEN_FAVINSERT,
    DRAWSCREEN_FAVORITES,
    DRAWSCREEN_PAGE,
    DRAWSCREEN_GAMES,
    DRAWSCREEN_PLAY,
    DRAWSCREEN_SOPTIONS,
    DRAWSCREEN_SDELETE,
    DRAWSCREEN_STGLOPT,
    DRAWSCREEN_EXITXMB,
    DRAWSCREEN_CANRUNFAV,
    DRAWSCREEN_MARKNOTEXEC,
    DRAWSCREEN_MARKNOTEX4G,
    DRAWSCREEN_GAMEINOFMNT,
    DRAWSCREEN_GAMEIASKDIR,
    DRAWSCREEN_GAMEICANTFD,
    DRAWSCREEN_GAMEIWLAUNCH,
    DRAWSCREEN_EXTEXENOTFND,
    DRAWSCREEN_EXTEXENOTCPY,
    DRAWSCREEN_REQBR,

    //DRAW OPTIONS
    DRAWGMOPT_OPTS,
    DRAWGMOPT_CFGGAME,
    DRAWGMOPT_CPYGAME,
    DRAWGMOPT_DELGAME,
    DRAWGMOPT_FIXGAME,
    DRAWGMOPT_TSTGAME,
    DRAWGMOPT_CPYEBOOTGAME,
    DRAWGMOPT_CPYTOFAV,
    DRAWGMOPT_DELFMFAV,

    DRAWGMOPT_FIXCOMPLETE,
    DRAWGMOPT_CPYOK,
    DRAWGMOPT_CPYERR,
    DRAWGMOPT_CPYNOTFND,

    DRAWGMCFG_CFGS,
    DRAWGMCFG_DSK,
    DRAWGMCFG_NO,
    DRAWGMCFG_YES,
    DRAWGMCFG_UPD,
    DRAWGMCFG_ON,
    DRAWGMCFG_OFF,
    DRAWGMCFG_EXTBOOT,
    DRAWGMCFG_BDEMU,
    DRAWGMCFG_EXTHDD0GAME,
    DRAWGMCFG_SAVECFG,

    DRAWGLOPT_OPTS,
    DRAWGLOPT_SCRADJUST,
    DRAWGLOPT_CHANGEDIR,
    DRAWGLOPT_CHANGEBCK,
    DRAWGLOPT_SWMUSICOFF,
    DRAWGLOPT_SWMUSICON,
    DRAWGLOPT_INITFTP,
    DRAWGLOPT_TOOLS,
    DRAWGLOPT_CREDITS,
    DRAWGLOPT_FTPINITED,
    DRAWGLOPT_FTPARINITED,
    DRAWGLOPT_FTPSTOPED,

    //DRAW TOOLS
    DRAWTOOLS_TOOLS,
    DRAWTOOLS_DELCACHE,
    DRAWTOOLS_SECDISABLE,
    DRAWTOOLS_SECENABLE,
    DRAWTOOLS_LOADX,

    //MAIN - OTHERS
    DRAWCACHE_CACHE,
    DRAWCACHE_ERRNEEDIT,
    DRAWCACHE_ASKTODEL,
    PATCHBEMU_ERRNOUSB,
    MOVEOBEMU_ERRSAVE,
    MOVEOBEMU_ERRMOVE,
    MOVEOBEMU_MOUNTOK,
    MOVETBEMU_ERRMOVE,


    //UTILS
    //FAST COPY ADD
    FASTCPADD_FAILED,
    FASTCPADD_ERRTMFILES,
    FASTCPADD_FAILEDSTAT,
    FASTCPADD_ERROPEN,
    FASTCPADD_COPYING,
    FASTCPADD_FAILFASTFILE,

    //FAST COPY PROCESS
    FASTCPPRC_JOINFILE,
    FASTCPPRC_COPYFILE,
    FASTCPPTC_OPENERROR,

    //GAME TEST FILES
    GAMETESTS_FOUNDINSTALL,
    GAMETESTS_BIGFILE,
    GAMETESTS_TESTFILE,
    GAMETESTS_CHECKSIZE,

    //GAME DELETE FILES
    GAMEDELFL_DELETED,
    GAMEDELFL_DELETING,

    //GAME COPY
    GAMECOPYS_GSIZEABCNTASK,
    GAMECOPYS_STARTED,
    GAMECOPYS_SPLITEDHDDNFO,
    GAMECOPYS_SPLITEDUSBNFO,
    GAMECOPYS_DONE,
    GAMECOPYS_FAILDELDUMP,
    GAMECOPYS_FAILDELDUMPFR,

    //GAME CACHE COPY
    GAMECHCPY_ISNEEDONEFILE,
    GAMECHCPY_NEEDMORESPACE,
    GAMECHCPY_NOSPACE,
    GAMECHCPY_CACHENFOSTART,

    //GLOBAL UTILS
    GLUTIL_SPLITFILE,
    GLUTIL_WROTE,
    GLUTIL_TIME,
    GLUTIL_TIMELEFT,
    GLUTIL_HOLDTRIANGLEAB,
    GLUTIL_HOLDTRIANGLESK,
    GLUTIL_ABORTEDUSER,
    GLUTIL_ABORTED,
    GLUTIL_XEXIT,
    GLUTIL_WANTCPYFROM,
    GLUTIL_WTO,

    //GLOBAL
    GLOBAL_RETURN,
    GLOBAL_SAVED,

    //END
    LANGSTRINGS_COUNT
};


int open_language (char * filename);
void close_language(void);

#endif
