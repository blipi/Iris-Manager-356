/* 
    (c) 2011 Hermes <www.elotrolado.net>

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

#include "syscall8.h"

#ifdef WITH_CFW355
#include "payload355/payload.h"
#endif

#include "spu_soundmodule.bin.h" // load SPU Module
#include "spu_soundlib.h"

#include <gcmodplay.h>
#include "beauty_mod.bin.h"

// SPU
u32 spu = 0;
sysSpuImage spu_image;
#define SPU_SIZE(x) (((x)+127) & ~127)

MODPlay mod_track;

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

#include "andika_ttf.bin.h"

// font 2: 224 chr from 32 to 255, 16 x 32 pix 2 bit depth
#include "font_b.h"

#define ROT_INC(x ,y , z) {x++; if(x > y) x = z;}
#define ROT_DEC(x ,y , z) {x--; if(x < y) x = z;}

int menu_screen = 0;

t_directories directories[MAX_DIRECTORIES];

int ndirectories = 0;

int currentdir = 0;
int currentgamedir = 0;

u8 * png_texture = NULL;
PngDatas Png_datas[16];
u32 Png_offset[16];

PngDatas Png_res[16];
u32 Png_res_offset[16];

#include "bluray_png.bin.h"
#include "usb_png.bin.h"

void Load_PNG_resources()
{
    int i;

    for(i = 0; i < 16; i++) Png_res[i].png_in = NULL;
    // datas for PNG from memory

    Png_res[0].png_in   = (void *) bluray_png_bin;
    Png_res[0].png_size = sizeof  (bluray_png_bin);

    Png_res[1].png_in   = (void *) usb_png_bin;
    Png_res[1].png_size = sizeof  (usb_png_bin);

   
    
    // load PNG from memory

    for(i = 0; i < 16; i++)
        if(Png_res[i].png_in != NULL)
            LoadPNG(&Png_res[i], NULL);

}

int LoadTexturePNG(char * filename, int index)
{
    
    u32 * texture_pointer2 = (u32 *) (png_texture + index * 2048 * 1024); // 4 MB reserved for PNG index

    // here you can add more textures using 'texture_pointer'. It is returned aligned to 16 bytes
   
	LoadPNG(&Png_datas[index], filename);
    free(Png_datas[index].png_in);

    Png_offset[index] = 0;
       
    if(Png_datas[index].bmp_out) {

        memcpy(texture_pointer2, Png_datas[index].bmp_out, Png_datas[index].wpitch * Png_datas[index].height);
        
        free(Png_datas[index].bmp_out);

        Png_datas[index].bmp_out= texture_pointer2;

        Png_offset[index] = tiny3d_TextureOffset(Png_datas[index].bmp_out);      // get the offset (RSX use offset instead address)

     return 0;
     }

    return -1;
}



void get_games() 
{
    int n;

    for(n = 0; n < 12; n++) {
        if(currentdir + n < ndirectories) {
            
            char path_name[MAXPATHLEN+1];

            sprintf(path_name, "%s/PS3_GAME/ICON0.PNG", directories[currentdir + n].path_name);

            if(LoadTexturePNG(path_name, n) < 0) Png_offset[n] = 0;
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

    u32 * texture_mem = tiny3d_AllocTexture(96*1024*1024); // alloc 96MB of space for textures (this pointer can be global)    

    u32 * texture_pointer; // use to asign texture space without changes texture_mem

    if(!texture_mem) return; // fail!

    texture_pointer = texture_mem;

    ResetFont();

    TTFLoadFont(NULL, (void *) andika_ttf_bin, sizeof(andika_ttf_bin));
    texture_pointer = (u32 *) AddFontFromTTF((u8 *) texture_pointer, 32, 255, 32, 32, TTF_to_Bitmap);
    texture_pointer = (u32 *) AddFontFromTTF((u8 *) texture_pointer, 32, 255, 64, 64, TTF_to_Bitmap);
    TTFUnloadFont();

    texture_pointer = (u32 *) AddFontFromBitmapArray((u8 *) font_b, (u8 *) texture_pointer, 32, 255, 16, 32, 2, BIT0_FIRST_PIXEL);

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
    0xff0040ff,
    0xff606060,
    0xff80804f,
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

#ifndef WITH_CFW355
int send_payload_code()
{
    int l, n;

    u64 v = lv2peek(0x8000000000017CD0ULL);
        
    if(v != 0x4E8000203C608001ULL) return 1; // if syscall 8 is modified payload is resident...

    if(sizeof (payload_groove_hermes_bin) > 0xfff) return 2; // payload too big...

    /* i repeat 25 times the patch because it is possible fail patching (cache instruction problem? maybe only in syscall 9 patch
    because the call of syscall 7) */
    
    for(l = 0; l < 25; l++) {
        u8 * p = (u8 *) &payload_groove_hermes_bin[0];

        
        for(n = 0; n < sizeof (payload_groove_hermes_bin); n += 8) {
        
            static u64 value;

            memcpy(&value, &p[n], 8);

            lv2poke(0x80000000007e0000ULL + (u64) n, value);

            __asm__("sync");

            value =  lv2peek(0x8000000000000000ULL);

            }
        
        // syscall 9 enable
        lv2poke(0x8000000000017CE0ULL , 0x7C6903A64E800420ULL);
        __asm__("sync");
    }


    return 0;
}
#endif

typedef struct {

	path_open_entry entries[2];

	char arena[0x420*2];

} path_open_table;

char self_path[MAXPATHLEN]= "/dev_hdd0/game/HMANAGER4";


u64 hmanager_key = 0x1759829723742374ULL;

// manager

char temp_buffer[2048];

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

#define ROUND_UP12(x) (((x)+11)/12*12)

void draw_screen1(float x, float y);
void draw_options(float x, float y, int index);
void draw_configs(float x, float y, int index);
void draw_gbloptions(float x, float y);

struct {
    int videoscale_x[4];
    int videoscale_y[4];
    int background_sel;
    char hdd_folder[64];
    u32 usekey;
    char pad[156];
} manager_cfg;

struct {
    int version;
    int perm;
    int xmb;
    int updates;
    int ext_ebootbin;
    int pad[9];
} game_cfg;

int load_ps3loadx = 0;


int inited = 0;

#define INITED_SPU          2
#define INITED_SOUNDLIB     4

void fun_exit()
{
    ftp_deinit();

    if(inited & INITED_SOUNDLIB) {
        
        MODPlay_Unload (&mod_track);
        SND_End();
    }

    if(inited & INITED_SPU) {
        sleep(1);
        lv2SpuRawDestroy(spu);
        sysSpuImageClose(&spu_image);
    }

    SysUnloadModule(SYSMODULE_GCM_SYS);
    SysUnloadModule(SYSMODULE_IO);
    SysUnloadModule(SYSMODULE_PNGDEC);
    SysUnloadModule(SYSMODULE_FS);

    if(manager_cfg.usekey) sys8_disable(hmanager_key);
    
    if(load_ps3loadx) {
        sysProcessExitSpawn2("/dev_hdd0/game/PSL145310/RELOAD.SELF", NULL, NULL, NULL, 0, 1001, SYS_PROCESS_SPAWN_STACK_SIZE_1M);
        load_ps3loadx = 0;
    }

}

void LoadManagerCfg()
{

    sprintf(temp_buffer, "%s/config/manager_setup.bin", self_path);

    int file_size;
    char *file = LoadFile(temp_buffer, &file_size);

    if(file) {
        if(file_size > sizeof(manager_cfg)) file_size = sizeof(manager_cfg);
        memcpy(&manager_cfg, file, file_size);
        free(file);
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
}

int SaveManagerCfg()
{
    sprintf(temp_buffer, "%s/config/manager_setup.bin", self_path);
    return SaveFile(temp_buffer, (char *) &manager_cfg, sizeof(manager_cfg));
}

void video_adjust()
{
    while(1) {

        static long frame_count = 0;

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
        DrawString(0, (512 - 24)/2 - 64, "Use LEFT (-X) / RIGHT (+X) / UP (-Y) / DOWN (+Y) to adjust the screen");

        DrawFormatString(0, (512 - 24)/2, "Video Scale X: %i Y: %i", videoscale_x, videoscale_y);

        DrawString(0, (512 - 24)/2 + 64, "Press 'X' to exit");
        DrawString(0, (512 - 24)/2 + 96, "Press 'O' to default values");
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
             
                DrawDialogOK("manager_setup.bin\n\nFile Saved");
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
    
    dir = opendir ("/dev_hdd0/GAMEZ");
    if(dir) {
        closedir (dir);
        sprintf(temp_buffer, "%s %s %s", "Want to use", "/dev_hdd0/GAMEZ", "to install the game?"); 

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

            sprintf(temp_buffer, "/dev_hdd0/game/%s/GAMEZ", entry->d_name);
        
            dir2 = opendir (temp_buffer);

            if(dir2) {
                
                closedir (dir2);
      
                sprintf(temp_buffer, "%s /%s %s", "Want to use", entry->d_name, "to install the game?");

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
        
        sprintf(temp_buffer, "%s %s %s", "Want to use", "/dev_hdd0/GAMEZ", "to install the game?");

        if(DrawDialogYesNo(temp_buffer) == 1) {
            strncpy(hdd_folder, "dev_hdd0", 64);
            strncpy(manager_cfg.hdd_folder, "dev_hdd0", 64); 
            mkdir("/dev_hdd0/GAMEZ", S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
        } else {
            strncpy(hdd_folder, "HMANAGER4", 64);
            strncpy(manager_cfg.hdd_folder, "HMANAGER4", 64);
            mkdir("/dev_hdd0/game/HMANAGER4/GAMEZ", S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);

            sprintf(temp_buffer, "%s %s %s", "Using", "dev_hdd0/game/HMANAGER4/GAMEZ", "to install the game");
            DrawDialogOK(temp_buffer);
        }
    }
}

s32 main(s32 argc, const char* argv[])
{
	int flag = 0;
    int n;

    u32 entry = 0;
    u32 segmentcount = 0;
    sysSpuSegment * segments;

#ifndef WITH_CFW355
    flag = send_payload_code();
#endif

    SysLoadModule(SYSMODULE_FS);
    SysLoadModule(SYSMODULE_PNGDEC);
    SysLoadModule(SYSMODULE_IO);
    SysLoadModule(SYSMODULE_GCM_SYS);

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

    atexit(fun_exit);


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

#ifndef WITH_CFW355
    if(!flag) {
        lv2launch(0x80000000007e0000ULL);
        __asm__("sync");
        sleep(1);
    }
    
    if(sys8_enable(0) < 0) {
        if(sys8_enable(hmanager_key) < 0) {
            tiny3d_Init(1024*1024);

            ioPadInit(7);
            
            DrawDialogOK("Invalid payload or payload locked");

            exit(0);
        }
    }
#else
	if (!is_payload_loaded()) {
	/*
		install_new_poke();

		if (!map_lv1()) {
			remove_new_poke();
			exit(0);
		}

		patch_lv2_protection();
		remove_new_poke();

		unmap_lv1();

		load_payload();
    */
        tiny3d_Init(1024*1024);

        ioPadInit(7);
        DrawDialogOK("LV2 Patcher v9 (syscall36) is need it!");
        exit(0);
	}
#endif


    usleep(250000);

    sys8_perm_mode(2);
    sys8_path_table(0LL);

    sprintf(temp_buffer, "%s/config", self_path);

    mkdir(temp_buffer, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);

    sprintf(temp_buffer, "%s/self", self_path);

    mkdir(temp_buffer, S_IRWXO | S_IRWXU | S_IRWXG | S_IFDIR);
        
    long frame = 0;

    tiny3d_Init(1024*1024);

    ioPadInit(7);

	// Load texture

    LoadTexture();

    init_twat();

    // initialize manager conf

    memset(&manager_cfg, 0, sizeof(manager_cfg));

    for(n=0; n<4; n++) 
        manager_cfg.videoscale_x[n] = 1024;

    manager_cfg.background_sel = 0;

    LoadManagerCfg();


    if(flag == 1) {
        float x = 0.0f, y = 0.0f;

        cls();

        SetFontSize(32, 64);
        
        
        
        SetFontColor(0xffffffff, 0x00000000);
        SetFontAutoCenter(1);

        x= (848-640) / 2; y=(512-360)/2;
        DrawBox(x - 16, y - 16, 65535.0f, 640.0f + 32, 360 + 32, 0x00000028);
        DrawBox(x, y, 65535.0f, 640.0f, 360, 0x30003018);

        x= 0.0; y = 512.0f/2.0f - 32.0f;
        if(sys8_enable(0) < 0x102) DrawString(0, y, "Payload Resident Is Old");

            else DrawString(0, y, "Payload Is Resident");
        
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
     
        if(manager_cfg.hdd_folder[0] == 0) strcpy(manager_cfg.hdd_folder, "HMANAGER4");
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
    sys8_perm_mode((u64) 2);

    
    MODPlay_Init(&mod_track);
    
    if(MODPlay_SetMOD (&mod_track, beauty_mod_bin)<0) {
        MODPlay_Unload (&mod_track);
    } else {
        MODPlay_SetVolume( &mod_track, 6, 6); // fix the volume to 16 (max 64)
	    MODPlay_Start (&mod_track); // Play the MOD
        SND_Pause(1);
    }

    while(!exit_program) {

        float x = 0.0f, y = 0.0f;
    
        flash = (frame >> 5) & 1;

        frame++;


        //////////////////////////////////////

        int count_devices=0;

        int found_game_insert=0;
        
        int found_game_remove=0;

        char filename[1024];
        
        if(forcedevices || (frame & 63)==0 || fdevices == 0)
	    for(find_device = 0;find_device < 12; find_device++) {

			if(find_device==11) sprintf(filename, "/dev_bdvd");
			else if(find_device==0) sprintf(filename, "/dev_hdd0");
			else sprintf(filename, "/dev_usb00%c", 47+find_device);

            DIR  *dir;
			dir = opendir (filename);

			if (dir) {
                closedir (dir);
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

                        sprintf(temp_buffer, "%s/%s", directories[ndirectories].path_name, "PS3_DISC.SFB" );
                        parse_ps3_disc((char *) temp_buffer, directories[ndirectories].title_id);
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
					
				forcedevices &= ~ (1<<find_device);

				if(find_device==0) {
                    if (!memcmp(hdd_folder,"dev_hdd0",8)) {
                        sprintf(filename, "/%s/GAMEZ",hdd_folder); 
                    } else {
                        sprintf(filename, "/dev_hdd0/game/%s/GAMEZ",hdd_folder);  
                    }
 
                    sysFsGetFreeSize("/dev_hdd0/", &blockSize, &freeSize);
                    freeSpace[find_device] = ( ((u64)blockSize * freeSize));
                    freeSpace[find_device] = freeSpace[find_device] / 1073741824.00;
            
                } else {
                    sprintf(filename, "/dev_usb00%c/", 47+find_device);
                    sysFsGetFreeSize(filename, &blockSize, &freeSize);
                    double space = ( ((double)blockSize) * ((double) freeSize) ) /  1073741824.0;
                    freeSpace[find_device] = (float) space;
                    sprintf(filename, "/dev_usb00%c/GAMEZ", 47+find_device);
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
          get_games();
          select_option = 0;     
          menu_screen = 0;
        }
     
        found_game_remove=0;
        found_game_insert=0;

        SND_Pause(0);

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
            default:
                menu_screen = 0;
                break;
        }

    
     }


	return 0;
}

void draw_screen1(float x, float y)
{

    int i, n, m;

    float x2;

    SetCurrentFont(FONT_DEFAULT);

    // header title

    DrawBox(x, y, 0, 200 * 4 - 8, 20, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);
  
    DrawFormatString(x, y - 2, " Page %i", currentdir/12 + 1);

    SetFontAutoCenter(0);

    // list device space

    m = select_px + select_py * 4;

    if(Png_offset[m]) {
        for(i = 0; i < 11; i++)
            if(directories[currentdir + m].flags == (1<<i)) break;
        m = i;
    } else m = -1;

    for(n = 0; n < 2; n++) {

        if(n == 0) x2 = 2000;

        for(i = 0; i < 11; i++) {

            if(((fdevices>>i) & 1)) {
                
                if(m == i) SetFontColor(0x00ff00ff, 0x00000000); else SetFontColor(0xffffffff, 0x00000000);
                if(i==0)
                    x2= DrawFormatString(x2, -2, "hdd0: %.2fGB ", freeSpace[i]);
                else
                    x2= DrawFormatString(x2, -2, "usb00%c: %.2fGB ", 47 + i, freeSpace[i]);
            }

        }
    
    x2 = 848 -(x2 - 2000) - x;
    }

    SetFontColor(0xffffffff, 0x00000000);

    y += 24;
    
    i = 0;

    for(n = 0; n < 3; n++) 
        for(m = 0; m < 4; m++) {
            
            DrawBox(x + 200 * m, y + n * 150, 0, 192, 142, 0x00000028 );
            
            if(Png_offset[i]) {
               
                tiny3d_SetTextureWrap(0, Png_offset[i], Png_datas[i].width, 
                Png_datas[i].height, Png_datas[i].wpitch, 
                    TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                DrawTextBox(x + 200 * m, y + n * 150, 0, 192, 142, 0xffffffff);

                // draw Bluray icon
                if(directories[currentdir + i].flags == (1<<11)) {
                    tiny3d_SetTextureWrap(0, Png_res_offset[0], Png_res[0].width, 
                    Png_res[0].height, Png_res[0].wpitch, 
                        TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                    DrawTextBox(x + 200 * m + 32, y + n * 150 + 7, 0, 128, 128, 0xffffffcf);    
                } else 
                // draw Usb icon    
                if(directories[currentdir + i].flags > 1) {
                    tiny3d_SetTextureWrap(0, Png_res_offset[1], Png_res[1].width, 
                    Png_res[1].height, Png_res[1].wpitch, 
                        TINY3D_TEX_FORMAT_A8R8G8B8,  TEXTWRAP_CLAMP, TEXTWRAP_CLAMP,1);
                    DrawTextBox(x + 200 * m + 4, y + n * 150 + 4, 0, 32, 24, 0xffffffcf);    
                }
            }
            
         i++;   
        }

    if(flash)  DrawBox(x + 200 * select_px - 4, y + select_py * 150 - 4 , 0, 200, 150, 0xa0a06080);


    // draw game name

    i = select_px + select_py * 4;

    DrawBox(x, y + 3 * 150, 0, 200 * 4 - 8, 40, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);

    if(Png_offset[i]) {

        if(directories[currentdir + i].flags == (1<<11)) {
            utf8_to_ansi(bluray_game, temp_buffer, 65);
            SetFontColor(0x00ff00ff, 0x00000000);
        } else utf8_to_ansi(directories[currentdir + i].title, temp_buffer, 65);

        temp_buffer[65] = 0;

        if(strlen(temp_buffer) < 50) SetFontSize(18, 32); 
        else SetFontSize(14, 32);

        SetFontAutoCenter(1);
  
        
        DrawString(0, y + 3 * 150, temp_buffer);

        SetFontAutoCenter(0);
    }
   
    tiny3d_Flip();

    ps3pad_read();

    if(new_pad & BUTTON_TRIANGLE) {if(DrawDialogYesNo("Exit to XMB?")==1) {exit_program = 1; return;}}

    if(new_pad & BUTTON_CROSS) {
        i = select_px + select_py * 4;
        if(Png_offset[i]) {

            if(directories[currentdir + i].title[0] == '_') {
                sprintf(temp_buffer, "%s\n\nMarked as not executable", directories[currentdir + i].title);
                DrawDialogOK(temp_buffer);
            } else {
                
                 // load game config
                sprintf(temp_buffer, "%s/config/%s.cfg", self_path, directories[currentdir + i].title_id);
                memset(&game_cfg, 0, sizeof(game_cfg));
                
                int file_size;
                char *file = LoadFile(temp_buffer, &file_size);
                if(file) {
                    if(file_size > sizeof(game_cfg)) file_size = sizeof(game_cfg);
                    memcpy(&game_cfg, file, file_size);
                    free(file);
                }
                
                if(game_cfg.xmb && (fdevices & 2048) == 0) {
            
                    DrawDialogOK("Required BR-Disc");
                    goto skip_sys8;
                }

                if(!game_cfg.ext_ebootbin) sys8_path_table(0LL);
                else {
                        
                    u64 dest_table_addr;
                    path_open_table open_table;

                    sprintf(temp_buffer, "%s/self/%s.BIN", self_path, directories[currentdir + i].title_id);
                    FILE *fp = fopen(temp_buffer, "rb");

                    if(!fp) {
                        sprintf(temp_buffer, "%s.BIN\n\nexternal executable not found", directories[currentdir + i].title_id);
                        DrawDialogOK(temp_buffer);
                        goto skip_sys8;
                    } else {

                        fclose(fp);

                        sys8_path_table(0LL);

                        dest_table_addr= 0x80000000007FF000ULL-((sizeof(path_open_table)+15) & ~15);
                        open_table.entries[0].compare_addr= ((uint64_t) &open_table.arena[0]) - ((uint64_t) &open_table) + dest_table_addr;
                        open_table.entries[0].replace_addr= ((uint64_t) &open_table.arena[0x420])- ((uint64_t) &open_table) + dest_table_addr;
                        open_table.entries[1].compare_addr= 0ULL; // the last entry always 0
                        
                        strncpy(&open_table.arena[0], "/dev_bdvd/PS3_GAME/USRDIR/EBOOT.BIN", 0x100); // compare 1
                        strncpy(&open_table.arena[0x420], temp_buffer, 0x420);     // replace 1

                        open_table.entries[0].compare_len= strlen(&open_table.arena[0]);		// 1
                        open_table.entries[0].replace_len= strlen(&open_table.arena[0x420]);

                        sys8_memcpy(dest_table_addr, (uint64_t) &open_table, sizeof(path_open_table));

                        // set the path table
                        sys8_path_table( dest_table_addr);
                    }
                
                }
                
                sys8_perm_mode((u64) (game_cfg.perm & 3));
                if(!game_cfg.xmb) sys8_sys_configure(CFG_XMB_DEBUG); else sys8_sys_configure(CFG_XMB_RETAIL);
                
                sys8_sys_configure(CFG_UNPATCH_APPVER + (game_cfg.updates != 0));

                syscall36(directories[currentdir + i].path_name);

                exit_program = 1; 
                return;
                skip_sys8: ;
            }
        }
    }

    if(new_pad & BUTTON_START) {
         menu_screen = 3; return;
    }

    if(new_pad & BUTTON_SELECT) {
        i = select_px + select_py * 4;

        select_option = 0;

        if(Png_offset[i]) {
            
            Png_offset[12] = 0;
            sprintf(temp_buffer, "%s/PS3_GAME/PIC1.PNG", directories[currentdir + i].path_name);
            
            if(LoadTexturePNG(temp_buffer, 12) < 0) {
                sprintf(temp_buffer, "%s/PS3_GAME/PIC0.PNG", directories[currentdir + i].path_name);
                if(LoadTexturePNG(temp_buffer, 12) < 0) {
                    sprintf(temp_buffer, "%s/PS3_GAME/ICON0.PNG", directories[currentdir + i].path_name);
                    if(LoadTexturePNG(temp_buffer, 12) < 0) Png_offset[12] = 0;
                }
            } 
            
            currentgamedir = currentdir + i;
            menu_screen = 1; return;
        }
    }

    if(new_pad & BUTTON_UP) {
        select_py--; 
        if(select_py < 0) {
            
            select_py = 0;  
            if(currentdir >= 12) {currentdir -= 12; select_py = 2; get_games();}
            else {currentdir = ROUND_UP12(ndirectories) - 12; select_py = 2; get_games();}
        }
    }

    if(new_pad & BUTTON_DOWN) {
        
        select_py++;
        
        if(select_py > 2) {
           
            select_py = 2; 
            
            if(currentdir < (ROUND_UP12(ndirectories) - 12)) {currentdir += 12; select_py = 0; get_games();}
            else {currentdir = 0; select_py = 0; get_games();}
        }
    }

    if(new_pad & BUTTON_LEFT) {
        
        select_px--; 

        if(select_px < 0) {
            
            select_px = 0;
            
            if(currentdir >= 12) {currentdir -= 12; select_px = 3; get_games();}
            else {currentdir = ROUND_UP12(ndirectories) - 12; select_px = 3; get_games();}
        }
    }

    if(new_pad & BUTTON_RIGHT) {
        
        select_px++; 

        if(select_px > 3) {
            
            select_px = 3; 
            
            if(currentdir < (ROUND_UP12(ndirectories) - 12)) {currentdir += 12; select_px = 0;get_games();}
            else {currentdir = 0; select_px = 0; get_games();}
        }
    }
}



void draw_options(float x, float y, int index)
{

    int i, n;

    float y2, x2;

    int copy_flag = 1;
    

    SetCurrentFont(FONT_DEFAULT);

    // header title

    DrawBox(x, y, 0, 200 * 4 - 8, 20, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);
  
    DrawString(x, y - 2, " Options");

    i = select_px + select_py * 4;

    if(directories[currentdir + i].flags & 1) {

        copy_flag = 0;

        for(n = 1; n < 11; n++) {
            
            if((fdevices >> n) & 1) copy_flag = 1;
        }

    } else if(directories[currentdir + i].flags & 2046){
    
        copy_flag = 0;
        
        if((fdevices >> 0) & 1) copy_flag = 1; 
        
    }

    if(directories[currentdir + i].title_id[0] == 0 && select_option == 0) {
        select_option = 1;
        
        if(!copy_flag && select_option == 1) select_option++;

        if((directories[i].flags & 2048) && select_option == 2) select_option++;

    }

    SetCurrentFont(FONT_BUTTON);
        
    SetFontSize(16, 20);

    utf8_to_ansi(directories[currentdir + i].title_id, temp_buffer, 64);
    temp_buffer[64] = 0;
    
    DrawString(848 - x - strlen(temp_buffer) * 16 - 8, y, temp_buffer);

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
    
    DrawButton1(x + 32, y2, 320, "Config. Game", (directories[currentdir + i].title_id[0] == 0) ? -1 : (flash && select_option == 0));
    
    y2+= 48;

    DrawButton1(x + 32, y2, 320, "Copy Game", copy_flag ? (flash && select_option == 1) : -1);
    
    y2+= 48;

    DrawButton1(x + 32, y2, 320, "Delete Game", (directories[i].flags & 2048) ? -1  : ((flash && select_option == 2) ? 1 : 0));
    
    y2+= 48;

    DrawButton1(x + 32, y2, 320, "Test Game", (flash && select_option == 3));
    
    y2+= 48;

    DrawButton1(x + 32, y2, 320, "Copy EBOOT.BIN from USB", (directories[currentdir + i].title_id[0] == 0) ? -1 : (flash && select_option == 4));
    
    y2+= 48;

    DrawButton1(x + 32, y2, 320, "Return", (flash && select_option == 5));
    
    y2+= 48;
    
    for(n = 0; n < 2; n++) {
        
        DrawButton1(x + 32, y2, 320, "", -1);
    
        y2+= 48;
    }


    SetCurrentFont(FONT_DEFAULT);

    // draw game name

    DrawBox(x, y + 3 * 150, 0, 200 * 4 - 8, 40, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);

    if(Png_offset[i]) {

        if(directories[currentdir + i].flags == (1<<11)) {
            utf8_to_ansi(bluray_game, temp_buffer, 65);
            SetFontColor(0x00ff00ff, 0x00000000);
        } else utf8_to_ansi(directories[currentdir + i].title, temp_buffer, 65);

        temp_buffer[65] = 0;

        if(strlen(temp_buffer) < 50) SetFontSize(18, 32); 
        else SetFontSize(14, 32);

        SetFontAutoCenter(1);
  
        DrawString(0, y + 3 * 150, temp_buffer);

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
                sprintf(temp_buffer, "%s/config/%s.cfg", self_path, directories[currentdir + i].title_id);
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
                 i = select_px + select_py * 4;

                 if(Png_offset[i]) {

                    SND_Pause(1);

                    copy_from_selection(currentdir + i);

                    SND_Pause(0);

                    currentgamedir = currentdir = 0;
                    select_px = select_py = 0;
                    select_option = 0;
                    menu_screen = 0;
                 }
                 return;

            case 2:
                 i = select_px + select_py * 4;

                 if(Png_offset[i]) {
                    
                    SND_Pause(1);
                    
                    delete_game(currentdir + i);
                    
                    SND_Pause(0);

                    currentgamedir = currentdir = 0;
                    select_px = select_py = 0;
                    select_option = 0;
                    menu_screen = 0;
                 }
                 return;

            case 3:
                 i = select_px + select_py * 4;

                 if(Png_offset[i]) {

                    SND_Pause(1);

                    test_game(currentdir + i);

                    SND_Pause(0);
                    
                 }
                 break;
            case 4:
                {
                // load game config
                sprintf(temp_buffer, "/dev_usb/ps3game/%s.BIN", directories[currentdir + i].title_id);
                
                int file_size;
                char *file = LoadFile(temp_buffer, &file_size);
                if(file) {
                    sprintf(temp_buffer, "%s/self/%s.BIN", self_path, directories[currentdir + i].title_id);
                    if(SaveFile(temp_buffer, file, file_size)==0) {
                        sprintf(temp_buffer, "%s/self/%s.BIN\n\nEBOOT.BIN copied successfully", self_path, directories[currentdir + i].title_id);
                        DrawDialogOK(temp_buffer);
                    } else {
                        sprintf(temp_buffer, "%s/self/%s.BIN\n\nError copying EBOOT.BIN", self_path, directories[currentdir + i].title_id);
                        DrawDialogOK(temp_buffer);
                    }
                    free(file);
                } else {
                    sprintf(temp_buffer, "/dev_usb/ps3game/%s.BIN\n\nEBOOT.BIN not found", directories[currentdir + i].title_id);
                    DrawDialogOK(temp_buffer);
                }

                }
                break;

            case 5:
                select_option = 0;
                menu_screen = 0;
                return;

            default:
               break;
        }
       // menu_screen = 0; return;
    }

    if(new_pad & BUTTON_CIRCLE) {
        menu_screen = 0; return;
    }
   

    if(new_pad & BUTTON_UP) {
        select_option--; 

        if((directories[i].flags & 2048) && select_option == 2) select_option--; 
        if(!copy_flag && select_option == 1) select_option--;

        if(directories[currentdir + i].title_id[0] == 0 && (select_option == 0 || select_option == 4)) select_option--;

        if(select_option < 0) {
            
            select_option = 5;  
          
        }
    }

    if(new_pad & BUTTON_DOWN) {
        
        select_option++;

        if(!copy_flag && select_option == 1) select_option++;

        if((directories[i].flags & 2048) && select_option == 2) select_option++;
        
        if(select_option > 5) {
           
            select_option = 0;
           
        }

        if(directories[currentdir + i].title_id[0] == 0 && (select_option == 0 || select_option == 4)) {
            select_option++;
            
            if(!copy_flag && select_option == 1) select_option++;

            if((directories[i].flags & 2048) && select_option == 2) select_option++; 

        }
    }

    
}


void draw_configs(float x, float y, int index)
{

    int i;

    float y2, x2;
   

    SetCurrentFont(FONT_DEFAULT);

    // header title

    DrawBox(x, y, 0, 200 * 4 - 8, 20, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);
  
    DrawString(x, y - 2, " Config. Game");

    i = select_px + select_py * 4;

    SetCurrentFont(FONT_BUTTON);
        
    SetFontSize(16, 20);

    utf8_to_ansi(directories[currentdir + i].title_id, temp_buffer, 64);
    temp_buffer[64] = 0;
    
    DrawString(848 - x - strlen(temp_buffer) * 16 - 8, y, temp_buffer);

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


    x2 = DrawButton1(x + 32, y2, 240, "Fix Permissions", (flash && select_option == 0)) + 16;
    
    x2 = DrawButton2(x2, y2, 0, " Default ", (game_cfg.perm == 0)) + 8;
    x2 = DrawButton2(x2, y2, 0, " PS jailbreak ", (game_cfg.perm == 1)) + 8;
    x2 = DrawButton2(x2, y2, 0, " v4 Perms (F1) ", (game_cfg.perm == 2)) + 8;
    
    y2+= 48;

    x2 = DrawButton1(x + 32, y2, 240, "Select XMB", (flash && select_option == 1))  + 16;
        
    x2 = DrawButton2(x2, y2, 0, "  Debug  ", (game_cfg.xmb == 0)) + 8;
    x2 = DrawButton2(x2, y2, 0, "  Retail  ", (game_cfg.xmb == 1)) + 8;

    y2+= 48;

    x2 = DrawButton1(x + 32, y2, 240, "Online Updates", (flash && select_option == 2))  + 16;
        
    x2 = DrawButton2(x2, y2, 0, "  On  ", (game_cfg.updates == 0)) + 8;
    x2 = DrawButton2(x2, y2, 0, "  Off  ", (game_cfg.updates != 0)) + 8;

    y2+= 48;

    x2 = DrawButton1(x + 32, y2, 240, "Extern EBOOT.BIN", (flash && select_option == 3))  + 16;
        
    x2 = DrawButton2(x2, y2, 0, "  On  ", (game_cfg.ext_ebootbin != 0)) + 8;
    x2 = DrawButton2(x2, y2, 0, "  Off  ", (game_cfg.ext_ebootbin == 0)) + 8;

    y2+= 48;

    x2 = DrawButton1(x + 32, y2, 240, "Save Config", (flash && select_option == 4))  + 16;
    y2+= 48;

    x2 = DrawButton1(x + 32, y2, 240, "Return", (flash && select_option == 5))  + 16;
    y2+= 48;


    SetCurrentFont(FONT_DEFAULT);

    // draw game name

    i = select_px + select_py * 4;

    DrawBox(x, y + 3 * 150, 0, 200 * 4 - 8, 40, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);

    if(Png_offset[i]) {

        if(directories[currentdir + i].flags == (1<<11)) {
            utf8_to_ansi(bluray_game, temp_buffer, 65);
            SetFontColor(0x00ff00ff, 0x00000000);
        } else utf8_to_ansi(directories[currentdir + i].title, temp_buffer, 65);

        temp_buffer[65] = 0;

        if(strlen(temp_buffer) < 50) SetFontSize(18, 32); 
        else SetFontSize(14, 32);

        SetFontAutoCenter(1);
  
        
        DrawString(0, y + 3 * 150, temp_buffer);

        SetFontAutoCenter(0);
    }
   

    tiny3d_Flip();

    ps3pad_read();


    if(new_pad & BUTTON_CIRCLE) {
        menu_screen = 1; select_option = 0; return;
    }

   
    if(new_pad & BUTTON_CROSS) {
     
        switch(select_option) {
            case 0:
                ROT_INC(game_cfg.perm, 2, 0);
                break;
            case 1:
                ROT_INC(game_cfg.xmb, 1, 0);
                break;
            case 2:
                ROT_INC(game_cfg.updates, 1, 0);
                break;
            case 3:
                ROT_INC(game_cfg.ext_ebootbin, 1, 0);
                break;
            case 4:
                // load game config
                sprintf(temp_buffer, "%s/config/%s.cfg", self_path, directories[currentdir + i].title_id);
              
                
                if(SaveFile(temp_buffer, (char *) &game_cfg, sizeof(game_cfg)) == 0) {
                    sprintf(temp_buffer, "%s.cfg\n\nFile Saved", directories[currentdir + i].title_id);
                    DrawDialogOK(temp_buffer);
                }

                break;
            default:
                menu_screen = 1; select_option = 0; return;
                break;
        }
     }

    if(new_pad & BUTTON_UP) {

        ROT_DEC(select_option, 0, 5)
        
    }

    if(new_pad & BUTTON_DOWN) {
        
        ROT_INC(select_option, 5, 0); 
        
    }

    if(new_pad & BUTTON_LEFT) {
     
        switch(select_option) {
            case 0:
                ROT_DEC(game_cfg.perm, 0, 2);
                break;
            case 1:
                ROT_DEC(game_cfg.xmb, 0, 1);
                break;
            case 2:
                ROT_DEC(game_cfg.updates, 0, 1);
                break;
            case 3:
                ROT_DEC(game_cfg.ext_ebootbin, 0, 1);
                break;
            default:
                break;
        }
     }

     if(new_pad & BUTTON_RIGHT) {
     
        switch(select_option) {
            case 0:
                ROT_INC(game_cfg.perm, 2, 0);
                break;
            case 1:
                ROT_INC(game_cfg.xmb, 1, 0);
                break;
            case 2:
                ROT_INC(game_cfg.updates, 1, 0);
                break;
            case 3:
                ROT_INC(game_cfg.ext_ebootbin, 1, 0);
                break;
            default:
                break;
        }
     }

}

void draw_gbloptions(float x, float y)
{

    int n;

    float y2, x2;
    

    SetCurrentFont(FONT_DEFAULT);

    // header title

    DrawBox(x, y, 0, 200 * 4 - 8, 20, 0x00000028);

    SetFontColor(0xffffffff, 0x00000000);

    SetFontSize(18, 20);

    SetFontAutoCenter(0);
  
    DrawString(x, y - 2, " Global Options");

    y += 24;

    DrawBox(x, y, 0, 200 * 4 - 8, 150 * 3 - 8, 0x00000028);

    
    x2 = x;
    y2 = y + 32;
    
    DrawButton1((848 - 520) / 2, y2, 520, "Video Adjust", (flash && select_option == 0));
    
    y2+= 48;

    DrawButton1((848 - 520) / 2, y2, 520, "Change Game Directory", (flash && select_option == 1));
    
    y2+= 48;

    DrawButton1((848 - 520) / 2, y2, 520, "Change Background Color", (flash && select_option == 2));
    
    y2+= 48;

    if(manager_cfg.usekey)
        DrawButton1((848 - 520) / 2, y2, 520, "Press To Disable Syscall Security", (flash && select_option == 3));
    else
        DrawButton1((848 - 520) / 2, y2, 520, "Press To Enable Syscall Security", (flash && select_option == 3));
    
    y2+= 48;

    DrawButton1((848 - 520) / 2, y2, 520, "Load PS3LoadX", (flash && select_option == 4));
    
    y2+= 48;

    DrawButton1((848 - 520) / 2, y2, 520, (ftp_ip_str[0]) ? ftp_ip_str : "Initialize FTP server", (flash && select_option == 5));
    
    y2+= 48;

    DrawButton1((848 - 520) / 2, y2, 520, "Return", (flash && select_option == 6));
    
    y2+= 48;
    
    for(n = 0; n < 1; n++) {
        
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
                video_adjust();
                select_option = 0;
                menu_screen = 0; 
                return;

            case 1:
                menu_screen = 0;
                Select_games_folder();
     
                if(manager_cfg.hdd_folder[0] == 0) strcpy(manager_cfg.hdd_folder, "HMANAGER4");
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
                manager_cfg.usekey = manager_cfg.usekey == 0;
                SaveManagerCfg();
                break;

            case 4:
                load_ps3loadx = 1;
                exit(0);
                break;

            case 5:
                if(ftp_init()==0) DrawDialogOK("Server FTP initialized");
                else DrawDialogOK("Server FTP already initialized");
                break;

            case 6:
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

        ROT_DEC(select_option, 0, 6)
        
    }

    if(new_pad & BUTTON_DOWN) {
        
        ROT_INC(select_option, 6, 0); 
        
    }

    
}

