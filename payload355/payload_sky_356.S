/*
 * payload_sky.S -- Adds system call 36 and 8 code on custom firmware
 *                  It contains hermes, pl3 and psgroove parts of code
 *
 * Copyright (C) Youness Alaoui (KaKaRoTo)
 * Copyright (C) Aaron Lindsay (Aaron')
 * Copyright (C) (subdub)
 * Copyright (C) (Hermes)
 * Copyright (C) Miguel Boton (Waninkoko)
 * Copyright (C) (D_Skywalk)
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 *
 * This payload is a modified version of the original PSJailbreak's payload.
 * The people behing PSJailbrak are the original authors and copyright holders
 * of the code they wrote.
 *
 */

#include "firmware_symbols.h"

#define BASE        0xF0A0
#define PATHSTR_LEN    0x9
#define ABS(target) ((target) - (BASE) - .)

// Dynamic macros to load a label into a register
#define MEM_BASE(dest) \
    li dest, 1; \
    rldicr dest, dest, 63, 0;
#define LOAD_LABEL(base, dest, source, address) \
    oris    dest, source, ((base) + (address))@h; \
    ori    dest, dest, ((base) + (address))@l;
#define LOAD_LABEL2(dest, source, address) \
    LOAD_LABEL(BASE, dest, source, address)
#define LOADI_LABEL2(dest, address) \
    LOAD_LABEL2(dest, dest, address)

.org 0
destination_code:
return1_h4:
    li      %r3, 1
    blr
return0_h4:
    li      %r3, 0
    blr
.org 0x10
    .byte 'S', 'K', '1','0'

.org 0x30
syscall36_map_dvd:
    stdu    %sp, -0xD0(%sp)
    mflr    %r0
    std    %r0, 0xE0(%sp)
    std    %r31, 0xC8(%sp)
    addi    %r4, %sp, 0x70
    bl    ABS(pathdup_from_user) // strdup %r3 from userspace and store the new pointer into %r1[0x70]
    MEM_BASE(%r31)
    LOADI_LABEL2(%r31, buffer)
    ld    %r3, 0(%r31)
    cmpdi    %r3, 0 // if game_path != NULL: free(game_path)
    beq    l_game_path_null
    li    %r4, 0x27
    bl    ABS(free)

l_game_path_null:
    li    %r4, 0x27
    li    %r3, 0x800
    bl    ABS(alloc)    // alloc (2048)
    std    %r3, 0(%r31)
    ld    %r4, 0x70(%sp)
    bl    ABS(strcpy)
    ld    %r3, 0x70(%sp)
    li    %r4, 0x27
    bl    ABS(free)
    ld    %r3, 0(%r31)
    bl    ABS(strlen)
    ld    %r4, 0(%r31)
    add    %r3, %r4, %r3
    std    %r3, 8(%r31)
    li    %r3, 0    // return 0
    ld    %r31, 0xC8(%sp)
    ld    %r0, 0xE0(%sp)
    addi    %sp, %sp, 0xD0
    mtlr    %r0
    blr

.org 0xD0
    .long 0x80000000
    .long BASE + syscall36_map_dvd

buffer:
    .quad 0
    .quad 0

.org 0xF8
open_hook:
	//overwriten by the jump
    mr    %r29, %r3

check_path_dvd:
//start new code
    MEM_BASE(%r31)
    LOAD_LABEL2(%r4, %r31, aDev_bdvd)
    li    %r5, PATHSTR_LEN
    bl    ABS(strncmp)
    cmpldi    %r3, 0
    bne    check_path_app //if not bdvd check app_home
	
    LOADI_LABEL2(%r31, buffer)

    ld    %r3, 0(%r31)
    cmpldi    %r3, 0
    beq    return_org_open_func //if null exit
    ld    %r3, 8(%r31)  //buffer + 1? save len?
    addi    %r4, %r29, PATHSTR_LEN
    bl    ABS(strcpy)
    ld    %r29, 0(%r31)
    b     return_org_open_func //process finished ok, go out now :D

check_path_app:
    mr    %r3, %r29
    MEM_BASE(%r31)
    LOAD_LABEL2(%r4, %r31, aApp_home)
    li    %r5, PATHSTR_LEN
    bl    ABS(strncmp)
    cmpldi    %r3, 0
    bne    return_org_open_func
	
    LOADI_LABEL2(%r31, buffer)

    ld    %r3, 0(%r31)
    cmpldi    %r3, 0
    beq    return_org_open_func //if null exit
    ld    %r3, 8(%r31)  //buffer + 1? save len?
    addi    %r4, %r29, PATHSTR_LEN
    bl    ABS(strcpy)
    ld    %r29, 0(%r31)

return_org_open_func:
    mr    %r3, %r29
    b    ABS(memory_patch_func)

aDev_bdvd:    .string    "/dev_bdvd"
aApp_home:    .string    "/app_home"

.org 0x200
//.org 0x5B8 //max 0x5AC?
/* next lv2 code
    stdu    %sp, -0x80(%sp)
*/
