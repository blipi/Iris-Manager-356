/*                                                                                                                                                                                 
 * Copyright (C) 2010 drizzt
 *
 * Authors:
 * drizzt <drizzt@ibeglab.org>
 * flukes1
 * kmeaw
 * D_Skywalk
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License.
 */

#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <psl1ght/lv2.h>
#include "payload.h"
#include "hvcall.h"
#include "mm.h"
#include "syscall8.h"
#include "utils.h"

#include "payload_sky_356.bin.h"
#include "payload_syscall36.bin.h"

u64 mmap_lpar_addr;
static int poke_syscall = 7;

extern char path_name[MAXPATHLEN];
extern char temp_buffer[4096];

u64 lv2memcpy_[9] = { 0xF821FFF1FBA10000, 0x2F830000419E002C, 0x2F840000419E0024, 0x2F850000419E001C, 0x8BA400009BA30000, 0x3884000138630001, 0x38A5FFFF4BFFFFE4, 0xEBA1000038210010, 0x4E80002060000000 };
u64 lv2memset_[5] = { 0x2F830000419E001C, 0x2F850000419E0014, 0x9883000038630001, 0x38A5FFFF4BFFFFEC, 0x4E80002060000000 };

u64 lv1Peek(u64 addr)
{
	return Lv2Syscall1(10, addr);
}

void lv1Poke(u64 addr, u64 opcode)
{
	Lv2Syscall2(11, addr, opcode);
}

u64 peekq(u64 addr)
{
    return Lv2Syscall1(6, addr);
}

void pokeq(u64 addr, u64 val)
{
    Lv2Syscall2(poke_syscall, addr, val);
}

void pokeq32(u64 addr, uint32_t val)
{
    uint32_t next = peekq(addr) & 0xffffffff;
    pokeq(addr, (u64) val << 32 | next);
}

void lv1_poke(u64 address, u64 value)
{
    Lv2Syscall2(7, HV_BASE + address, value);
}

static inline void _poke(u64 addr, u64 val)
{
    pokeq(0x8000000000000000ULL + addr, val);
}

static inline void _poke32(u64 addr, uint32_t val)
{
    pokeq32(0x8000000000000000ULL + addr, val);
}

int is_payload_loaded(void)
{
    //1st classic syscall 36 check
    u64 *tmp = (u64 *) (u64) & payload_syscall36_bin[0]; //syscall 36 payload
    if(peekq(0x80000000002be4a0ULL) == *tmp)
        return SYS36_PAYLOAD;

    //2nd new syscall 36 - sky mod check
    if(peekq(0x800000000000F0A0ULL) == 0x534B313000000000ULL) //SK10 HEADER
        return SKY10_PAYLOAD;

    //WaninV2 CFW
    if(peekq(0x8000000000079d80ULL) == 0x3880000090830000ULL) //WaninV2
        return WANIN_PAYLOAD;

    return ZERO_PAYLOAD;
    
}

inline static void lv2memcpy( u64 to, const u64 from, size_t sz)
{
    Lv2Syscall3(15, to, from, sz);
}

inline static void lv2memset( u64 dst, const u64 val, size_t sz)
{
    Lv2Syscall3(16, dst, val, sz);
}

inline static void lv2_memcpy( u64 to, const u64 from, size_t sz)
{
    Lv2Syscall3(NEW_POKE_SYSCALL, to, from, sz);
}

inline static void lv2_memset( u64 dst, const u64 val, size_t sz)
{

    u64 *tmp = memalign(32, (sz*(sizeof(u64))) );
    if(!tmp)
        return;

    memset(tmp, val, sz);
    
    Lv2Syscall3(NEW_POKE_SYSCALL, dst, (u64) tmp, sz);

    free(tmp);
}

inline void install_lv2_new_syscalls()
{
	pokeq(0x80000000003465E8ULL, 0x800000000000F014ULL);
	pokeq(0x80000000003465F0ULL, 0x800000000000F024ULL);
	pokeq(0x800000000000F014ULL, 0x800000000000F034ULL);
	pokeq(0x800000000000F01CULL, 0x8000000000330540ULL);
	pokeq(0x800000000000F024ULL, 0x800000000000F07CULL);
	pokeq(0x800000000000F02CULL, 0x8000000000330540ULL);

	int i;
	for(i = 0; i < 9; i++)
		pokeq(0x800000000000F034ULL + (i * 8), lv2memcpy_[i]);
	
	for(i = 0; i < 5; i++)
		pokeq(0x800000000000F07CULL + (i * 8), lv2memset_[i]);
		
	__asm__("sync");
	sleep(1);
}
	
inline void install_lv2_memcpy()
{
    /* install memcpy */
    /* This does not work on some PS3s */
    pokeq(NEW_POKE_SYSCALL_ADDR, 0x4800000428250000ULL);
    pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0x4182001438a5ffffULL);
    pokeq(NEW_POKE_SYSCALL_ADDR + 16, 0x7cc428ae7cc329aeULL);
    pokeq(NEW_POKE_SYSCALL_ADDR + 24, 0x4bffffec4e800020ULL);
}

inline void remove_lv2_memcpy()
{
    /* restore syscall */
    remove_new_poke();
    pokeq(NEW_POKE_SYSCALL_ADDR + 16, 0xebc2fe287c7f1b78ULL);
    pokeq(NEW_POKE_SYSCALL_ADDR + 24, 0x3860032dfba100e8ULL);
}

void install_payload_exploit(void)
{
    /* install jump to exploit */
    pokeq(NEW_POKE_SYSCALL_ADDR, 0x7C6903A64E800420ULL); //mtctr   %r3 // bctr /* jump to exploit addr */
    pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0x4E8000204E800020ULL); // blr //blr /* maybe not need it */
    poke_syscall = NEW_POKE_SYSCALL;
}

inline static int lv2_call_payload(u64 addr)
{
    return Lv2Syscall3(NEW_POKE_SYSCALL, (u64) addr, 0,0); /* call new syscall and jump to exploit */
}


void remove_payload_exploit(void)
{
    /* restore syscall */
    remove_new_poke();
}


void load_payload(int mode)
{

    install_lv2_new_syscalls();
    /* WARNING!! It supports only payload with a size multiple of 8 */
    lv2memcpy(0x800000000000F0A0ULL,
                   (u64) payload_sky_356_bin, 
                   sizeof(payload_sky_356_bin));
			
    /* BASIC PATCHES SYS36 */
    // by 2 anonymous people
	/*
    _poke32(0x55f14, 0x60000000);
    _poke32(0x55f1c, 0x48000098);
    _poke32(0x7af68, 0x60000000);
    _poke32(0x7af7c, 0x60000000);
    _poke(0x55EA0, 0x63FF003D60000000);  // fix 8001003D error 
    _poke(0x55F64, 0x3FE080013BE00000);  // fix 8001003E error 
	*/
    /*
        -002b3290  f8 01 00 b0 7c 9c 23 78  7c 7d 1b 78 4b d9 b4 11  |....|.#x|}.xK...|
        +002b3290  f8 01 00 b0 7c 9c 23 78  4b d5 bf 40 4b d9 b4 11  |....|.#xK..@K...| (openhook jump - 0xF1D8)
    */
    _poke(0x2B3330, 0x4BD5BE684BD9B325); //jump hook
			
    /*
        00346690  80 00 00 00 00 32 49 68  80 00 00 00 00 32 49 68  Ç....2IhÇ....2Ih
    */
	
    _poke(0x346690, 0x800000000000F170ULL); // syscall_map_open_desc - sys36
    //_poke(0x3465b0, 0x800000000000F2E0ULL); // syscall_8_desc - sys8
	
	__asm__("sync");
	
	
#ifdef CONFIG_USE_SYS8PERMH4
    /*
        0x0E7F0, b perm_routine    //0x48000A30
        0x24E44, bl perm0_routine  //0x4BFEA3AD
        0xC1DD0, bl perm0_routine  //0x4BF4D421
    */
    //_poke32(0x0e7f0, 0x48000A30);
    //_poke32(0x24e44, 0x4BFEA3AD);
    //_poke32(0xc1dd0, 0x4BF4D421);
#endif

}

void load_payload_syscall36old(int mode)
{

    install_lv2_memcpy();
    /* WARNING!! It supports only payload with a size multiple of 8 */
    lv2_memcpy(0x80000000002be4a0ULL, 
                   (u64) payload_syscall36_bin, 
                   sizeof(payload_syscall36_bin));
    remove_lv2_memcpy();

    /* by 2 anonymous people */
    _poke32(0x55f14, 0x60000000);
    _poke32(0x55f1c, 0x48000098);
    _poke32(0x7af68, 0x60000000);
    _poke32(0x7af7c, 0x60000000);

    _poke(0x55EA0, 0x63FF003D60000000);  /* fix 8001003D error */
    _poke(0x55F64, 0x3FE080013BE00000);  /* fix 8001003E error */

    _poke(0x2b3274, 0x4800B32C2BA30420); /* add a jump to payload2_start - hook */
    _poke(0x346690, 0x80000000002be570); /* syscall_map_open_desc - sys36 */

}

int map_lv1(void)
{
    int result = lv1_undocumented_function_114(0, 0xC, HV_SIZE, &mmap_lpar_addr);
    if (result != 0) {
        return 0;
    }

    result = mm_map_lpar_memory_region(mmap_lpar_addr, HV_BASE, HV_SIZE, 0xC, 0);
    if (result) {
        return 0;
    }

    return 1;
}

void unmap_lv1(void)
{
    if (mmap_lpar_addr != 0)
        lv1_undocumented_function_115(mmap_lpar_addr);
}

void patch_lv2_protection(void)
{
    // changes protected area of lv2 to first byte only
    lv1Poke(0x3661F8, 1);
	lv1Poke(0x366200, 0xE0D251B556C59F05ULL);
	lv1Poke(0x366208, 0xC232FCAD552C80D7ULL);
	lv1Poke(0x366210, 0x65140CD200000000ULL);
}

void install_new_poke(void)
{
    // install poke with icbi instruction
    pokeq(NEW_POKE_SYSCALL_ADDR, 0xF88300007C001FACULL);
    pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0x4C00012C4E800020ULL);
    poke_syscall = NEW_POKE_SYSCALL;
}

void remove_new_poke(void)
{
    poke_syscall = 7;
    pokeq(NEW_POKE_SYSCALL_ADDR, 0xF821FF017C0802A6ULL);
    pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0xFBC100F0FBE100F8ULL);
}


/* SYS36 utils */
void sys36_memcpy( u64 to, const u64 from, size_t sz)
{
    install_lv2_memcpy();
    lv2_memcpy( to, from, sz);
    remove_lv2_memcpy();
}

/******************************************************************************************************************************************************/
/* BDVDEMU FUNCTIONS                                                                                                                                  */
/******************************************************************************************************************************************************/

#define LV2MOUNTADDR_356 0x80000000003F64C4ULL

int lv2_unpatch_bdvdemu(void)
{
    int n;
    int flag = 0;
 
    char * mem = temp_buffer;
    memset(mem, 0, 0xff0);
    
    lv2memcpy( (u64) mem, LV2MOUNTADDR_356, 0xff0);

    for(n = 0; n< 0xff0; n+= 0x100)
    {
        if(!memcmp(mem + n, "CELL_FS_IOS:PATA0_BDVD_DRIVE", 29))
        {
            if(!memcmp(mem + n + 0x69, "temp_bdvd", 10))
            {
                lv2memcpy(LV2MOUNTADDR_356 + n + 0x69, (u64) "dev_bdvd\0", 10);
                flag++;
            }  
        }

        if(!memcmp(mem + n, "CELL_FS_IOS:USB_MASS_STORAGE0", 29)) 
        {
            if(!memcmp(mem + n + 0x69, "dev_bdvd", 9) || !memcmp(mem + n + 0x69, "temp_usb", 9))
            {
                lv2memcpy(LV2MOUNTADDR_356 + n + 0x69, (u64) (mem + n + 0x79), 11);
                lv2memset(LV2MOUNTADDR_356 + n + 0x79, 0ULL, 12);
                flag+=10;
            }
        }
    }
    
    if((mem[0] == 0) && (flag == 0))
        return -1;
    else
        return flag;

}


int lv2_patch_bdvdemu(uint32_t flags)
{
    int n;
    int flag = 0;
    int usb = -1;

    char * mem = temp_buffer;
    memset(mem, 0, 0xff0);


    lv2memcpy((u64) mem, LV2MOUNTADDR_356, 0xff0);

    for(n = 1; n < 11; n++) 
    {
        if(flags == (1 << n))
        {
            usb = n - 1;
            break;
        }
    }

    sprintf(path_name, "CELL_FS_IOS:USB_MASS_STORAGE00%c", 48 + usb);
    sprintf(&path_name[128], "dev_usb00%c", 48 + usb);

    for(n = 0; n< 0xff0; n+= 0x100)
    {
        if(!memcmp(mem + n, "CELL_FS_IOS:PATA0_BDVD_DRIVE", 29))
        {
            lv2memcpy(LV2MOUNTADDR_356 + n + 0x69, (u64) "temp_bdvd", 10);
            flag++;
        }

        if(!memcmp(mem + n, path_name, 32))
        {
            lv2memcpy(LV2MOUNTADDR_356 + n + 0x69, (u64) "dev_bdvd\0\0", 11);
            lv2memcpy(LV2MOUNTADDR_356 + n + 0x79, (u64) &path_name[128], 11);
            flag+=10;
        }
    }
    
    return flag;
}

