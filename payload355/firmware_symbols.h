// Defines for PS3 3.55
#define strncmp                0x4e6d8                                                                                                                                    
#define strcpy                 0x4e684                                                                                                                                            
#define pathdup_from_user      0x18dc68                                                                                                                                           
#define open_mapping_table_ext 0x7fff00                                                                                                                                           
#define strlen                 0x4e6ac                                                                                                                                            
#define alloc                  0x60b78                                                                                                                                            
#define free                   0x60fb4
#define memory_patch_func      0x2b329c

// FROM PL3 - 3.55
// TOC at 0x330540
// Shell code addr
/*
# PL3:
ef48: payload.bin
# Segment 0:
55dc4: 38600000 # lv2open: patch_func8_offset1
55f28: 60000000 # lv2open: patch_func8_offset2
2b3298: 4BD5C050 # hook_open (patch_func3 + patch_func3_offset)
# Segment 1:
346688: 800000000000f2dc # syscall_map_open_desc
*/

/* Common Symbols */

#define memcpy 0x7c3a4
#define memset 0x4e4d8
#define copy_from_user 0xf8c0
#define copy_to_user 0xf6a4
#define alloc_and_copy_from_user 0x18DE28
#define strdup_from_user 0x192B8C
#define strdup 0x192B8C

#define SYSCALL_TABLE 0x346570

#if 0 // disabled - i dont need this :D
#define memory_patch_func 0x1c34c
#define patch_func1 0x323C
#define patch_func1_offset 0x34
#define patch_func2 0x7A7C4
#define patch_func2_offset 0x2C
#define patch_func3 0x2B3274 // hooked_open
#define patch_func3_offset 0x24
#define patch_func4 0x79D80
#define patch_func4_offset 0x118
#define patch_func5 0xE7F0
#define patch_func5_offset 0x0
#define patch_func6 0x24DC4
#define patch_func6_offset 0x80
#define patch_func7 0xC1DA4 // syscall_512
#define patch_func7_offset 0x2C
#define patch_func8 0x55D20 //lv2open update patch
#define patch_func8_offset1 0xA4 //lv2open update patch
#define patch_func8_offset2 0x208 //lv2open update patch
#define patch_func9 0x7AB90 // must upgrade error
#define patch_func9_offset 0x3EC
#define patch_syscall_func 0x297db0
#define patch_data1 0x3c2f00
#define rtoc_entry_1 0x1030
#define rtoc_entry_2 -0x5B80

#define lv2_printf 0x29285C
#define lv2_printf_null 0x2972CC
#define hvsc107_1 0xFC3C
#define hvsc107_2 0xFCD0
#define hvsc107_3 0xFB0C

// Payload bases
//#define MEM_BASE2 0xECF0
#define MEM_BASE2 0xEF48

#define RESIDENT_AREA_MAXSIZE 1452
#endif

