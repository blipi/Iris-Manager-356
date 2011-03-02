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

#include "payload_syscall36.bin.h"
#include "patch_syscall36_txt.bin.h"

u64 mmap_lpar_addr;
static int poke_syscall = 7;

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
	uint32_t next = peekq(addr) & 0xffffffffUL;
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

inline static void lv2_memcpy(void *to, const void *from, size_t sz)
{
	Lv2Syscall3(NEW_POKE_SYSCALL, (unsigned long long)to,
		    (unsigned long long)
		    from, sz);
}

void load_payload(void)
{
	char *ptr, *ptr2;
	unsigned long long addr, value;
	int patches = 0;

#ifdef USE_MEMCPY_SYSCALL
	/* This does not work on some PS3s */
	pokeq(NEW_POKE_SYSCALL_ADDR, 0x4800000428250000ULL);
	pokeq(NEW_POKE_SYSCALL_ADDR + 8, 0x4182001438a5ffffULL);
	pokeq(NEW_POKE_SYSCALL_ADDR + 16, 0x7cc428ae7cc329aeULL);
	pokeq(NEW_POKE_SYSCALL_ADDR + 24, 0x4bffffec4e800020ULL);

    lv2_memcpy((void *) 0x80000000002be4a0ULL, 
				   payload_syscall36_bin, 
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

/* vim: set ts=4 sw=4 sts=4 tw=120 */
