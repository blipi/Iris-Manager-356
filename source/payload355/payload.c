/*                                                                                                                                                                                 
 * Copyright (C) 2010 drizzt                                                                                                                                                       
 *                                                                                                                                                                                 
 * Authors:                                                                                                                                                                        
 * drizzt <drizzt@ibeglab.org>                                                                                                                                                     
 * flukes1
 * kmeaw
 *                                                                                                                                                                                 
 * This program is free software; you can redistribute it and/or modify                                                                                                            
 * it under the terms of the GNU General Public License as published by                                                                                                            
 * the Free Software Foundation, version 3 of the License.                                                                                                                         
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <psl1ght/lv2.h>
#include "payload.h"
#include "hvcall.h"
#include "mm.h"
#include "syscall8.h"

#include "payload_syscall36.bin.h"

/* removed, better use direct memory patch */
/* quick and safe */
#if 0
#include "patch_syscall36_txt.bin.h"
#endif
 
u64 mmap_lpar_addr;
static int poke_syscall = 7;

extern char path_name[MAXPATHLEN];
extern char temp_buffer[4096];

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
	u64 *tmp = (u64 *) (u64) & payload_syscall36_bin[0];

	return peekq(0x80000000002be4a0ULL) == *tmp;
}

inline static void lv2_memcpy( u64 to, const u64 from, size_t sz)
{
	Lv2Syscall3(NEW_POKE_SYSCALL, to, from, sz);
}

inline static void lv2_memcpy_b(void *to, const void *from, size_t sz)
{
	Lv2Syscall3(NEW_POKE_SYSCALL, (unsigned long long)to,
		    (unsigned long long)
		    from, sz);
}

void load_payload(void)
{

#ifdef USE_MEMCPY_SYSCALL

	/* This does not work on some PS3s */
	pokeq(NEW_POKE_SYSCALL_ADDR, 0x4800000428250000ULL);
	pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0x4182001438a5ffffULL);
	pokeq(NEW_POKE_SYSCALL_ADDR + 16, 0x7cc428ae7cc329aeULL);
	pokeq(NEW_POKE_SYSCALL_ADDR + 24, 0x4bffffec4e800020ULL);

    lv2_memcpy(0x80000000002be4a0ULL, 
				   (u64) payload_syscall36_bin, 
				   sizeof(payload_syscall36_bin));

	/* restore syscall */
	remove_new_poke();
	pokeq(NEW_POKE_SYSCALL_ADDR + 16, 0xebc2fe287c7f1b78);
	pokeq(NEW_POKE_SYSCALL_ADDR + 24, 0x3860032dfba100e8);

#else
	/* WARNING!! It supports only payload with a size multiple of 4 */
	uint32_t i;

	u64 *pl64 = (u64 *) (u64) &payload_syscall36_bin[0];

	for (i = 0; i < (u64) sizeof(payload_syscall36_bin) / sizeof(u64); i++) {
		pokeq(0x80000000002be4a0ULL + i * sizeof(u64), *pl64++);
	}
	if ((u64) sizeof(payload_syscall36_bin) % sizeof(u64)) {
		pokeq(0x80000000002be4a0ULL + i * sizeof(u64), (uint32_t) * pl64);
	}
#endif

/*
55f14: 60000000
55f1c: 48000098
7af68: 60000000
7af7c: 60000000

2b3274: 4800b32c2ba30420 # add a jump to payload2_start

55EA0: 63FF003D60000000  # fix 8001003D error
55F64: 3FE080013BE00000  # fix 8001003E error

346690: 80000000002be570 # syscall_map_open_desc
*/

    /* by 2 anonymous people */
    _poke32(0x55f14, 0x60000000);
    _poke32(0x55f1c, 0x48000098);
    _poke32(0x7af68, 0x60000000);
    _poke32(0x7af7c, 0x60000000);

    _poke(0x2b3274, 0x4800b32c2ba30420); /* add a jump to payload2_start */
    _poke(0x55EA0, 0x63FF003D60000000);  /* fix 8001003D error */
    _poke(0x55F64, 0x3FE080013BE00000);  /* fix 8001003E error */
    
    _poke(0x346690, 0x80000000002be570); /* syscall_map_open_desc */

/* removed, better use direct memory patch */
/* quick and safe */
#if 0

	char *ptr, *ptr2;
	unsigned long long addr, value;
	int patches = 0;

	char *tmp = strtok((char *) &patch_syscall36_txt_bin[0], "\n");

	do {
		ptr = strchr(tmp, '#');
		if (ptr)
			*ptr = 0;
		ptr = tmp;

		while (*ptr == ' ' || *ptr == '\t')
			ptr++;
		if (!strchr("0123456789abcdefABCDEF", *ptr))
			continue;
		addr = strtoull(ptr, &ptr, 16);
		if (*ptr != ':')
			continue;
		else
			ptr++;
		while (*ptr == ' ' || *ptr == '\t')
			ptr++;
		if (!strchr("0123456789abcdefABCDEF", *ptr))
			continue;
		ptr2 = ptr;
		value = strtoull(ptr, &ptr, 16);

		patches++;

		if (ptr - ptr2 == 8) {
			_poke32(addr, value);
		} else if (ptr - ptr2 == 16) {
			_poke(addr, value);
		} else
			patches--;
	}
	while ((tmp = strtok(NULL, "\n")));
#endif

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
	lv1_poke(0x363a78, 0x0000000000000001ULL);
	lv1_poke(0x363a80, 0xe0d251b556c59f05ULL);
	lv1_poke(0x363a88, 0xc232fcad552c80d7ULL);
	lv1_poke(0x363a90, 0x65140cd200000000ULL);
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


/******************************************************************************************************************************************************/
/* BDVDEMU FUNCTIONS                                                                                                                                  */
/******************************************************************************************************************************************************/

#define LV2MOUNTADDR_355 0x80000000003F64C4ULL

int lv2_unpatch_bdvdemu(void)
{
    int n;
    int flag = 0;
 
    char * mem = temp_buffer;
    memset(mem, 0, 0xff0);
    
    install_new_poke();

    lv2_memcpy( (u64) mem, LV2MOUNTADDR_355, 0xff0);

    for(n = 0; n< 0xff0; n+= 0x100) {

        if(!memcmp(mem + n, "CELL_FS_IOS:PATA0_BDVD_DRIVE", 29))
        {
            if(!memcmp(mem + n + 0x69, "temp_bdvd", 10))
            {
                sys8_memcpy(LV2MOUNTADDR_355 + n + 0x69, (u64) "dev_bdvd\0", 10ULL);
                flag++;
            }  
        }

        if(!memcmp(mem + n, "CELL_FS_IOS:USB_MASS_STORAGE0", 29)) {
            if(!memcmp(mem + n + 0x69, "dev_bdvd", 9)) 
            {
                sys8_memcpy(LV2MOUNTADDR_355 + n + 0x69, (u64) (mem + n + 0x79), 11ULL);
                sys8_memset(LV2MOUNTADDR_355 + n + 0x79, 0ULL, 12ULL);
                flag+=10;
            }
            
        }
      
    }
    
    remove_new_poke();

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

    install_new_poke();

    lv2_memcpy((u64) mem, LV2MOUNTADDR_355, 0xff0);

    sprintf(path_name, "CELL_FS_IOS:USB_MASS_STORAGE00%c", 48 + usb);
    sprintf(&path_name[128], "dev_usb00%c", 48 + usb);

    for(n = 0; n< 0xff0; n+= 0x100) {

        if(!memcmp(mem + n, "CELL_FS_IOS:PATA0_BDVD_DRIVE", 29)) {
    
            sys8_memcpy(LV2MOUNTADDR_355 + n + 0x69, (u64) "temp_bdvd", 10ULL);
            flag++;
        }

        if(!memcmp(mem + n, path_name, 32)) {
           
            sys8_memcpy(LV2MOUNTADDR_355 + n + 0x69, (u64) "dev_bdvd\0\0", 11ULL);
            sys8_memcpy(LV2MOUNTADDR_355 + n + 0x79, (u64) &path_name[128], 11ULL);
            
            flag+=10;
        }
      
    }
    
    remove_new_poke();
    
    return flag;
}

