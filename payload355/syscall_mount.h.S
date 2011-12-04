/*
 * sycall_mount.h.S -- PS3 Jailbreak payload : mount syscall
 *
 * Copyright (C) Danger Blarg
 *
 * This software is distributed under the terms of the GNU General Public
 * License ("GPL") version 3, as published by the Free Software Foundation.
 *
 */

#ifndef __SYSCALL_MOUNT_H_S__
#define __SYSCALL_MOUNT_H_S__

#define LOAD_ABS(dest, source, address) LOAD_LABEL(0, dest, source, address)
#define LOADI_ABS(dest, address) LOAD_ABS(dest, dest, address)

#define BRANCH_ABSOLUTE(dest, target) 	\
	MEM_BASE (dest);		\
	LOADI_ABS (dest, target);	\
	mtctr	dest;			\
	bctrl;

.org 0x378
syscall_mount_desc:
    .long 0x80000000
    .long BASE + syscall_mount

.org 0x388
syscall_mount:
    // Set up work area, backup lr
    mflr %r0
    stdu %r1, -0x100(%r1)
    std %r0, 0x110(%r1)

    // Backup registers
    std %r25, 0x60(%r1)
    std %r26, 0x70(%r1)
    std %r27, 0x80(%r1)
    std %r28, 0x90(%r1)

    // Store parameters
    mr %r26, %r3 // %r26 = fsdev
    mr %r27, %r4 // %r27 = fstype
    mr %r28, %r5 // %r28 = mountpath

    // %r1[0x50] = kernel fsdev
    mr %r3, %r26
    addi %r4, %r1, 0x50
    BRANCH_ABSOLUTE(%r6, pathdup_from_user)

    // %r1[0x40] = kernel fstype
    mr %r3, %r27
    addi %r4, %r1, 0x40
    BRANCH_ABSOLUTE(%r6, pathdup_from_user)

    // %r1[0x30] = kernel mountpath
    mr %r3, %r28
    addi %r4, %r1, 0x30
    BRANCH_ABSOLUTE(%r6, pathdup_from_user)

    // u64 mount(char* fsdev, char* fstype, char* mountpath, u64 u1, u64 u2, u64 u3, char** sarr, int carr)
    ld %r3, 0x50(%r1) // fsdev
    ld %r4, 0x40(%r1) // fstype
    ld %r5, 0x30(%r1) // mountpath
    li %r6, 0 // u1
    li %r7, 0 // u2 - protection on?
    li %r8, 0 // u3
    li %r9, 0 // sarr
    li %r10, 0 // carr
    BRANCH_ABSOLUTE(%r11, mount)

    // Store return value
    std %r3, 0x20(%r1)

syscall_mount_exit_free:
    ld %r3, 0x30(%r1)
    li %r4, 0x27
    BRANCH_ABSOLUTE(%r6, free)

    ld %r3, 0x40(%r1)
    li %r4, 0x27
    BRANCH_ABSOLUTE(%r6, free)

    ld %r3, 0x50(%r1)
    li %r4, 0x27
    BRANCH_ABSOLUTE(%r6, free)

syscall_mount_exit:
    // Restore registers
    ld %r28, 0x90(%r1)
    ld %r27, 0x80(%r1)
    ld %r26, 0x70(%r1)
    ld %r25, 0x60(%r1)

    // Restore return value
    ld %r3, 0x20(%r1)

    // Restore lr / r1 and return
    ld %r0, 0x110(%r1)
    addi %r1, %r1, 0x100
    mtlr %r0
    blr

#endif /* __SYSCALL_MOUNT_H_S__ */
