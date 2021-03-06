/* 
    (c) 2011 Hermes <www.elotrolado.net>
    IrisManager (HMANAGER port) (c) 2011 D_Skywalk <http://david.dantoine.org>

    HMANAGER4 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    HMANAGER4 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with HMANAGER4.  If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include <lv2/process.h>
#include <psl1ght/lv2/filesystem.h>
#include <psl1ght/lv2.h>
#include <sys/stat.h>
#include <sys/sysfs.h>

#include <sysmodule/sysmodule.h>
#include <pngdec/loadpng.h>


#include <io/pad.h>

#include <tiny3d.h>
#include <libfont.h>
#include "language.h"
#include "syscall8.h"
#include "payload355/payload.h"

#include "spu_soundmodule.bin.h" // load SPU Module
#include "spu_soundlib.h"

#include <gcmodplay.h>
#include "credits.h"
#include "main.h"
#include "music1_mod.bin.h"
#include "music2_mod.bin.h"
#include "music3_mod.bin.h"

// music
char * music[6] = {
            (char *) music1_mod_bin, 
            (char *) music2_mod_bin, 
            (char *) music3_mod_bin,
            "Song: Jester - stardust memories (1997)",
            "Song: Jester - elysium (1997)",
            "Song: jogeir-liljedahl - Overture (2000)"};

int song_selected = 0;
#define MAX_SONGS 3

// SPU
u32 spu = 0;
sysSpuImage spu_image;
#define SPU_SIZE(x) (((x)+127) & ~127)

MODPlay mod_track;

static u64 frame_count = 0;

// you need the Oopo ps3libraries to work with freetype

#include <ft2build.h>
#include <freetype/freetype.h> 
#include <freetype/ftglyph.h>

#include "gfx.h"
#include "utils.h"
#include "pad.h"

#include "ftp.h"

#ifndef WITH_CFW355
#include "payload_groove_hermes.bin.h"
#endif

// include fonts
#include "comfortaa_ttf.bin.h"
#include "comfortaa_bold_ttf.bin.h"

// font 2: 224 chr from 32 to 255, 16 x 32 pix 2 bit depth
#include "font_b.h"
#include "bluray_png.bin.h"
#include "direct_png.bin.h"
#include "usb_png.bin.h"
#include "missing_png.bin.h"

#define ROT_INC(x ,y , z) {x++; if(x > y) x = z;}
#define ROT_DEC(x ,y , z) {x--; if(x < y) x = z;}

int menu_screen = 0;
int mode_favourites = 1;

t_directories directories[MAX_DIRECTORIES];

int ndirectories = 0;

int currentdir = 0;
int currentgamedir = 0;

void unpatch_bdvdemu();
int patch_bdvdemu(u32 flags);
int move_origin_to_bdemubackup(char *path);
int move_bdemubackup_to_origin(u32 flags);

u8 * png_texture = NULL;
PngDatas Png_datas[16];
u32 Png_offset[16];
int Png_iscover[16];

PngDatas Png_res[16];
u32 Png_res_offset[16];

extern char * language[];
char self_path[MAXPATHLEN]= __MKDEF_MANAGER_FULLDIR__;

void Load_PNG_resources()
{
    int i;

    for(i = 0; i < 16; i++) Png_res[i].png_in = NULL;
    for(i = 0; i < 16; i++) Png_iscover[i] = 0;

    // datas for PNG from memory

    Png_res[0].png_in   = (void *) bluray_png_bin;
    Png_res[0].png_size = sizeof  (bluray_png_bin);

    Png_res[1].png_in   = (void *) usb_png_bin;
    Png_res[1].png_size = sizeof  (usb_png_bin);

    Png_res[2].png_in   = (void *) missing_png_bin;
    Png_res[2].png_size = sizeof  (missing_png_bin);

    Png_res[3].png_in   = (void *) direct_png_bin;
    Png_res[3].png_size = sizeof (direct_png_bin);

    // load PNG from memory

    for(i = 0; i < 16; i++)
        if(Png_res[i].png_in != NULL)
            LoadPNG(&Png_res[i], NULL);

}

int LoadTexturePNG(char * filename, int index)
{
    
    u32 * texture_pointer2 = (u32 *) (png_texture + index * 4096 * 1024); // 4 MB reserved for PNG index

    // here you can add more textures using 'texture_pointer'. It is returned aligned to 16 bytes
   
    memset(&Png_datas[index], 0, sizeof(PngDatas));
	if(LoadPNG(&Png_datas[index], filename) <0) memset(&Png_datas[index], 0, sizeof(PngDatas));
 
    Png_offset[index] = 0;
       
    if(Png_datas[index].bmp_out) {

        memcpy(texture_pointer2, Png_datas[index].bmp_out, Png_datas[index].wpitch * Png_datas[index].height);
        
        free(Png_datas[index].bmp_out);

        Png_datas[index].bmp_out= texture_pointer2;

        Png_offset[index] = tiny3d_TextureOffset(Png_datas[index].bmp_out);      // get the offset (RSX use offset instead address)

     return 0;
     } else {

         // fake PNG
        Png_datas[index].bmp_out= texture_pointer2;

        Png_offset[index] = tiny3d_TextureOffset(Png_datas[index].bmp_out);

        int n;
        u32 * text = texture_pointer2;

        Png_datas[index].width = Png_datas[index].height = 64;
        
        Png_datas[index].wpitch = Png_datas[index].width * 4;
       
        for (n = 0; n < Png_datas[index].width * Png_datas[index].height; n++) *text++ = 0xff000000;
       
     }

    return -1;
}

char path_name[MAXPATHLEN];

inline int get_icon(char * path, const int num_dir)
{
    struct stat s;

    sprintf(path, "%s/COVERS/%s.PNG", self_path, directories[num_dir].title_id);

    if(stat(path, &s)<0)
    {
        sprintf(path, "%s/PS3_GAME/ICON0.PNG", directories[num_dir].path_name);
        return 0;
    }
    else
        return 1;

}

void get_games() 
{
    int n;

    if(currentdir < 0 ||  currentdir >= ndirectories) currentdir = 0;

    if(mode_favourites) {
        for(n = 0; n < 12; n++) {
        
            if(favourites.list[n].index < 0 || favourites.list[n].title_id[0] == 0 || favourites.list[n].index >= ndirectories) Png_offset[n] = 0;
            else {

                Png_iscover[n] = get_icon(path_name, favourites.list[n].index);
                if(LoadTexturePNG(path_name, n) < 0) ;//Png_offset[n] = 0;

            }
        }
    
    return;
    }

    for(n = 0; n < 12; n++) {
        if((currentdir + n) < ndirectories) {
            
            Png_iscover[n] = get_icon(path_name, (currentdir + n));
            if(LoadTexturePNG(path_name, n) < 0) ;//Png_offset[n] = 0;

        } else Png_offset[n] = 0;
    }

}

/******************************************************************************************************************************************************/
/* TTF functions to load and convert fonts                                                                                                             */
/******************************************************************************************************************************************************/

int ttf_inited = 0;

FT_Library freetype;
FT_Face face;

/* TTFLoadFont can load TTF fonts from device or from memory:

path = path to the font or NULL to work from memory

from_memory = pointer to the font in memory. It is ignored if path != NULL.

size_from_memory = size of the memory font. It is ignored if path != NULL.

*/

int TTFLoadFont(char * path, void * from_memory, int size_from_memory)
{
   
    if(!ttf_inited)
        FT_Init_FreeType(&freetype);
    ttf_inited = 1;

    if(path) {
        if(FT_New_Face(freetype, path, 0, &face)) return -1;
    } else {
        if(FT_New_Memory_Face(freetype, from_memory, size_from_memory, 0, &face)) return -1;
        }

    return 0;
}

/* release all */

void TTFUnloadFont()
{
   FT_Done_FreeType(freetype);
   ttf_inited = 0;
}

/* function to render the character

chr : character from 0 to 255

bitmap: u8 bitmap passed to render the character character (max 256 x 256 x 1 (8 bits Alpha))

*w : w is the bitmap width as input and the width of the character (used to increase X) as output
*h : h is the bitmap height as input and the height of the character (used to Y correction combined with y_correction) as output

y_correction : the Y correction to display the character correctly in the screen

*/

void TTF_to_Bitmap(u8 chr, u8 * bitmap, short *w, short *h, short *y_correction)
{
    FT_Set_Pixel_Sizes(face, (*w), (*h));
    
    FT_GlyphSlot slot = face->glyph;

    memset(bitmap, 0, (*w) * (*h));

    if(FT_Load_Char(face, (char) chr, FT_LOAD_RENDER )) {(*w) = 0; return;}

    int n, m, ww;

    *y_correction = (*h) - 1 - slot->bitmap_top;
    
    ww = 0;

    for(n = 0; n < slot->bitmap.rows; n++) {
        for (m = 0; m < slot->bitmap.width; m++) {

            if(m >= (*w) || n >= (*h)) continue;
            
            bitmap[m] = (u8) slot->bitmap.buffer[ww + m];
        }
    
    bitmap += *w;

    ww += slot->bitmap.width;
    }

    *w = ((slot->advance.x + 31) >> 6) + ((slot->bitmap_left < 0) ? -slot->bitmap_left : 0);
    *h = slot->bitmap.rows;
}


void DrawCenteredBar2D(float y, float w, float h, u32 rgba)
{
    float x = (848.0f - w)/ 2.0f;

    tiny3d_SetPolygon(TINY3D_QUADS);

    tiny3d_VertexPos(x    , y    , 1.0f);
    tiny3d_VertexColor(rgba);

    tiny3d_VertexPos(x + w, y    , 1.0f);

    tiny3d_VertexPos(x + w, y + h, 1.0f);

    tiny3d_VertexPos(x    , y + h, 1.0f);
    tiny3d_End();
}

void LoadTexture()
{
    int i;

    u32 * texture_mem = tiny3d_AllocTexture(100*1024*1024); // alloc 100MB of space for textures (this pointer can be global)    

    u32 * texture_pointer; // use to asign texture space without changes texture_mem

    if(!texture_mem) return; // fail!

    texture_pointer = texture_mem;

    ResetFont();

    TTFLoadFont(NULL, (void *) comfortaa_ttf_bin, sizeof(comfortaa_ttf_bin));
    texture_pointer = (u32 *) AddFontFromTTF((u8 *) texture_pointer, 32, 255, 32, 32, TTF_to_Bitmap);
    texture_pointer = (u32 *) AddFontFromTTF((u8 *) texture_pointer, 32, 255, 20, 20, TTF_to_Bitmap);
    TTFUnloadFont();

    //debug font
    texture_pointer = (u32 *) AddFontFromBitmapArray((u8 *) font_b, (u8 *) texture_pointer, 32, 255, 16, 32, 2, BIT0_FIRST_PIXEL);

    //new button font
    TTFLoadFont(NULL, (void *) comfortaa_bold_ttf_bin, sizeof(comfortaa_bold_ttf_bin));
    texture_pointer = (u32 *) AddFontFromTTF((u8 *) texture_pointer, 32, 255, 24, 24, TTF_to_Bitmap);
    TTFUnloadFont();

    Load_PNG_resources();

    for(i = 0; i < 16; i++) {
       
        if(Png_res[i].png_in == NULL) continue;

        Png_res_offset[i]   = 0;
       
        if(Png_res[i].bmp_out) {

            memcpy(texture_pointer, Png_res[i].bmp_out, Png_res[i].wpitch * Png_res[i].height);
            
            free(Png_res[i].bmp_out); // free the PNG because i don't need this datas

            Png_res_offset[i] = tiny3d_TextureOffset(texture_pointer);      // get the offset (RSX use offset instead address)

            texture_pointer += ((Png_res[i].wpitch * Png_res[i].height + 15) & ~15) / 4; // aligned to 16 bytes (it is u32) and update the pointer
         }
    }



    png_texture = (u8 *) texture_pointer;
}


int background_sel = 0;

u32 background_colors[4] = {
    0xff80804f,
    0xff10000F,
    0xff606060,
    0xff904f80,
};

void cls()
{
    
    tiny3d_Clear(background_colors[background_sel & 3], TINY3D_CLEAR_ALL);
        
    // Enable alpha Test
    tiny3d_AlphaTest(1, 0x10, TINY3D_ALPHA_FUNC_GEQUAL);

    // Enable alpha blending.
    tiny3d_BlendFunc(1, TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA | TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_ALPHA,
        NV30_3D_BLEND_FUNC_DST_RGB_ONE_MINUS_SRC_ALPHA | NV30_3D_BLEND_FUNC_DST_ALPHA_ZERO,
        TINY3D_BLEND_RGB_FUNC_ADD | TINY3D_BLEND_ALPHA_FUNC_ADD);
}

void cls2()
{
    tiny3d_Clear(0xff000000, TINY3D_CLEAR_ALL);
        
    // Enable alpha Test
    tiny3d_AlphaTest(1, 0x10, TINY3D_ALPHA_FUNC_GEQUAL);

    // Enable alpha blending.
    tiny3d_BlendFunc(1, TINY3D_BLEND_FUNC_SRC_RGB_SRC_ALPHA | TINY3D_BLEND_FUNC_SRC_ALPHA_SRC_ALPHA,
        NV30_3D_BLEND_FUNC_DST_RGB_ONE_MINUS_SRC_ALPHA | NV30_3D_BLEND_FUNC_DST_ALPHA_ZERO,
        TINY3D_BLEND_RGB_FUNC_ADD | TINY3D_BLEND_ALPHA_FUNC_ADD);
}

/******************************************************************************************************************************************************/
/* Payload functions                                                                                                                                  */
/******************************************************************************************************************************************************/

u64 lv2peek(u64 addr) 
{ return Lv2Syscall1(6, (u64) addr); }

u64 lv2poke(u64 addr, u64 value) 
{ return Lv2Syscall2(7, (u64) addr, (u64) value); }

int lv2launch(u64 addr) 
{ return Lv2Syscall8(9, (u64) addr, 0,0,0,0,0,0,0); }

int syscall36(char * path) 
{ return Lv2Syscall1(36, (u64) path); }

//Syscall to get firmware version
#define SYS_GET_FIRMWARE_VERSION 376
#define FIRMWARE_356 0x8B10

u64 lv2GetFirmwareVersion(void)
{
	return Lv2Syscall0(SYS_GET_FIRMWARE_VERSION);
}


u64 hmanager_key = 0x1759829723742374ULL;

/******************************************************************************************************************************************************/

// manager

char temp_buffer[4096];
char payload_str[256];

int videoscale_x = 0;
int videoscale_y = 0;

int flash;

int select_px = 0;
int select_py = 0;

u32 blockSize;
u64 freeSize;
float freeSpace[12];

int select_option = 0;

u32 fdevices=0;
u32 fdevices_old=0;
u32 forcedevices=0;
int find_device=0;

char hdd_folder[64]="12345";

char bluray_game[64]; // name of the game

static int exit_program = 0;

#define ROUND_UP12(x) ((((x)+11)/12)*12)

void draw_screen1(float x, float y);
void draw_options(float x, float y, int index);
void draw_configs(float x, float y, int index);
void draw_gbloptions(float x, float y);
void draw_toolsoptions(float x, float y);
void draw_cachesel(float x, float y);

struct {
    int videoscale_x[4];
    int videoscale_y[4];
    int background_sel;
    char hdd_folder[64];
    u32 usekey;
    char pad[156];
} manager_oldcfg;

struct {
    int videoscale_x[4];
    int videoscale_y[4];
    int background_sel;
    char hdd_folder[64];
    u32 usekey;
    char pad[156];
    u32 opt_flags;
} manager_cfg;


struct {
    int version;
    int perm;
    int xmb;
    int updates;
    int ext_ebootbin;
    int bdemu;
    int exthdd0emu;
    int direct_boot;
    int pad[6];
} game_cfg;

int load_ps3loadx = 0;


int inited = 0;

#define INITED_SPU          2
#define INITED_SOUNDLIB     4
#define INITED_GCM_SYS      8
#define INITED_IO          16
#define INITED_PNGDEC      32
#define INITED_FS          64
#define INITED_MODLIB     128

void fun_exit()
{
	uninstall_lv2_new_syscalls();
	
    close_language();
    ftp_deinit();

    if(inited & INITED_SOUNDLIB) {
        if(inited & INITED_MODLIB)  
            MODPlay_Unload (&mod_track);
        SND_End();

        
    }

    if(inited & INITED_SPU) {
        sleep(1);
        lv2SpuRawDestroy(spu);
        sysSpuImageClose(&spu_image);
    }

    if(inited & INITED_GCM_SYS) SysUnloadModule(SYSMODULE_GCM_SYS);
    if(inited & INITED_IO)      SysUnloadModule(SYSMODULE_IO);
    if(inited & INITED_PNGDEC)  SysUnloadModule(SYSMODULE_PNGDEC);
    if(inited & INITED_FS)      SysUnloadModule(SYSMODULE_FS);

    inited = 0;
    if(manager_cfg.usekey) sys8_disable(hmanager_key);
    
    if(load_ps3loadx) {
        sysProcessExitSpawn2("/dev_hdd0/game/PSL145310/RELOAD.SELF", NULL, NULL, NULL, 0, 1001, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
        load_ps3loadx = 0;
    }
    
    if(game_cfg.direct_boot)
        sysProcessExitSpawn2("/dev_bdvd/PS3_GAME/USRDIR/EBOOT.BIN", NULL, NULL, NULL, 0, 3071, SYS_PROCESS_SPAWN_STACK_SIZE_1M);

}

void LoadManagerCfg()
{

    sprintf(temp_buffer, "%s/config/manager_setup.bin", self_path);

    int file_size;
    char *file = LoadFile(temp_buffer, &file_size);

    if(file)
    {
        if(file_size != sizeof(manager_cfg))
        {
            file_size = sizeof(manager_oldcfg); // safe
            manager_cfg.opt_flags |= OPTFLAGS_PLAYMUSIC; // enabled by default
        }

        memcpy(&manager_cfg, file, file_size);
        free(file);
    }
    else
    {
        manager_cfg.opt_flags |= OPTFLAGS_PLAYMUSIC; // enabled by default
    }

    if (manager_cfg.opt_flags & OPTFLAGS_FTP) // maybe we need add an icon to user...
    {
        if(ftp_init() == 0)
        {
            DrawDialogOK("FTP Service init on boot: OK");
        }
    }

    background_sel = manager_cfg.background_sel & 3;

    switch(Video_Resolution.height) {
        case 480:
            videoscale_x = manager_cfg.videoscale_x[0];
            videoscale_y = manager_cfg.videoscale_y[0];
            break;
       case 576:
            videoscale_x = manager_cfg.videoscale_x[1];
            videoscale_y = manager_cfg.videoscale_y[1];
            break;
       case 720:
            videoscale_x = manager_cfg.videoscale_x[2];
            videoscale_y = manager_cfg.videoscale_y[2];
            break;
       default:
            videoscale_x = manager_cfg.videoscale_x[3];
            videoscale_y = manager_cfg.videoscale_y[3];
            break;
    }

    char lang_chosen[256] = "language.ini";

    sprintf(temp_buffer, "%s/config/%s", self_path, lang_chosen);
    int errn = open_language(temp_buffer);
    if( errn < 0)
    {
        char errstr[256];
        sprintf(errstr, "Invalid language.ini file or not found (%i), need a reinstall?", errn); 

        tiny3d_Init(1024*1024*2);
        ioPadInit(7);

        DrawDialogOK(errstr);
        exit(0);
    }
}

int SaveManagerCfg()
{
    sprintf(temp_buffer, "%s/config/manager_setup.bin", self_path);
    return SaveFile(temp_buffer, (char *) &manager_cfg, sizeof(manager_cfg));
}

void video_adjust()
{
    while(1) {

        double sx = (double) Video_Resolution.width;
        double sy = (double) Video_Resolution.height;
        double psx = (double) (1000 + videoscale_x)/1000.0;
        double psy = (double) (1000 + videoscale_y)/1000.0;
        
        tiny3d_UserViewport(1, 
            (float) ((sx - sx * psx) / 2.0), // 2D position
            (float) ((sy - sy * psy) / 2.0), 
            (float) ((sx * psx) / 848.0),    // 2D scale
            (float) ((sy * psy) / 512.0),
            (float) ((sx / 1920.0) * psx),  // 3D scale
            (float) ((sy / 1080.0) * psy));

        cls();
        
        DrawAdjustBackground(0xffffffff) ; // light blue 

        update_twat();
        SetFontSize(16, 24);
        SetFontColor(0xffffffff, 0x0);

        SetFontAutoCenter(1);
        DrawFormatString(0, (512 - 24)/2 - 64, language[VIDEOADJUST_POSITION]);

        DrawFormatString(0, (512 - 24)/2, language[VIDEOADJUST_SCALEINFO], videoscale_x, videoscale_y);

        DrawFormatString(0, (512 - 24)/2 + 64, language[VIDEOADJUST_EXITINFO]);
        DrawFormatString(0, (512 - 24)/2 + 96, language[VIDEOADJUST_DEFAULTS]);
        SetFontAutoCenter(0);

        tiny3d_Flip();

        ps3pad_read();

        if(!(frame_count & 3)) {
            if(old_pad & BUTTON_UP) {if(videoscale_y > -179) videoscale_y--;}
            if(old_pad & BUTTON_DOWN) {if(videoscale_y < 10) videoscale_y++;}
            if(old_pad & BUTTON_LEFT) {if(videoscale_x > -199) videoscale_x--;}
            if(old_pad & BUTTON_RIGHT) {if(videoscale_x < 10) videoscale_x++;}
        }

        
        if(new_pad & BUTTON_CROSS) {

            switch(Video_Resolution.height) {
                case 480:
                    manager_cfg.videoscale_x[0] = videoscale_x;
                    manager_cfg.videoscale_y[0] = videoscale_y;
                    break;
               case 576:
                    manager_cfg.videoscale_x[1] = videoscale_x;
                    manager_cfg.videoscale_y[1] = videoscale_y;
                    break;
               case 720:
                    manager_cfg.videoscale_x[2] = videoscale_x;
                    manager_cfg.videoscale_y[2] = videoscale_y;
                    break;
               default:
                    manager_cfg.videoscale_x[3] = videoscale_x;
                    manager_cfg.videoscale_y[3] = videoscale_y;
                    break;
            }
            
      
            if(SaveManagerCfg() == 0) {
                sprintf(temp_buffer, "manager_setup.bin\n\n%s", language[GLOBAL_SAVED]);
                DrawDialogOK(temp_buffer);
            }

            break;
        }

        if(new_pad & BUTTON_CIRCLE) {videoscale_x = videoscale_y = -120;}

        frame_count++;
    }
}

void Select_games_folder() 
{

    DIR  *dir, *dir2;
    int selected = 0;
    char tmp[256];

    dir = opendir ("/dev_hdd0/GAMES");
    if(dir) {
        closedir (dir);
        sprintf(temp_buffer, "%s %s %s", language[GAMEFOLDER_WANTUSE], "/dev_hdd0/GAMES", language[GAMEFOLDER_TOINSTALLNTR]); 

        if(DrawDialogYesNo(temp_buffer) == 1) {
            strncpy(hdd_folder, "dev_hdd0_2", 64);
            strncpy(manager_cfg.hdd_folder, "dev_hdd0_2", 64); 
            return;
        }
    }
    
    dir = opendir ("/dev_hdd0/" __MKDEF_GAMES_DIR);
    if(dir) {
        closedir (dir);
        sprintf(temp_buffer, "%s %s %s", language[GAMEFOLDER_WANTUSE], "/dev_hdd0/" __MKDEF_GAMES_DIR, language[GAMEFOLDER_TOINSTALLNTR]); 

        if(DrawDialogYesNo(temp_buffer) == 1) {
            strncpy(hdd_folder, "dev_hdd0", 64);
            strncpy(manager_cfg.hdd_folder, "dev_hdd0", 64); 
            return;
        }
    }

    dir = opendir ("/dev_hdd0/game");

    if(dir) {

        while(1) {

            struct dirent *entry = readdir (dir);
          
            if(!entry) break;

            if(entry->d_name[0]=='.') continue;

            if(!(entry->d_type & DT_DIR)) continue;

            sprintf(temp_buffer, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR, entry->d_name);
        
            dir2 = opendir (temp_buffer);

            if(dir2) {
                
                closedir (dir2);
      
                sprintf(temp_buffer, "%s /%s %s", language[GAMEFOLDER_WANTUSE], entry->d_name, language[GAMEFOLDER_TOINSTALLNTR]);

                if(DrawDialogYesNo(temp_buffer) == 1) {
                    strncpy(hdd_folder, entry->d_name, 64);
                    strncpy(manager_cfg.hdd_folder, entry->d_name, 64);
                    selected = 1;
                    break;
                }
            }
            
          }
    closedir (dir);
    }

    if(!selected) {
        
        sprintf(temp_buffer, "%s %s %s", language[GAMEFOLDER_WANTUSE], "/dev_hdd0/" __MKDEF_GAMES_DIR, language[GAMEFOLDER_TOINSTALLNTR]);

        if(DrawDialogYesNo(temp_buffer) == 1) {
            strncpy(hdd_folder, "dev_hdd0", 64);
            strncpy(manager_cfg.hdd_folder, "dev_hdd0", 64); 
            mkdir("/dev_hdd0/" __MKDEF_GAMES_DIR, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
        } else {
            strncpy(hdd_folder, __MKDEF_MANAGER_DIR__, 64);
            strncpy(manager_cfg.hdd_folder, __MKDEF_MANAGER_DIR__, 64);
            sprintf(tmp, "%s/" __MKDEF_GAMES_DIR, __MKDEF_MANAGER_FULLDIR__);
            mkdir(tmp, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);

            sprintf(temp_buffer, "%s %s %s", language[GAMEFOLDER_USING], tmp, language[GAMEFOLDER_TOINSTALL]);
            DrawDialogOK(temp_buffer);
        }
    }
}

void pause_music(int pause)
{
    if((!pause)&&(!(manager_cfg.opt_flags & OPTFLAGS_PLAYMUSIC)))
        return;

    SND_Pause(pause);
}

void init_music()
{
    MODPlay_Init(&mod_track);
    
    int file_size;
    char *file;

    sprintf(temp_buffer, "%s/music.mod", self_path);
    file = LoadFile(temp_buffer, &file_size);

    if(!file) {
   
        sprintf(temp_buffer, "%s/MUSIC.MOD", self_path);
        file = LoadFile(temp_buffer, &file_size);
    }
    
    if(!file) {
        srand(time(0)); // randomize seed
        song_selected = rand() % MAX_SONGS;
        file = (char *) music[song_selected];
    } else {
        // paranoid code to copy the .mod in aligned and large memory

        char *file2 = memalign(32, file_size + 32768);
        if(file2) {memcpy(file2, file, file_size);free(file); file = file2;}
    }

    if(MODPlay_SetMOD (&mod_track, file)<0) {
        MODPlay_Unload (&mod_track);
    } else {
        MODPlay_SetVolume( &mod_track, 6, 6); // fix the volume to 16 (max 64)
	    MODPlay_Start (&mod_track); // Play the MOD
        inited |= INITED_MODLIB;
        SND_Pause(1); //force pause here
    }

}

static char filename[1024];
int payload_mode = 0;

/******************************************************************************************************************************************************/

s32 main(s32 argc, const char* argv[])
{
	if(lv2GetFirmwareVersion() != FIRMWARE_356) //Avoid running on non 356 firmwares
		return 0;
		
    int n;
    
    u32 entry = 0;
    u32 segmentcount = 0;
    sysSpuSegment * segments;

    atexit(fun_exit);

    if(SysLoadModule(SYSMODULE_FS) ==0)      inited|= INITED_FS;      else exit(0);
    if(SysLoadModule(SYSMODULE_PNGDEC) ==0)  inited|= INITED_PNGDEC;  else exit(0);
    if(SysLoadModule(SYSMODULE_IO) ==0)      inited|= INITED_IO;      else exit(0);
    if(SysLoadModule(SYSMODULE_GCM_SYS) ==0) inited|= INITED_GCM_SYS; else exit(0);

    lv2SpuInitialize(6, 5);

    lv2SpuRawCreate(&spu, NULL);

    sysSpuElfGetInformation(spu_soundmodule_bin, &entry, &segmentcount);

    size_t segmentsize = sizeof(sysSpuSegment) * segmentcount;
    segments = (sysSpuSegment*)memalign(128, SPU_SIZE(segmentsize)); // must be aligned to 128 or it break malloc() allocations
    memset(segments, 0, segmentsize);

    sysSpuElfGetSegments(spu_soundmodule_bin, segments, segmentcount);
    
    sysSpuImageImport(&spu_image, spu_soundmodule_bin, 0);
    
    sysSpuRawImageLoad(spu, &spu_image);

    inited |= INITED_SPU;

    if(SND_Init(spu)==0) inited |= INITED_SOUNDLIB;

    if(argc>0 && argv) {
    
        if(!strncmp(argv[0], "/dev_hdd0/game/", 15)) {
            int n;

            strcpy(self_path, argv[0]);

            n= 15; while(self_path[n] != '/' && self_path[n] != 0) n++;

            if(self_path[n] == '/') {
                self_path[n] = 0;
            }
        }
    }
	
	
    tiny3d_Init(1024*1024*2);
    ioPadInit(7);

    payload_mode = is_payload_loaded();

    switch(payload_mode)
    {
        case ZERO_PAYLOAD: //no payload installed
    		
			payload_mode = SKY10_PAYLOAD;
			patch_lv2_protection();
			load_payload(payload_mode);
			
			break;
			
		case SKY10_PAYLOAD:
			install_lv2_new_syscalls();
            break;
			
		default:
			DrawDialogOK("Unkown error\n");
	}

    //usleep(250000);
/*
    if(payload_mode >= ZERO_PAYLOAD)
    {
        int test = 0x100;

        //check syscall8 status
        test = sys8_enable(0ULL);
        if(test < 6)
        {
                sprintf(payload_str, "payload-sk10 - new syscall8 Err?! v(%i)", test);
        }
        else
        {
            if(payload_mode == ZERO_PAYLOAD)
                sprintf(payload_str, "payload-sk10 - new syscall8 v%i", test);
            else if (payload_mode == SKY10_PAYLOAD)
                sprintf(payload_str, "payload-sk10 resident - new syscall8 v%i", test);
        }
    }

    sys8_perm_mode(2);
    sys8_path_table(0LL);
*/
    if(payload_mode < ZERO_PAYLOAD) //if mode is wanin or worse, launch advert
    {
        DrawDialogOK(temp_buffer);
    }

    sprintf(temp_buffer, "%s/config", self_path);

    mkdir(temp_buffer, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);

    sprintf(temp_buffer, "%s/self", self_path);

    mkdir(temp_buffer, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
        
	// Load texture
	

    LoadTexture();

    init_twat();

	
    // initialize manager conf

    memset(&manager_cfg, 0, sizeof(manager_cfg));

    for(n=0; n<4; n++) 
        manager_cfg.videoscale_x[n] = 1024;

    manager_cfg.background_sel = 0;

    //load cfg and language strings
    LoadManagerCfg();

    if(0)
    {
        float x = 0.0f, y = 0.0f;

        cls();

        SetFontSize(32, 64);
       
        SetFontColor(0xffffffff, 0x00000000);
        SetFontAutoCenter(1);

        x= (848-640) / 2; y=(512-360)/2;
        DrawBox(x - 16, y - 16, 65535.0f, 640.0f + 32, 360 + 32, 0x00000028);
        DrawBox(x, y, 65535.0f, 640.0f, 360, 0x30003018);

        x= 0.0; y = 512.0f/2.0f - 32.0f;
        DrawFormatString(0, y, "Payload WaninV2 Patched");
        
        SetFontAutoCenter(0);
        tiny3d_Flip();
    
        sleep(2);
    }
    
    if(videoscale_x >= 1024) {
        videoscale_x = videoscale_y = 0;
        video_adjust();
    }

    if(manager_cfg.hdd_folder[0] == 0) {
        
        Select_games_folder();
     
        if(manager_cfg.hdd_folder[0] == 0) strcpy(manager_cfg.hdd_folder, __MKDEF_MANAGER_DIR__);
        SaveManagerCfg();
    } 
    
    strncpy(hdd_folder, manager_cfg.hdd_folder, 64);
   

    double sx = (double) Video_Resolution.width;
    double sy = (double) Video_Resolution.height;
    double psx = (double) (1000 + videoscale_x)/1000.0;
    double psy = (double) (1000 + videoscale_y)/1000.0;
    
    tiny3d_UserViewport(1, 
        (float) ((sx - sx * psx) / 2.0), // 2D position
        (float) ((sy - sy * psy) / 2.0), 
        (float) ((sx * psx) / 848.0),    // 2D scale
        (float) ((sy * psy) / 512.0),
        (float) ((sx / 1920.0) * psx),  // 3D scale
        (float) ((sy / 1080.0) * psy));

    select_px = select_py = 0;

    fdevices=0;
	fdevices_old=0;
	forcedevices=0;
	find_device=0;
	
    syscall36("/dev_bdvd");
    //sys8_perm_mode((u64) 2);
	
    unpatch_bdvdemu();
	
    init_music();

    sprintf(temp_buffer, "%s/config/favourites.bin", self_path);

    LoadFavourites(temp_buffer);

    while(!exit_program) {

        float x = 0.0f, y = 0.0f;
    
        flash = (frame_count >> 5) & 1;

        frame_count++;

        int count_devices=0;

        int found_game_insert=0;
        
        int found_game_remove=0;

        if(tiny3d_MenuActive()) frame_count = 32; // to avoid the access to hdd when menu is active
        
        if(forcedevices || (frame_count & 63)==0 || fdevices == 0)
	    for(find_device = 0; find_device < 12; find_device++) {

			if(find_device==11) sprintf(filename, "/dev_bdvd");
			else if(find_device==0) sprintf(filename, "/dev_hdd0");
			else sprintf(filename, "/dev_usb00%c", 47+find_device);

            DIR  *dir;
			dir = opendir (filename);

			if (dir) {
                closedir (dir);

                if(find_device > 0 && find_device < 11) move_bdemubackup_to_origin(1 << find_device);

				fdevices|= 1<<find_device;
		    } else
				fdevices&= ~ (1<<find_device);

			// limit to 3 the devices selectables
			if(((fdevices>>find_device) & 1) && find_device!=11) {
			
                count_devices++;

				if(count_devices>3) fdevices&= ~ (1<<find_device);

			}

			// bdvd
			if(find_device==11) {
				
				if(fdevices!=fdevices_old || ((forcedevices>>find_device) & 1)) {
				
                    found_game_insert = 1;
                    currentdir=0;
					sprintf(filename, "/dev_bdvd/PS3_GAME/PARAM.SFO");
					bluray_game[0]=0;
					if(parse_param_sfo("/dev_bdvd/PS3_GAME/PARAM.SFO", bluray_game)==-1);
					bluray_game[63]=0;
					found_game_insert=1;		
					if((fdevices>>11) & 1) {

						if(ndirectories>=MAX_DIRECTORIES) ndirectories= MAX_DIRECTORIES-1;
								
						sprintf(directories[ndirectories].path_name, "/dev_bdvd");
								
					    memcpy(directories[ndirectories].title, bluray_game, 63);
						directories[ndirectories].title[63]=0;
						directories[ndirectories].flags=(1<<11);

                        sprintf(filename, "%s/%s", directories[ndirectories].path_name, "PS3_DISC.SFB" );
                        parse_ps3_disc((char *) filename, directories[ndirectories].title_id);
                        directories[ndirectories].title_id[63]=0;
                        //parse_param_sfo_id(filename, directories[ndirectories].title_id);
                        //directories[ndirectories].title_id[63]=0;
                       
						ndirectories++;
                        found_game_insert=1;

		    	    } else {
						
                        delete_entries(directories, &ndirectories, (1<<11));
                        found_game_remove=1;
                    }

					sort_entries(directories, &ndirectories );              
				}
				
				forcedevices &= ~ (1<<find_device);
				fdevices_old &= ~ (1<<find_device);
				fdevices_old |= fdevices & (1<<find_device);
			} else
			// refresh list 
			if(fdevices!=fdevices_old || ((forcedevices>>find_device) & 1)) {
					
                currentdir=0;
				found_game_insert = 1;	
				forcedevices &= ~ (1<<find_device);

				if(find_device==0) {
                    if (!memcmp(hdd_folder,"dev_hdd0",9)) {
                        sprintf(filename, "/%s/" __MKDEF_GAMES_DIR,hdd_folder); 
                    } else if (!memcmp(hdd_folder,"dev_hdd0_2", 11)) {
                        sprintf(filename, "/%s/GAMES", "dev_hdd0"); 
                    } 
                    else {
                        sprintf(filename, "/dev_hdd0/game/%s/" __MKDEF_GAMES_DIR,hdd_folder);  
                    }
 
                    sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
                    freeSpace[find_device] = ( ((u64)blockSize * freeSize));
                    freeSpace[find_device] = freeSpace[find_device] / 1073741824.0;
            
                } else {
                    sprintf(filename, "/dev_usb00%c/", 47+find_device);
                    sysFsGetFreeSize(filename, &blockSize, &freeSize);
                    double space = ( ((double)blockSize) * ((double) freeSize) ) /  1073741824.0;
                    freeSpace[find_device] = (float) space;
                    sprintf(filename, "/dev_usb00%c/" __MKDEF_GAMES_DIR, 47+find_device);
                }

		        if((fdevices>>find_device) & 1) {
				    fill_entries_from_device(filename, directories, &ndirectories, (1<<find_device), 0);
                    found_game_insert=1;
                } else {
					
                    delete_entries(directories, &ndirectories, (1<<find_device));
                    found_game_remove=1;
                }
				
                sort_entries(directories, &ndirectories );

				fdevices_old&= ~ (1<<find_device);
				fdevices_old|= fdevices & (1<<find_device);
			}
		}
       
        if (found_game_insert || found_game_remove){

          UpdateFavourites(directories, ndirectories);

          if(mode_favourites && !havefavourites) mode_favourites = 0;
          get_games();
          load_gamecfg(-1); // force refresh game info

          mode_favourites = mode_favourites != 0; // avoid insert favourites

          select_option = 0;     
          menu_screen = 0;
        }
     
        found_game_remove=0;
        found_game_insert=0;

        pause_music(0);

        /////////////////////////////////////

        cls();

        update_twat();

        x= (848 - 640) / 2; y=(512 - 360) / 2;
//        DrawBox(x - 16, y - 16, 65535.0f, 640.0f + 32, 360 + 32, 0x00000028);
  //      DrawBox(x, y, 65535.0f, 640.0f, 360, 0x30003018);

         
        x= 28; y= 0;

        if((old_pad & (BUTTON_L2 | BUTTON_R2 | BUTTON_START)) == (BUTTON_L2 | BUTTON_R2 | BUTTON_START)) {
            
            videoscale_x = videoscale_y = 0;
            video_adjust();
        }

        // paranoid checks

        if(select_px < 0 || select_px > 3) select_px = 0;
        if(select_py < 0 || select_py > 2) select_py = 0;
        if(currentdir < 0 || currentdir >= ndirectories) currentdir = 0;
        if(currentgamedir < 0 || currentgamedir >= ndirectories) currentgamedir = 0;

       
       // paranoid favourite check
        for(n = 0; n < 12; n++) {
            if(favourites.list[n].index >=0) {
                if(favourites.list[n].title_id[0] == 0) exit(0);
                if(favourites.list[n].index >= ndirectories) exit(0);
                if(directories[favourites.list[n].index].flags == 0) exit(0);
            }
        }
                
            
        switch(menu_screen) {
            case 0:
                draw_screen1(x, y);
                break;
            case 1:
                draw_options(x, y, currentgamedir);
                break;
            case 2:
                draw_configs(x, y, currentgamedir);
                break;
            case 3:
                draw_gbloptions(x, y);
                break;
            case 4:
                draw_toolsoptions(x, y);
                break;
            case 5:
                draw_cachesel(x, y);
                break;
            default:
                menu_screen = 0;
                break;
        }

    
     }
     
	return 0;
}

// draw_cachesel
struct {
    u64 size;
    char title[64];
    char title_id[64];
} cache_list[64];

int ncache_list = 0;

void LoadCacheDatas() 
{
    DIR  *dir, *dir2;

    sprintf(temp_buffer, "%s/cache", self_path);
    dir = opendir (temp_buffer);
    if(!dir) return;

    sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
    freeSpace[0] = ( ((u64)blockSize * freeSize));
    freeSpace[0] = freeSpace[0] / 1073741824.0;

    ncache_list = 0;

    while(1) {
        struct dirent *entry= readdir (dir);
        
        if(!entry) break;
        if(entry->d_name[0]=='.') continue;

        if(!(entry->d_type & DT_DIR)) continue;

        strncpy(cache_list[ncache_list].title_id, entry->d_name, 64);

        cache_list[ncache_list].size = 0ULL;

        sprintf(temp_buffer + 1024, "%s/cache/%s/name_entry", self_path, entry->d_name);
        int size;
        char *name = LoadFile(temp_buffer + 1024, &size);

        memset(cache_list[ncache_list].title, 0, 64);
        if(name) {
            memcpy(cache_list[ncache_list].title, name, (size < 64) ? size : 63);
            free(name);
        }

        sprintf(temp_buffer + 1024, "%s/cache/%s", self_path, entry->d_name);
        dir2 = opendir (temp_buffer + 1024);
        if(dir2) {
            while(1) {
                struct dirent *entry2= readdir (dir2);
                struct stat s;
                
                if(!entry2) break;
                if(entry2->d_name[0]=='.') continue;

                if((entry2->d_type & DT_DIR)) continue;

                sprintf(temp_buffer + 2048, "%s/cache/%s/%s", self_path, entry->d_name, entry2->d_name);
                if(stat(temp_buffer + 2048, &s) == 0) {
                    cache_list[ncache_list].size += s.st_size;
                }
            }
        }

        ncache_list++; if(ncache_list >= 64) break;
    }
}

inline int get_currentdir(int i)
{
    if(mode_favourites !=0)
        return favourites.list[i].index;

    return (currentdir + i);
}

void load_gamecfg (int current_dir)
{

    static int last_selected = -1;
    
    if(current_dir < 0) //check reset info
    {
        last_selected = current_dir;
        memset(&game_cfg, 0, sizeof(game_cfg));
        return;
    }
    
    if(last_selected == current_dir)
        return;
    
    last_selected = current_dir; //prevents load again
    
    sprintf(temp_buffer, "%s/config/%s.cfg", self_path, directories[current_dir].title_id);
    memset(&game_cfg, 0, sizeof(game_cfg));

    int file_size;
    char *file = LoadFile(temp_buffer, &file_size);
    if(file)
    {
        if(file_size > sizeof(game_cfg)) file_size = sizeof(game_cfg);
        memcpy(&game_cfg, file, file_size);
        free(file);
    }
}

int check_disc(void)
{
    int get_user = 1;

    while(get_user == 1)
    {
        DIR  *dir;
        dir = opendir ("/dev_bdvd");

        if(dir)
        {
            closedir (dir);
            return 1;
        }
        else
            get_user = DrawDialogYesNo(language[DRAWSCREEN_REQBR]);
    }
    
    return -1;
}

void draw_screen1(float x, float y)
{

    int i, n, m;

    float x2;

    int selected = select_px + select_py * 4;

    SetCurrentFont(FONT_DEFAULT);

    // header title

    DrawBox(x, y, 0, 200 * 4 - 8, 18, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);

    if(mode_favourites >= 131072) DrawFormatString(x, y - 2, " %s", language[DRAWSCREEN_FAVSWAP]);
    else if(mode_favourites >= 65536) DrawFormatString(x, y - 2, " %s", language[DRAWSCREEN_FAVINSERT]);
    else if(mode_favourites) DrawFormatString(x, y - 2, " %s", language[DRAWSCREEN_FAVORITES]);
    else DrawFormatString(x, y - 5, " %s %i/%i (%i %s)", language[DRAWSCREEN_PAGE], currentdir/12 + 1, ROUND_UP12(ndirectories)/12, ndirectories, language[DRAWSCREEN_GAMES]);

    // list device space

    m = selected;

    if(Png_offset[m]) {

        i = -1;

        if(!mode_favourites || ((mode_favourites !=0) && favourites.list[m].index >= 0))
            for(i = 0; i < 11; i++)
                if(directories[(mode_favourites !=0) ? favourites.list[m].index : (currentdir + m)].flags == (1<<i)) break;
        m = i;
    } else m = -1;

    for(n = 0; n < 2; n++) {

        if(n == 0) x2 = 2000;

        for(i = 0; i < 11; i++) {

            if(((fdevices>>i) & 1)) {
                
                if(m == i) SetFontColor(0xafd836ff, 0x00000000); else SetFontColor(0xffffff44, 0x00000000);
                if(i==0)
                    x2= DrawFormatString(x2, -4, "hdd0: %.2fGB ", freeSpace[i]);
                else
                    x2= DrawFormatString(x2, -4, "usb00%c: %.2fGB ", 47 + i, freeSpace[i]);
            }

        }
    
    x2 = 848 -(x2 - 2000) - x;
    }

    SetFontAutoCenter(0);
    SetFontSize(18, 20);

    SetFontColor(0xffffffff, 0x00000000);

    y += 24;
    
    i = 0;

    for(n = 0; n < 3; n++) 
        for(m = 0; m < 4; m++) {
            
            int f = select_px == m && select_py == n;

            DrawBox(x + 200 * m - 4 * f, y + n * 150 - 4 * f, 0, 192 + 8 * f, 142 + 8 * f, 0x00000028 );
           

            //draw Splited box
            //if(directories[currentgamedir].splitted)
            //    DrawBox(x + 198 * m, (y - 2) + n * 150, 0, 194, 144, 0x55ff3328 );
            
            if(Png_offset[i]) {
               
                tiny3d_SetTextureWrap(0, Png_offset[i], Png_datas[i].width, 
                Png_datas[i].height, Png_datas[i].wpitch, 
                    TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

                if(Png_iscover[i])
                    DrawTextBox(x + 36 + 200 * m - 4 * f, y + n * 150  - 4 * f, 0, 124 + 8 * f, 142 + 8 * f, 0xffffffff);
                else
                    DrawTextBox(x + 200 * m - 4 * f, y + n * 150  - 4 * f, 0, 192 + 8 * f, 142 + 8 * f, 0xffffffff);
                
               // if((mode_favourites !=0) && favourites.list[i].index < 0) exit(0);
                if((mode_favourites !=0) && favourites.list[i].index < 0) exit(0);

                if(!mode_favourites || ((mode_favourites !=0) && favourites.list[i].index >= 0)) {
                    // draw Bluray icon
                    if(directories[get_currentdir(i)].flags == (1<<11)) {
                        tiny3d_SetTextureWrap(0, Png_res_offset[0], Png_res[0].width, 
                        Png_res[0].height, Png_res[0].wpitch, 
                            TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

                        DrawTextBox(x + 200 * m + 4 - 4 * f, y + n * 150 + 4 - 4 * f, 0, 32, 32, 0xffffffcf);
                    } else 
                    // draw Usb icon    
                    if(directories[get_currentdir(i)].flags > 1) {
                        tiny3d_SetTextureWrap(0, Png_res_offset[1], Png_res[1].width, 
                        Png_res[1].height, Png_res[1].wpitch, 
                            TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

                        if(directories[get_currentdir(i)].splitted)
                            DrawTextBox(x + 200 * m + 4 - 4 * f, y + n * 150 + 4 - 4 * f, 0, 32, 24, 0xff9999aa);
                        else
                            DrawTextBox(x + 200 * m + 4 - 4 * f, y + n * 150 + 4 - 4 * f, 0, 32, 24, 0xffffffcf);
                        
                    }
                }

            } else if(mode_favourites && favourites.list[i].title_id[0] != 0) {
                tiny3d_SetTextureWrap(0, Png_res_offset[2], Png_res[2].width, 
                    Png_res[2].height, Png_res[2].wpitch, 
                        TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                    DrawTextBox(x + 200 * m + 32 - 4 * f, y + n * 150 + 7 - 4 * f, 0, 128 + 8 * f, 128 + 8 * f, 0xffffff3f); 
            }
            
        i++;   
        }

    i = selected;

    if(flash) {

        int png_on = 0;

        //DrawBox(x + 200 * select_px - 4, y + select_py * 150 - 4 , 0, 200, 150, 0xa0a06080);
            

        if(mode_favourites >= 65536) {

            if(mode_favourites < 131072) {

                if(Png_offset[12]) {
                    tiny3d_SetTextureWrap(0, Png_offset[12], Png_datas[12].width, 
                        Png_datas[12].height, Png_datas[12].wpitch, 
                            TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                    png_on = 1;
                }
            }
            else {
                i = mode_favourites - 131072;

                if(i>= 0 && i < 12) {
                    if(!Png_offset[i] && favourites.list[i].title_id[0] != 0) {
                        tiny3d_SetTextureWrap(0, Png_res_offset[0], Png_res[0].width, 
                            Png_res[0].height, Png_res[0].wpitch, 
                                TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                        png_on = 1;
                            
                    } else if(Png_offset[i]){
                        png_on = 1;
                        tiny3d_SetTextureWrap(0, Png_offset[i], Png_datas[i].width, 
                        Png_datas[i].height, Png_datas[i].wpitch, 
                            TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                    }
                }
            }

            if(png_on)
                DrawTextBox(x + 200 * select_px - 4, y + select_py * 150 - 4 , 0, 200, 150, 0x8fff8fcf);
        }
    
    }

    SetFontColor(0xffffffff, 0x00000000);

    if(Png_offset[i])
    {
    
        DrawBox(x + 200 * select_px - 8 + (200 - 24 * 8)/2, y + select_py * 150 - 4 + 150 - 40, 0, 200, 40, 0x404040ac);
        SetCurrentFont(FONT_NEWBUTTON);
        SetFontSize(24, 24);
        x2 = DrawFormatString(x + 200 * select_px - 4 + (200 - 24 * 8)/2, y + select_py * 150 - 4 + 150 - 40, "  %s ", language[DRAWSCREEN_PLAY]);

        SetCurrentFont(FONT_DEFAULT);
        SetFontSize(20, 20);
        x2= DrawFormatString(1024, 0, " %s ", language[DRAWSCREEN_SOPTIONS]);
        DrawFormatString(x + 4 * 200 - (x2 - 1024) - 12 , y + 3 * 150 - 4, " %s ", language[DRAWSCREEN_SOPTIONS]);
   
    }
    else if(mode_favourites && mode_favourites < 65536 && favourites.list[i].title_id[0] != 0) 
    {
        DrawBox(x + 200 * select_px - 8 + (200 - 24 * 8)/2, y + select_py * 150 - 4 + 150 - 40, 0, 200, 40, 0x404040ac);
        SetCurrentFont(FONT_NEWBUTTON);
        SetFontSize(24, 24);
        x2= DrawFormatString(x + 200 * select_px - 4 + (200 - 23 * 8)/2, y + select_py * 150 - 4 + 150 - 24, "  %s ", language[DRAWSCREEN_PLAY]);

        SetCurrentFont(FONT_DEFAULT);
        SetFontSize(20, 20);
        x2= DrawFormatString(1024, 0, " %s ", language[DRAWSCREEN_SDELETE]);
        DrawFormatString(x + 4 * 200 - (x2 - 1024) - 12 , y + 3 * 150 - 4, " %s ", language[DRAWSCREEN_SDELETE]);
    }
    else
    {
        DrawBox(x + 200 * select_px , y + select_py * 150 , 0, 192, 142, 0x40404040);
        SetCurrentFont(FONT_DEFAULT); // get default
        SetFontSize(20, 20);
    }
    
    x2= DrawFormatString(1024, 0, " %s ", language[DRAWSCREEN_STGLOPT]);

    DrawFormatString(x + 4 * 200 - (x2 - 1024) - 12 , y + 3 * 150 + 18, " %s ", language[DRAWSCREEN_STGLOPT]);

    // draw game name
    i = selected;

    DrawBox(x, y + 3 * 150, 0, 200 * 4 - 8, 40, 0x00000028);

    SetFontColor(0xffffffee, 0x00000000);

    if((Png_offset[i] && !mode_favourites) || (mode_favourites && favourites.list[i].title_id[0] != 0)) {

        if(mode_favourites) {

            utf8_to_ansi(favourites.list[i].title, temp_buffer, 65);

        } else if(directories[(currentdir + i)].flags == (1<<11)) {
            utf8_to_ansi(bluray_game, temp_buffer, 65);
            SetFontColor(0xafd836ee, 0x00000000);
        } else utf8_to_ansi(directories[(currentdir + i)].title, temp_buffer, 65);

        temp_buffer[65] = 0;

        if(strlen(temp_buffer) < 50) SetFontSize(28, 28); 
        else SetFontSize(20, 20);

        SetFontAutoCenter(0);
  
        DrawFormatString(x + 3, y + 3 * 150, temp_buffer);

        SetFontAutoCenter(0);

        load_gamecfg (get_currentdir(i)); // refresh game info

        if((game_cfg.xmb) || (game_cfg.direct_boot == 2))
        {
            tiny3d_SetTextureWrap(0, Png_res_offset[0], Png_res[0].width, 
                Png_res[0].height, Png_res[0].wpitch, 
                TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

            DrawTextBox(x + 200 * select_px + 148 + (200 - 23 * 8)/2, y + select_py * 150 - 4 + 150 - 36, 0, 32, 32, 0xffffff99);
        }
        
        if(game_cfg.direct_boot)
        {
            tiny3d_SetTextureWrap(0, Png_res_offset[3], Png_res[3].width, 
                Png_res[3].height, Png_res[3].wpitch, 
                TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);

            DrawTextBox(x + 200 * select_px + 148 + (200 - 23 * 8)/2, y + select_py * 150 - 4 + 150 - 36, 0, 32, 32, 0xffffffff);        
        }

    }

    //SetCurrentFont(FONT_DEFAULT);

    tiny3d_Flip();

    ps3pad_read();

    if((old_pad & BUTTON_L2) && (new_pad & BUTTON_TRIANGLE)) {
        mode_favourites = 131072 | (selected);
        return;
    }

    if(new_pad & BUTTON_TRIANGLE) {
        if(mode_favourites >= 65536) {
            mode_favourites = 1;

        } else {
            if(DrawDialogYesNo(language[DRAWSCREEN_EXITXMB])==1) {exit_program = 1; return;}
        }
    }

    if(new_pad & BUTTON_CROSS) {
        i = selected;

        if(mode_favourites >= 131072) { // swap favourites
            
            entry_favourites swap = favourites.list[i];

            favourites.list[i] = favourites.list[mode_favourites - 131072]; favourites.list[mode_favourites - 131072] = swap;

            sprintf(temp_buffer, "%s/config/favourites.bin", self_path);
            SaveFavourites(temp_buffer);

            mode_favourites = 1;
            get_games();

            return;     
        }

        if(mode_favourites >= 65536) { // insert favourites
            
            DeleteFavouritesIfExits(directories[mode_favourites - 65536].title_id);
            AddFavourites(i, directories, mode_favourites - 65536);

            sprintf(temp_buffer, "%s/config/favourites.bin", self_path);
            SaveFavourites(temp_buffer);

            mode_favourites = 1;
            get_games();

            return;     
        }

        if(Png_offset[i]) {
            if(mode_favourites != 0 && favourites.list[i].index < 0) {
                DrawDialogOK(language[DRAWSCREEN_CANRUNFAV]);return;
            } else {

                currentgamedir = (mode_favourites !=0) ? favourites.list[i].index : (currentdir + i);

                if(currentgamedir < 0 || currentgamedir >= ndirectories) return;

                reset_sys8_path_table();

                int use_cache = 0;

                if(directories[currentgamedir].splitted == 1) {
                    if( payload_mode >= ZERO_PAYLOAD )
                    {
                        sprintf(temp_buffer, "%s/cache/%s/%s", self_path, 
                        directories[currentgamedir].title_id, "/paths.dir");

                        struct stat s;
                    
                        if(stat(temp_buffer, &s)<0) {
                            sprintf(temp_buffer + 1024, "%s\n\n%s", 
                            directories[currentgamedir].title, language[DRAWSCREEN_MARKNOTEXEC]);
                            DrawDialogOK(temp_buffer + 1024);
                            
                            copy_to_cache(currentgamedir, self_path);
    
                            sprintf(temp_buffer, "%s/cache/%s/%s", self_path, 
                            directories[currentgamedir].title_id, "/paths.dir");
                            if(stat(temp_buffer, &s)<0) return; // cannot launch without cache files
                        }
                     

                        use_cache = 1;
                    }
                    else
                    {
                        sprintf(temp_buffer, 
                            "%s\n\n%s", 
                            directories[get_currentdir(i)].title, language[DRAWSCREEN_MARKNOTEX4G]);
                        DrawDialogOK(temp_buffer);return;
                    }
                }
                
                if(game_cfg.exthdd0emu) {

                    if((directories[currentgamedir].flags & 2046) != 0) {
                        
                        for(n = 1; n < 11 ; n++) if(directories[currentgamedir].flags == (1 << n)) break;
                        sprintf(temp_buffer, "/dev_usb00%c/GAMEI", 47 + n);
                        mkdir(temp_buffer, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
                        add_sys8_path_table(self_path, self_path);
                        
                        if(game_cfg.bdemu) 
                            add_sys8_path_table("/dev_hdd0/game", "/dev_bdvd/GAMEI");
                        else
                            add_sys8_path_table("/dev_hdd0/game", temp_buffer);

                    } else if((fdevices & 2046) != 0){

                        for(n = 1; n < 11; n++) { // searching directory
                            if(fdevices & (1 << n)) {
                                DIR  *dir;
                                
                                sprintf(temp_buffer, "/dev_usb00%c/GAMEI", 47 + n);
                                dir = opendir (temp_buffer);
                                if(dir) {
                                    closedir (dir);
                                    
                                    add_sys8_path_table(self_path, self_path);
                                    add_sys8_path_table("/dev_hdd0/game", temp_buffer);
                                    break;
                                }
                            }
                        }
                        
                        if(n == 11) { // directory not found, Asking to create one
                        
                             for(n = 1; n < 11 ; n++) {
                                if(fdevices & (1 << n)) {
                                    sprintf(temp_buffer, "%s\n\n%s%c?"
                                            , language[DRAWSCREEN_GAMEINOFMNT], language[DRAWSCREEN_GAMEIASKDIR], 47 + n);
                                    if(DrawDialogYesNo(temp_buffer) == 1) {
                                        sprintf(temp_buffer, "/dev_usb00%c/GAMEI", 47 + n);
                                        mkdir(temp_buffer, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
                                        add_sys8_path_table(self_path, self_path);
                                        add_sys8_path_table("/dev_hdd0/game", temp_buffer);
                                        break;
                                    }
                                }
                             }
                             
                             if(n == 11) {
                                 sprintf(temp_buffer, "%s\n\n%s", language[DRAWSCREEN_GAMEICANTFD], language[DRAWSCREEN_GAMEIWLAUNCH]);
                                 if(DrawDialogYesNo(temp_buffer) != 1) return;
                             }
                        }
                    }

                    if((fdevices & 2046) == 0) {
                        sprintf(temp_buffer, "%s\n\n%s", language[DRAWSCREEN_GAMEICANTFD], language[DRAWSCREEN_GAMEIWLAUNCH]);
                        if(DrawDialogYesNo(temp_buffer) != 1) return;
                    }

                }

                if((game_cfg.xmb && (fdevices & 2048) == 0) || (game_cfg.direct_boot == 2))
                {
                    if(check_disc() == -1)
                        return;
                }
                
                if(!(directories[currentgamedir].flags & 2048))
                    param_sfo_util(directories[currentgamedir].path_name, (game_cfg.updates != 0));

                if(!game_cfg.ext_ebootbin) sys8_path_table(0LL);
                else {

                    sprintf(temp_buffer, "%s/self/%s.BIN", self_path, 
                        directories[get_currentdir(i)].title_id);

                    FILE *fp = fopen(temp_buffer, "rb");

                    if(!fp) {
                        sprintf(temp_buffer, " %s.BIN\n %s\n\n%s", 
                            directories[currentgamedir].title_id, language[DRAWSCREEN_EXTEXENOTFND], language[DRAWSCREEN_EXTEXENOTCPY]);
                        DrawDialogOK(temp_buffer);
                        goto skip_sys8;
                    } else {

                        fclose(fp);
                        add_sys8_path_table("/dev_bdvd/PS3_GAME/USRDIR/EBOOT.BIN", temp_buffer);
                    }
                
                }
                
                
                if(!game_cfg.xmb) sys8_sys_configure(CFG_XMB_DEBUG); else sys8_sys_configure(CFG_XMB_RETAIL);
                
                sys8_sys_configure(CFG_UNPATCH_APPVER + (game_cfg.updates != 0));
                
                if(game_cfg.bdemu) {
                    n = move_origin_to_bdemubackup(directories[currentgamedir].path_name);
                    if(n < 0)
                    {
                        syscall36("/dev_bdvd"); // in error exits
                        sys8_perm_mode((u64) 1);

                        if(game_cfg.ext_ebootbin)
                            build_sys8_path_table(); //prepare extern eboot

                        exit_program = 1; 
                        return;
                    }

                    if(n == 1) game_cfg.bdemu = 0; // if !dev_usb... bdemu is not usable
                }

                if(game_cfg.bdemu && 
                    patch_bdvdemu(directories[currentgamedir].flags) == 0) {

                    syscall36("//dev_bdvd"); // for hermes special flag see syscall36-3.41-hermes "//"
                }
                else {

                    if(use_cache) {
                       
                        sprintf(temp_buffer + 1024, "%s/cache/%s/%s", self_path,  //check replace (1024)
                        directories[currentgamedir].title_id, "/paths.dir");
                        
                        char *path = LoadFile(temp_buffer + 1024, &n);
                        char *mem = path;

                        n = n & ~2047; // if file truncated break bad datas...

                        if(path)
                        {
                            while(n > 0)
                            {
                                char *t = path;

                                sprintf(temp_buffer + 1024, "%s/cache/%s/%s", self_path, 
                                    directories[currentgamedir].title_id, path + 1024);
                                 
                                path = strstr(path, "PS3_GAME/");
                               
                                sprintf(temp_buffer, "/dev_bdvd/%s", path);

                                add_sys8_path_table(temp_buffer, temp_buffer + 1024);

                                path = t + 2048;
                                n   -= 2048;
                            }
                            free(mem);
                        }
                    }


                    syscall36(directories[currentgamedir].path_name);
                }

                build_sys8_path_table();
                sys8_perm_mode((u64) (game_cfg.perm & 3));
            
                exit_program = 1; 
                
                skip_sys8: 
                    return;
            }
        }
    }

    if(new_pad & BUTTON_START) {
        select_option = 0;
         menu_screen = 3; return;
    }

    if(new_pad & BUTTON_SELECT) {
        i = selected;

        select_option = 0;
        
        if(!Png_offset[i] && mode_favourites && mode_favourites < 65536 && favourites.list[i].title_id[0] != 0) {
            DeleteFavouritesIfExits(favourites.list[i].title_id);
            sprintf(temp_buffer, "%s/config/favourites.bin", self_path);
            SaveFavourites(temp_buffer);
            if(mode_favourites && !havefavourites) mode_favourites = 0;
            get_games();
            return;
        }

        if(Png_offset[i]) {
            
            Png_offset[12] = 0;

            if(!mode_favourites || (mode_favourites != 0 && favourites.list[i].index >= 0)) {
                sprintf(temp_buffer, "%s/PS3_GAME/PIC1.PNG", directories[get_currentdir(i)].path_name);
                
                if(LoadTexturePNG(temp_buffer, 12) < 0) {
                    sprintf(temp_buffer, "%s/PS3_GAME/PIC0.PNG", directories[(mode_favourites != 0) 
                        ? favourites.list[i].index : (currentdir + i)].path_name);
                    if(LoadTexturePNG(temp_buffer, 12) < 0) {
                        sprintf(temp_buffer, "%s/PS3_GAME/ICON0.PNG", directories[(mode_favourites != 0) 
                            ? favourites.list[i].index : (currentdir + i)].path_name);
                        if(LoadTexturePNG(temp_buffer, 12) < 0) Png_offset[12] = 0;
                    }
                }
            }
            
            currentgamedir = get_currentdir(i);
            if(currentgamedir >= 0 && currentgamedir < ndirectories) {
                menu_screen = 1; return;
            }
        }
    }

    static int auto_up = 0, auto_down = 0, auto_left = 0, auto_right = 0;

    AUTO_BUTTON_REP(auto_up, BUTTON_UP)
    AUTO_BUTTON_REP(auto_down, BUTTON_DOWN)
    AUTO_BUTTON_REP(auto_left, BUTTON_LEFT)
    AUTO_BUTTON_REP(auto_right, BUTTON_RIGHT)

    if(new_pad & BUTTON_UP) {
        select_py--;

        auto_up = 1;
        frame_count = 32;

        if(select_py < 0) {
            
            select_py = 2;
            
            if(mode_favourites >= 65536) ;
            else if(mode_favourites) {mode_favourites = 0; get_games();}
            else if(currentdir >= 12) {mode_favourites = 0; currentdir -= 12; get_games();}
            else {mode_favourites = (!mode_favourites && havefavourites); currentdir = ROUND_UP12(ndirectories) - 12; get_games();}
        }
        
        return;
    }

    if(new_pad & BUTTON_DOWN) {
        
        select_py++;

        auto_down = 1;
        frame_count = 32;
        
        if(select_py > 2) {
           
            select_py = 0; 
            
            if(mode_favourites >= 65536) ;
            else if(mode_favourites) {mode_favourites = 0; get_games();}
            else if(currentdir < (ROUND_UP12(ndirectories) - 12)) {mode_favourites = 0; currentdir += 12; get_games();}
            else {mode_favourites = (!mode_favourites && havefavourites); currentdir = 0; get_games();}
            
        }

        return;
    }

    if(new_pad & BUTTON_LEFT) {
        
        select_px--;

        auto_left = 1;
        frame_count = 32;

        if(select_px < 0) {
            
            select_px = 3;
            
            if(mode_favourites >= 65536) ;
            else if(mode_favourites) {mode_favourites = 0; get_games();}
            else if(currentdir >= 12) {mode_favourites = 0; currentdir -= 12; get_games();}
            else {mode_favourites = (!mode_favourites && havefavourites); currentdir = ROUND_UP12(ndirectories) - 12; get_games();}

        }

        return;
    }

    if(new_pad & BUTTON_RIGHT) {
        
        select_px++; 

        auto_right = 1;
        frame_count = 32;

        if(select_px > 3) {
            
            select_px = 0; 
            
            if(mode_favourites >= 65536) ;
            else if(mode_favourites) {mode_favourites = 0; get_games();}
            else if(currentdir < (ROUND_UP12(ndirectories) - 12)) {mode_favourites = 0; currentdir += 12; get_games();}
            else {mode_favourites = (!mode_favourites && havefavourites); currentdir = 0; get_games();}
        }

        return;
    }

    if(new_pad & BUTTON_L1) //change page
    {
        
        frame_count = 32;
            
        if(mode_favourites >= 65536) ;
        else if(mode_favourites) {mode_favourites = 0; get_games();}
        else if(currentdir >= 12) {mode_favourites = 0; currentdir -= 12; get_games();}
        else {mode_favourites = (!mode_favourites && havefavourites); currentdir = ROUND_UP12(ndirectories) - 12; get_games();}

        return;
    }
    
    if(new_pad & BUTTON_R1) //change page
    {
        //maybe wait some seconds here...
        frame_count = 32;
            
        if(mode_favourites >= 65536) ;
        else if(mode_favourites) {mode_favourites = 0; get_games();}
        else if(currentdir < (ROUND_UP12(ndirectories) - 12)) {mode_favourites = 0; currentdir += 12; get_games();}
        else {mode_favourites = (!mode_favourites && havefavourites); currentdir = 0; get_games();}

        return;
    }
    
}



void draw_options(float x, float y, int index)
{

    int i, n;

    float y2, x2;

    int copy_flag = 1;

    int selected = select_px + select_py * 4;
    

    SetCurrentFont(FONT_DEFAULT);

    // header title

    DrawBox(x, y, 0, 200 * 4 - 8, 20, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);
  
    DrawFormatString(x, y - 2, " %s", language[DRAWGMOPT_OPTS]);


    if(directories[currentgamedir].flags & 1) {

        copy_flag = 0;

        for(n = 1; n < 11; n++) {
            
            if((fdevices >> n) & 1) copy_flag = 1;
        }

    } else if(directories[currentgamedir].flags & 2046){
    
        copy_flag = 0;
        
        if((fdevices >> 0) & 1) copy_flag = 1; 
        
    }

    if(directories[currentgamedir].title_id[0] == 0 && select_option == 0) {
        select_option = 1;
        
        if(!copy_flag && select_option == 1) select_option++;

        if((directories[currentgamedir].flags & 2048) && select_option == 2) select_option++;

    }

    SetCurrentFont(FONT_BUTTON);
        
    SetFontSize(16, 20);

    utf8_to_ansi(directories[currentgamedir].title_id, temp_buffer, 64);
    temp_buffer[64] = 0;
    
    DrawFormatString(848 - x - strlen(temp_buffer) * 16 - 8, y, temp_buffer);

    y += 24;
    
    if(Png_offset[12]) {
               
        tiny3d_SetTextureWrap(0, Png_offset[12], Png_datas[12].width, 
        Png_datas[12].height, Png_datas[12].wpitch, 
            TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
        DrawTextBox(x, y, 0, 200 * 4 - 8, 150 * 3 - 8, 0xffffffff);
    }

    DrawBox(x, y, 0, 200 * 4 - 8, 150 * 3 - 8, 0x00000028);

    
    x2 = x;
    y2 = y + 32;
    
    DrawButton1(x + 32, y2, 320, language[DRAWGMOPT_CFGGAME], (directories[currentgamedir].title_id[0] == 0) ? -1 : (flash && select_option == 0));
    
    y2+= 48;

    DrawButton1(x + 32, y2, 320, language[DRAWGMOPT_CPYGAME], copy_flag ? (flash && select_option == 1) : -1);
    
    y2+= 48;

    DrawButton1(x + 32, y2, 320, language[DRAWGMOPT_DELGAME], (directories[currentgamedir].flags & 2048) ? -1  : ((flash && select_option == 2) ? 1 : 0));
    
    y2+= 48;

    DrawButton1(x + 32, y2, 320, language[DRAWGMOPT_FIXGAME], (flash && select_option == 3));
    
    y2+= 48;

    DrawButton1(x + 32, y2, 320, language[DRAWGMOPT_TSTGAME], (flash && select_option == 4));
    
    y2+= 48;

    DrawButton1(x + 32, y2, 320, language[DRAWGMOPT_CPYEBOOTGAME], (directories[currentgamedir].title_id[0] == 0) ? -1 : (flash && select_option == 5));
    
    y2+= 48;
    
    if(!TestFavouritesExits(directories[currentgamedir].title_id))
        DrawButton1(x + 32, y2, 320, language[DRAWGMOPT_CPYTOFAV], (flash && select_option == 6));
    else
        DrawButton1(x + 32, y2, 320, language[DRAWGMOPT_DELFMFAV], (flash && select_option == 6));
    
    y2+= 48;

    DrawButton1(x + 32, y2, 320, language[GLOBAL_RETURN], (flash && select_option == 7));
    
    y2+= 48;
    /*
    for(n = 0; n < 1; n++) {
        
        DrawButton1(x + 32, y2, 320, "", -1);
    
        y2+= 48;
    }
    */

    SetCurrentFont(FONT_DEFAULT);

    // draw game name

    DrawBox(x, y + 3 * 150, 0, 200 * 4 - 8, 40, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);

    i = selected;

    if(Png_offset[i]) {

        if(directories[currentgamedir].flags == (1<<11)) {
            utf8_to_ansi(bluray_game, temp_buffer, 65);
            SetFontColor(0x00ff00ff, 0x00000000);
        } else utf8_to_ansi(directories[currentgamedir].title, temp_buffer, 65);

        temp_buffer[65] = 0;

        if(strlen(temp_buffer) < 50) SetFontSize(22, 32); 
        else SetFontSize(18, 32);

        SetFontAutoCenter(1);
  
        DrawFormatString(0, y + 3 * 150, temp_buffer);

        SetFontAutoCenter(0);
    
    }
    
    tiny3d_Flip();

    ps3pad_read();
   
    if(new_pad & BUTTON_CROSS) {

        switch(select_option) {
            case 0:
                select_option = 0;
                menu_screen = 2; 

                // load game config
                sprintf(temp_buffer, "%s/config/%s.cfg", self_path, directories[currentgamedir].title_id);
                memset(&game_cfg, 0, sizeof(game_cfg));
                
                int file_size;
                char *file = LoadFile(temp_buffer, &file_size);
                if(file) {
                    if(file_size > sizeof(game_cfg)) file_size = sizeof(game_cfg);
                    memcpy(&game_cfg, file, file_size);
                    free(file);
                }
                return;

            case 1:
                 i = selected;

                 if(Png_offset[i]) {

                    pause_music(1);

                    copy_from_selection(currentgamedir);

                    pause_music(0);

                    currentgamedir = currentdir = 0;
                    select_px = select_py = 0;
                    select_option = 0;
                    menu_screen = 0;
                 }
                 return;

            case 2:
                 i = selected;

                 if(Png_offset[i]) {
                    
                    pause_music(1);
                    
                    delete_game(currentgamedir);
                    
                    pause_music(0);

                    currentgamedir = currentdir = 0;
                    select_px = select_py = 0;
                    select_option = 0;
                    menu_screen = 0;
                 }
                 return;
            case 3:
                 i = selected;

                 if(Png_offset[i]) {

                    pause_music(1);                

                    FixDirectory(directories[currentgamedir].path_name);

                    pause_music(0);

                    DrawDialogOK(language[DRAWGMOPT_FIXCOMPLETE]);
             
                 }
                 break;
            case 4:
                 i = selected;

                 if(Png_offset[i]) {

                    pause_music(1);

                    test_game(currentgamedir);

                    pause_music(0);
                    
                 }
                 break;
            case 5:
                {
                // load game config
                sprintf(temp_buffer, "/dev_usb/ps3game/%s.BIN", directories[currentgamedir].title_id);
                
                int file_size;
                char *file = LoadFile(temp_buffer, &file_size);
                if(file) {
                    sprintf(temp_buffer, "%s/self/%s.BIN", self_path, directories[currentgamedir].title_id);
                    if(SaveFile(temp_buffer, file, file_size)==0) {
                        sprintf(temp_buffer, "%s/self/%s.BIN\n\nEBOOT.BIN %s", self_path, directories[currentgamedir].title_id, language[DRAWGMOPT_CPYOK]);
                        DrawDialogOK(temp_buffer);
                    } else {
                        sprintf(temp_buffer, "%s/self/%s.BIN\n\n%s EBOOT.BIN", self_path, directories[currentgamedir].title_id, language[DRAWGMOPT_CPYERR]);
                        DrawDialogOK(temp_buffer);
                    }
                    free(file);
                } else {
                    sprintf(temp_buffer, "/dev_usb/ps3game/%s.BIN\n\nEBOOT.BIN %s", directories[currentgamedir].title_id, language[DRAWGMOPT_CPYNOTFND]);
                    DrawDialogOK(temp_buffer);
                }

                }
                break;
            
            case 6:
                if(TestFavouritesExits(directories[currentgamedir].title_id)) 
                    {
                        DeleteFavouritesIfExits(directories[currentgamedir].title_id);
                        sprintf(temp_buffer, "%s/config/favourites.bin", self_path);
                        SaveFavourites(temp_buffer);

                        if(mode_favourites && !havefavourites) {
                            mode_favourites = 0; get_games(); select_option = 0;
                            menu_screen = 0;
                            return;
                        }
                    
                        get_games();
                    }
                else {
                    mode_favourites = currentgamedir  | 65536;
                    
                    get_icon(path_name, currentgamedir);
                    if(LoadTexturePNG(path_name, 12) < 0) ;
                    get_games();
                    select_option = 0;
                    menu_screen = 0;
                    return;
                    
                }
                break;
            case 7:
                Png_offset[12] = 0;
                select_option = 0;
                menu_screen = 0;
                return;

            default:
               break;
        }
       // menu_screen = 0; return;
    }

    if(new_pad & BUTTON_CIRCLE) {
        Png_offset[12] = 0; menu_screen = 0; select_option = 0; return;
    }
   

    if(new_pad & BUTTON_UP) {
        select_option--; 

        frame_count = 32;

        if((directories[currentgamedir].flags & 2048) && select_option == 2) select_option--; 
        if(!copy_flag && select_option == 1) select_option--;

        if(directories[currentgamedir].title_id[0] == 0 && (select_option == 0 || select_option == 5)) select_option--;

        if(select_option < 0) {
            
            select_option = 7;  
          
        }
    }

    if(new_pad & BUTTON_DOWN) {
        
        select_option++;

        frame_count = 32;

        if(!copy_flag && select_option == 1) select_option++;

        if((directories[currentgamedir].flags & 2048) && select_option == 2) select_option++;
        
        if(select_option > 7) {
           
            select_option = 0;
           
        }

        if(directories[currentgamedir].title_id[0] == 0 && (select_option == 0 || select_option == 5)) {
            select_option++;
            
            if(!copy_flag && select_option == 1) select_option++;

            if((directories[currentgamedir].flags & 2048) && select_option == 2) select_option++; 

        }
    }

    
}


void draw_configs(float x, float y, int index)
{

    int i;

    float y2, x2;
   
    int selected = select_px + select_py * 4;

    SetCurrentFont(FONT_DEFAULT);

    // header title

    DrawBox(x, y, 0, 200 * 4 - 8, 20, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);
  
    DrawFormatString(x, y - 2, " %s", language[DRAWGMCFG_CFGS]);

    i = selected;

    SetCurrentFont(FONT_BUTTON);
        
    SetFontSize(16, 20);

    utf8_to_ansi(directories[currentgamedir].title_id, temp_buffer, 64);
    temp_buffer[64] = 0;
    
    DrawFormatString(848 - x - strlen(temp_buffer) * 16 - 8, y, temp_buffer);

    y += 24;
    

    if(Png_offset[12]) {
               
        tiny3d_SetTextureWrap(0, Png_offset[12], Png_datas[12].width, 
        Png_datas[12].height, Png_datas[12].wpitch, 
            TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
        DrawTextBox(x, y, 0, 200 * 4 - 8, 150 * 3 - 8, 0xffffffff);
    }

    DrawBox(x, y, 0, 200 * 4 - 8, 150 * 3 - 8, 0x00000028);

    
    x2 = x;
    y2 = y + 32;


#ifdef CONFIG_USE_SYS8PERMH4
    x2 = DrawButton1(x + 32, y2, 240, "Fix Permissions", (flash && select_option == 0)) + 16; // do no translate this (3.44)
    
    x2 = DrawButton2(x2, y2, 0, " Default ", (game_cfg.perm == 0) ) + 8;
    x2 = DrawButton2(x2, y2, 0, " PS jailbreak ", (game_cfg.perm == 1)) + 8;
    x2 = DrawButton2(x2, y2, 0, " v4 Perms (F1) ", (game_cfg.perm == 2)) + 8;

    y2+= 48;
#endif


#ifdef CONFIG_USE_SYS8CONFIG
    x2 = DrawButton1(x + 32, y2, 240, "Select XMB", (flash && select_option == 0))  + 16; // do no translate this (3.44 atm)
    x2 = DrawButton2(x2, y2, 0, " Debug ", (game_cfg.xmb == 0)) + 8;
    x2 = DrawButton2(x2, y2, 0, " Retail ", (game_cfg.xmb == 1)) + 8;
#else
    x2 = DrawButton1(x + 32, y2, 240, language[DRAWGMCFG_DSK], (flash && select_option == 0))  + 16;
    x2 = DrawButton2(x2, y2, 0, language[DRAWGMCFG_NO] , (game_cfg.xmb == 0)) + 8;
    x2 = DrawButton2(x2, y2, 0, language[DRAWGMCFG_YES], (game_cfg.xmb == 1)) + 8;
#endif
    y2+= 48;

    #if 0
    x2 = DrawButton1(x + 32, y2, 240, language[DRAWGMCFG_UPD], (flash && select_option == 1))  + 16;
        
    x2 = DrawButton2(x2, y2, 0, language[DRAWGMCFG_ON] , /*(game_cfg.updates == 0)*/ -1) + 8;
    x2 = DrawButton2(x2, y2, 0, language[DRAWGMCFG_OFF], /*(game_cfg.updates != 0)*/ 1) + 8;

    y2+= 48;
    #endif

    x2 = DrawButton1(x + 32, y2, 240, "Direct Boot", (flash && select_option == 1)) + 16;
    
    x2 = DrawButton2(x2, y2, 0, language[DRAWGMCFG_NO], (game_cfg.direct_boot == 0) ) + 8;
    x2 = DrawButton2(x2, y2, 0, language[DRAWGMCFG_YES], (game_cfg.direct_boot == 1)) + 8;
    x2 = DrawButton2(x2, y2, 0, "With BR", (game_cfg.direct_boot == 2)) + 8;

    y2+= 48;

    x2 = DrawButton1(x + 32, y2, 240, language[DRAWGMCFG_EXTBOOT], (flash && select_option == 2))  + 16;
        
    x2 = DrawButton2(x2, y2, 0, language[DRAWGMCFG_ON] , (payload_mode >= ZERO_PAYLOAD) ? (game_cfg.ext_ebootbin != 0) : -1 ) + 8;
    x2 = DrawButton2(x2, y2, 0, language[DRAWGMCFG_OFF], (game_cfg.ext_ebootbin == 0)) + 8;

    y2+= 48;

    x2 = DrawButton1(x + 32, y2, 240, language[DRAWGMCFG_BDEMU], (flash && select_option == 3))  + 16;
        
    x2 = DrawButton2(x2, y2, 0, language[DRAWGMCFG_ON] , (directories[currentgamedir].splitted) ? -1: (game_cfg.bdemu != 0)) + 8;
    x2 = DrawButton2(x2, y2, 0, language[DRAWGMCFG_OFF], (game_cfg.bdemu == 0)) + 8;

    y2+= 48;

    x2 = DrawButton1(x + 32, y2, 240, language[DRAWGMCFG_EXTHDD0GAME], (flash && select_option == 4))  + 16;
        
    x2 = DrawButton2(x2, y2, 0, language[DRAWGMCFG_ON] , (payload_mode >= ZERO_PAYLOAD) ? (game_cfg.exthdd0emu != 0): -1) + 8;
    x2 = DrawButton2(x2, y2, 0, language[DRAWGMCFG_OFF], (game_cfg.exthdd0emu == 0)) + 8;

    y2+= 48;

    x2 = DrawButton1(x + 32, y2, 240, language[DRAWGMCFG_SAVECFG], (flash && select_option == 5))  + 16;
    y2+= 48;

    x2 = DrawButton1(x + 32, y2, 240, language[GLOBAL_RETURN], (flash && select_option == 6))  + 16;
    y2+= 48;


    SetCurrentFont(FONT_DEFAULT);

    // draw game name

    i = selected;

    DrawBox(x, y + 3 * 150, 0, 200 * 4 - 8, 40, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);

    if(Png_offset[i]) {

        if(directories[currentgamedir].flags == (1<<11)) {
            utf8_to_ansi(bluray_game, temp_buffer, 65);
            SetFontColor(0x00ff00ff, 0x00000000);
        } else utf8_to_ansi(directories[currentgamedir].title, temp_buffer, 65);

        temp_buffer[65] = 0;

        if(strlen(temp_buffer) < 50) SetFontSize(22, 32); 
        else SetFontSize(18, 32);

        SetFontAutoCenter(1);
  
        
        DrawFormatString(0, y + 3 * 150, temp_buffer);

        SetFontAutoCenter(0);
    }
   

    tiny3d_Flip();

    ps3pad_read();


    if(new_pad & BUTTON_CIRCLE) {
        menu_screen = 1; select_option = 0; return;
    }

   
    if(new_pad & BUTTON_CROSS) {
     
        switch(select_option) {
            //removed sys8 calls not supported yet on 3.55
#ifdef CONFIG_USE_SYS8PERMH4
            case 0:
                ROT_INC(game_cfg.perm, 2, 0);
                break;
#endif
            case 0:
                ROT_INC(game_cfg.xmb, 1, 0);
                break;
            case 1:
                ROT_INC(game_cfg.direct_boot, 2, 0);
                break;
            case 2:
                if(payload_mode >= ZERO_PAYLOAD)
                    ROT_INC(game_cfg.ext_ebootbin, 1, 0);
                break;
            case 3:
                if(!directories[currentgamedir].splitted)
                    ROT_INC(game_cfg.bdemu, 1, 0);
                break;
            case 4:
                if(payload_mode >= ZERO_PAYLOAD)
                    ROT_INC(game_cfg.exthdd0emu, 1, 0);
                break;
            case 5:
                // save game config
                sprintf(temp_buffer, "%s/config/%s.cfg", self_path, directories[currentgamedir].title_id);
              
                
                if(SaveFile(temp_buffer, (char *) &game_cfg, sizeof(game_cfg)) == 0) {
                    sprintf(temp_buffer, "%s.cfg\n\n%s", directories[currentgamedir].title_id, language[GLOBAL_SAVED]);
                    DrawDialogOK(temp_buffer);
                }

                break;
            default:
                menu_screen = 1; select_option = 0; return;
                break;
        }
     }

    if(new_pad & BUTTON_UP) {

        frame_count = 32;

        ROT_DEC(select_option, 0, 6)
        
    }

    if(new_pad & BUTTON_DOWN) {
        
        frame_count = 32;

        ROT_INC(select_option, 6, 0); 
        
    }

    if(new_pad & BUTTON_LEFT) {
     
        switch(select_option) {
            //removed sys8 calls not supported yet on 3.55
#ifdef CONFIG_USE_SYS8PERMH4
            case 0:
                ROT_DEC(game_cfg.perm, 0, 2);
                break;
#endif
            case 0:
                ROT_DEC(game_cfg.xmb, 0, 1);
                break;
            case 1:
                ROT_DEC(game_cfg.direct_boot, 0, 2);
                break;
            case 2:
                if(payload_mode >= ZERO_PAYLOAD)
                    ROT_DEC(game_cfg.ext_ebootbin, 0, 1);
                break;
            case 3:
                if((!directories[currentgamedir].splitted))
                    ROT_DEC(game_cfg.bdemu, 0, 1);
                break;
             case 4:
                if(payload_mode >= ZERO_PAYLOAD)
                    ROT_DEC(game_cfg.exthdd0emu, 0, 1);
                break;
            default:
                break;
        }
     }

     if(new_pad & BUTTON_RIGHT) {
     
        switch(select_option) {
            //removed sys8 calls not supported yet on 3.55
#ifdef CONFIG_USE_SYS8PERMH4
            case 0:
                ROT_INC(game_cfg.perm, 2, 0);
                break;
#endif
            case 0:
                ROT_INC(game_cfg.xmb, 1, 0);
                break;
            case 1:
                ROT_INC(game_cfg.direct_boot, 2, 0);
                break;
            case 2:
                if(payload_mode >= ZERO_PAYLOAD)
                    ROT_INC(game_cfg.ext_ebootbin, 1, 0);
                break;
            case 3:
                if(!directories[currentgamedir].splitted)
                    ROT_INC(game_cfg.bdemu, 1, 0);
                break;
            case 4:
                if(payload_mode >= ZERO_PAYLOAD)
                    ROT_INC(game_cfg.exthdd0emu, 1, 0);
                break;
            default:
                break;
        }
     }

}

void draw_gbloptions(float x, float y)
{

    float y2, x2;
    static float x3 = -1;
    
    SetCurrentFont(FONT_DEFAULT);

    // header title

    DrawBox(x, y, 0, 200 * 4 - 8, 20, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);
  
    DrawFormatString(x, y - 2, " %s", language[DRAWGLOPT_OPTS]);

    if(x3 < 0)
    {
        x3 = 2000;
        x3 = DrawFormatString(x3, y - 2,  music[song_selected + MAX_SONGS]); // calculate first time
        x3 = 848 -(x3 - 2000) - x;
    }else
        DrawFormatString(x3, y - 2,  music[song_selected + MAX_SONGS]); //print current song name
    
    y += 24;

    DrawBox(x, y, 0, 200 * 4 - 8, 150 * 3 - 8, 0x00000028);

    
    x2 = x;
    y2 = y + 32;
    
    DrawButton1((848 - 520) / 2, y2, 520, language[DRAWGLOPT_SCRADJUST], (flash && select_option == 0));
    
    y2+= 48;

    DrawButton1((848 - 520) / 2, y2, 520, language[DRAWGLOPT_CHANGEDIR], (flash && select_option == 1));
    
    y2+= 48;

    DrawButton1((848 - 520) / 2, y2, 520, language[DRAWGLOPT_CHANGEBCK], (flash && select_option == 2));
    
    y2+= 48;

    DrawButton1((848 - 520) / 2, y2, 520, (manager_cfg.opt_flags & OPTFLAGS_PLAYMUSIC)? language[DRAWGLOPT_SWMUSICOFF] : language[DRAWGLOPT_SWMUSICON] , (flash && select_option == 3));
    
    y2+= 48;

    DrawButton1((848 - 520) / 2, y2, 520, (ftp_ip_str[0]) ? ftp_ip_str : language[DRAWGLOPT_INITFTP], (flash && select_option == 4));
    
    y2+= 48;

    DrawButton1((848 - 520) / 2, y2, 520, language[DRAWGLOPT_TOOLS], (flash && select_option == 5));
    
    y2+= 48;

    DrawButton1((848 - 520) / 2, y2, 520, language[GLOBAL_RETURN], (flash && select_option == 6));
    
    y2+= 48;
    
    DrawButton1((848 - 520) / 2, y2, 520, language[DRAWGLOPT_CREDITS], (flash && select_option == 7));
   
    y2+= 48;

    DrawBox(x, y + 3 * 150, 0, 200 * 4 - 8, 40, 0x00000028);

    // draw sys version
    SetCurrentFont(FONT_DEFAULT);

    SetFontColor(0xccccffff, 0x00000000);
    SetFontSize(18, 20);
    SetFontAutoCenter(1);
    DrawFormatString(0, y2 + 40, payload_str );

    tiny3d_Flip();

    ps3pad_read();

    if(new_pad & BUTTON_CROSS) {
    
        switch(select_option) {
            case 0:
                video_adjust();
                select_option = 0;
                menu_screen = 0; 
                return;

            case 1:
                menu_screen = 0;
                Select_games_folder();
     
                if(manager_cfg.hdd_folder[0] == 0) strcpy(manager_cfg.hdd_folder, __MKDEF_MANAGER_DIR__);
                SaveManagerCfg();
                currentgamedir = currentdir = 0;
                select_px = select_py = 0;
                select_option = 0;
                menu_screen = 0;
                
                ndirectories = 0;
                fdevices=0;
                fdevices_old=0;
                forcedevices=0;
                find_device=0;

                return;

            case 2:
                background_sel++;
                background_sel &= 3;
                manager_cfg.background_sel = background_sel;
                SaveManagerCfg();
                break;

            case 3:
                manager_cfg.opt_flags ^= OPTFLAGS_PLAYMUSIC; //change bit
                pause_music((manager_cfg.opt_flags & OPTFLAGS_PLAYMUSIC)? 0 : 1);
                SaveManagerCfg();
                break;

            case 4:
                if ((manager_cfg.opt_flags & OPTFLAGS_FTP) == 0)
                {
                    if(ftp_init() == 0)
                    {
                        if(DrawDialogYesNo(language[DRAWGLOPT_FTPINITED]) != 1)
                            break;
                    }
                    else
                    {
                        DrawDialogOK(language[DRAWGLOPT_FTPARINITED]);
                        break;
                    }
                }
                else
                {
                        DrawDialogOK(language[DRAWGLOPT_FTPSTOPED]);
                        ftp_deinit();
                }
                manager_cfg.opt_flags ^= OPTFLAGS_FTP;
                SaveManagerCfg();
                break;

            case 5:
                select_option = 0;
                menu_screen = 4; 
                return;

            case 6:
                select_option = 0;
                menu_screen = 0; 
                return;

            case 7:
                   DrawDialogOK(credits_str1);
                   DrawDialogOK(credits_str2);
                   DrawDialogOK(credits_str3);
                   break;

            default:
               break;
        }
    
    }

    if(new_pad & BUTTON_CIRCLE) {
        menu_screen = 0; return;
    }
   

    if(new_pad & BUTTON_UP) {

        frame_count = 32;

        ROT_DEC(select_option, 0, 7)
        
    }

    if(new_pad & BUTTON_DOWN) {

        frame_count = 32;
        
        ROT_INC(select_option, 7, 0); 
        
    }
    
}


void draw_toolsoptions(float x, float y)
{

    int n;

    float y2, x2;
    

    SetCurrentFont(FONT_DEFAULT);

    // header title

    DrawBox(x, y, 0, 200 * 4 - 8, 20, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);
  
    DrawFormatString(x, y - 2, " %s", language[DRAWTOOLS_TOOLS]);

    y += 24;

    DrawBox(x, y, 0, 200 * 4 - 8, 150 * 3 - 8, 0x00000028);

    
    x2 = x;
    y2 = y + 32;
    
    DrawButton1((848 - 520) / 2, y2, 520, language[DRAWTOOLS_DELCACHE], (flash && select_option == 0));
    
    y2+= 48;

    if(manager_cfg.usekey)
        DrawButton1((848 - 520) / 2, y2, 520, language[DRAWTOOLS_SECDISABLE], (flash && select_option == 1));
    else
        DrawButton1((848 - 520) / 2, y2, 520, language[DRAWTOOLS_SECENABLE], (flash && select_option == 1));
    
    y2+= 48;

    DrawButton1((848 - 520) / 2, y2, 520, language[DRAWTOOLS_LOADX], (flash && select_option == 2));
    
    y2+= 48;

    DrawButton1((848 - 520) / 2, y2, 520, language[GLOBAL_RETURN], (flash && select_option == 3));
    
    y2+= 48;
    
    for(n = 0; n < 4; n++) {
        
        DrawButton1((848 - 520) / 2, y2, 520, "", -1);
    
        y2+= 48;
    }


    SetCurrentFont(FONT_DEFAULT);

    // draw game name

    DrawBox(x, y + 3 * 150, 0, 200 * 4 - 8, 40, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);


    tiny3d_Flip();

    ps3pad_read();

    if(new_pad & BUTTON_CROSS) {
    
        switch(select_option) {
            case 0:
               
                LoadCacheDatas();
                // draw_cachesel

                if(ncache_list > 0) {
                    menu_screen = 5;
                    select_option = 0;
                }

                return;
            case 1:
                manager_cfg.usekey = manager_cfg.usekey == 0;
                SaveManagerCfg();
                break;

            case 2:
                load_ps3loadx = 1;
                exit(0);
                break;

            case 3:
                select_option = 0;
                menu_screen = 0; 
                return;

            default:
               break;
        }
    
    }

    if(new_pad & BUTTON_CIRCLE) {
        menu_screen = 0; return;
    }
   

    if(new_pad & BUTTON_UP) {

        frame_count = 32;

        ROT_DEC(select_option, 0, 3)
        
    }

    if(new_pad & BUTTON_DOWN) {

        frame_count = 32;
        
        ROT_INC(select_option, 3, 0); 
        
    }
    
}

void draw_cache_external()
{
    int menu = menu_screen;

    LoadCacheDatas();
  
    if(ncache_list > 0) {
        menu_screen = 4;
        select_option = 0;
    } else return;

    while(menu_screen != 0) {
        flash = (frame_count >> 5) & 1;

        frame_count++;
        cls();

        update_twat();
        menu_screen = 5;
        draw_cachesel(28, 0);
    }

    menu_screen = menu;
}

void draw_cachesel(float x, float y)
{

    int n;

    float y2, x2;
    

    SetCurrentFont(FONT_DEFAULT);

    // header title

    DrawBox(x, y, 0, 200 * 4 - 8, 20, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);
  
    DrawFormatString(x, y - 2, " %s", language[DRAWCACHE_CACHE]);
    
    x2= DrawFormatString(2000, -2, "hdd0: %.2fGB ", freeSpace[0]);
    x2 = 848 -(x2 - 2000) - x;
    DrawFormatString(x2, -2, "hdd0: %.2fGB ", freeSpace[0]);


    y += 24;

    DrawBox(x, y, 0, 200 * 4 - 8, 150 * 3 - 8, 0x00000028);

    
    x2 = x;
    y2 = y + 32;

    for(n = (select_option / 8) * 8; (n < (select_option / 8) * 8 + 8); n++) {
        if(n < ncache_list) {
            sprintf(temp_buffer, "%s (%1.2f GB)", cache_list[n].title_id, ((double) cache_list[n].size)/(1024.0*1024.*1024.0));
            DrawButton1((848 - 520) / 2, y2, 520, temp_buffer, (flash && select_option == n));
        } else DrawButton1((848 - 520) / 2, y2, 520, "", -1);
    
        y2+= 48;
    }

    SetCurrentFont(FONT_DEFAULT);

    // draw game name

    DrawBox(x, y + 3 * 150, 0, 200 * 4 - 8, 40, 0x00000028);

    
    if(flash && cache_need_free != 0) {
        SetCurrentFont(FONT_DEFAULT);
        SetFontSize(20, 20);
        SetFontColor(0xffff00ff, 0x00000000);
        SetFontAutoCenter(1);
        DrawFormatString(0, y + 3 * 150 + 6, language[DRAWCACHE_ERRNEEDIT], cache_need_free);
        SetFontAutoCenter(0);

    } else if(select_option < ncache_list){
    

        SetFontColor(0xffffffff, 0x00000000);

        utf8_to_ansi(cache_list[select_option].title, temp_buffer, 65);

        temp_buffer[65] = 0;

        if(strlen(temp_buffer) < 50) SetFontSize(18, 32); 
        else SetFontSize(14, 32);

        SetFontAutoCenter(1);
  
        DrawFormatString(0, y + 3 * 150, temp_buffer);

        SetFontAutoCenter(0);
    
    
    }
    SetFontColor(0xffffffff, 0x00000000);


    tiny3d_Flip();

    ps3pad_read();

    if(new_pad & BUTTON_CROSS) {
    
        if(select_option >= ncache_list) return;

        sprintf(temp_buffer, language[DRAWCACHE_ASKTODEL], cache_list[select_option].title_id);

        if(DrawDialogYesNo(temp_buffer) == 1) {
           
            sprintf(temp_buffer, "%s/cache/%s", self_path, cache_list[select_option].title_id);
            DeleteDirectory(temp_buffer);
            rmdir(temp_buffer);
            LoadCacheDatas();

            if(ncache_list >= select_option) select_option = ncache_list - 1;

            if(ncache_list <= 0) {
                select_option = 0;
                menu_screen = 0; 
            }
        }
        new_pad = 0;
        return;
    }

    if(new_pad & BUTTON_CIRCLE) {
        select_option = 0;
        menu_screen = 0; return;
    }
   

    if(new_pad & BUTTON_UP) {

        frame_count = 32;

        ROT_DEC(select_option, 0, ncache_list - 1)
        
    }

    if(new_pad & BUTTON_DOWN) {
        
        frame_count = 32;

        ROT_INC(select_option, ncache_list - 1, 0); 
        
    }
    
}

/******************************************************************************************************************************************************/
/* BDVDEMU FUNCTIONS                                                                                                                                  */
/******************************************************************************************************************************************************/

#define LV2MOUNTADDR_341 0x80000000003EE504ULL

void unpatch_bdvdemu()
{
//LV2 Mount for 355 in his payload code
#if 0
    int n;
    int flag = 0, flag2 = 0;
 
    char * mem = temp_buffer;
    memset(mem, 0, 0xff0);

    sys8_memcpy((u64) mem, LV2MOUNTADDR_341, 0xff0ULL);

    for(n = 0; n< 0xff0; n+= 0x100) {

        if(!memcmp(mem + n, "CELL_FS_IOS:PATA0_BDVD_DRIVE", 29))
        {
            if(!memcmp(mem + n + 0x69, "temp_bdvd", 10))
            {
                sys8_memcpy(LV2MOUNTADDR_341 + n + 0x69, (u64) "dev_bdvd\0", 10ULL);
                flag++;
            }  
        }

        if(!memcmp(mem + n, "CELL_FS_IOS:USB_MASS_STORAGE0", 29)) {
            if(!memcmp(mem + n + 0x69, "dev_bdvd", 9)) 
            {
                sys8_memcpy(LV2MOUNTADDR_341 + n + 0x69, (u64) (mem + n + 0x79), 11ULL);
                sys8_memset(LV2MOUNTADDR_341 + n + 0x79, 0ULL, 12ULL);
                flag2++;
            }
            
        }
      
    }
#endif
    int flag = 0;
    flag = lv2_unpatch_bdvdemu();

#if 0
    cls();
    SetFontSize(18, 20);
       
    SetFontColor(0xffffffff, 0x00000000);
    SetFontAutoCenter(0);

    DrawFormatString(16, 32, "unpatched (%i) mem(%c%c%c)", flag, temp_buffer[0], temp_buffer[1], temp_buffer[2]);

    tiny3d_Flip();

    sleep(4);
#endif

}


int patch_bdvdemu(u32 flags)
{
    int n;
    int usb = -1;

    for(n = 1; n < 11; n++) {
        if(flags == (1 << n)) {usb = n - 1; break;}
    }

    if(usb < 0) {
        DrawDialogOK(language[PATCHBEMU_ERRNOUSB]);
        return -1;
    }

#if 0
    int flag = 0, flag2 = 0;
    char * mem = temp_buffer;

    sys8_memcpy((u64) mem, LV2MOUNTADDR_341, 0xff0);

    sprintf(path_name, "CELL_FS_IOS:USB_MASS_STORAGE00%c", 48 + usb);
    sprintf(&path_name[128], "dev_usb00%c", 48 + usb);

    for(n = 0; n< 0xff0; n+= 0x100) {

        if(!memcmp(mem + n, "CELL_FS_IOS:PATA0_BDVD_DRIVE", 29)) {
    
            sys8_memcpy(LV2MOUNTADDR_341 + n + 0x69, (u64) "temp_bdvd", 10ULL);
            flag++;
        }

        if(!memcmp(mem + n, path_name, 32)) {
           
            sys8_memcpy(LV2MOUNTADDR_341 + n + 0x69, (u64) "dev_bdvd\0\0", 11ULL);
            sys8_memcpy(LV2MOUNTADDR_341 + n + 0x79, (u64) &path_name[128], 11ULL);
            
            flag2++;
        }
      
    }
#endif

    int flag = 0;
    flag = lv2_patch_bdvdemu(flags);

#if 0
    cls();
    SetFontSize(18, 20);
       
    SetFontColor(0xffffffff, 0x00000000);
    SetFontAutoCenter(0);

    DrawFormatString(16, 32, "patched %i mem(%c%c%c)", flag, temp_buffer[0], temp_buffer[1], temp_buffer[2]);

    tiny3d_Flip();

    sleep(4);

    if(flag < 10) return -1;
#endif

    return 0;
}

int move_origin_to_bdemubackup(char *path)
{
    if(strncmp(path, "/dev_usb00", 10)) return 1;
    
    sprintf(temp_buffer, "%s/PS3_GAME/PS3PATH.BUP", path);
    sprintf(temp_buffer + 1024, "%s/PS3_GAME", path);

    if(SaveFile(temp_buffer, temp_buffer + 1024, strlen(temp_buffer + 1024))!=0) {
        
        sprintf(temp_buffer + 1024, language[MOVEOBEMU_ERRSAVE], temp_buffer);
        DrawDialogOK(temp_buffer + 1024);
        
        return -1;
    }
    
    strncpy(temp_buffer + 11, "/PS3_GAME", 16);

    if(lv2FsRename(temp_buffer  + 1024, temp_buffer) != 0)  {
        
        sprintf(temp_buffer + 256, language[MOVEOBEMU_ERRMOVE], temp_buffer);
        DrawDialogOK(temp_buffer + 256);

        return -1;
     }
    
    sprintf(temp_buffer + 256, language[MOVEOBEMU_MOUNTOK], temp_buffer);
    DrawDialogOK(temp_buffer + 256);

    return 0;
}

int move_bdemubackup_to_origin(u32 flags)
{
    int n;
    int usb = -1;

    static u32 olderrflags = 0;

    for(n = 1; n < 11; n++) {
        if(flags == (1 << n)) {usb = n - 1; break;}
    }

    if(usb < 0) return -1;

    sprintf(temp_buffer, "/dev_usb00%c/PS3_GAME", 48 + usb);

    sprintf(temp_buffer + 256, "/dev_usb00%c/PS3_GAME/PS3PATH.BUP", 48 + usb);

    int file_size;
    char *file;

    file = LoadFile(temp_buffer + 256, &file_size);
    
    if(!file) return -1;

    memset(temp_buffer + 1024, 0, 0x420);
    
    if(file_size > 0x400) file_size = 0x400;

    memcpy(temp_buffer + 1024, file, file_size);

    free(file);

    for(n=0; n< 0x400; n++) {
        if(temp_buffer[1024 + n] == 0) break;
        if(((u8)temp_buffer[1024 + n]) < 32) {temp_buffer[1024 + n] = 0; break;}
    }
    
    
    if(strncmp(temp_buffer, temp_buffer + 1024, 10))  return -1; // if not /dev_usb00x return

    memcpy(temp_buffer + 1024, temp_buffer, 11); 

    if(lv2FsRename(temp_buffer, temp_buffer + 1024) != 0)  {
        if(!(olderrflags & flags)) {
            sprintf(temp_buffer, language[MOVETBEMU_ERRMOVE], temp_buffer + 1024);
            DrawDialogOK(temp_buffer);
            olderrflags |= flags;
        }
        return -1;
    }

    return 0;
}

