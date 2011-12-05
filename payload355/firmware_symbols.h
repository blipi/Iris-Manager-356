// Defines for PS3 3.55
#define strncmp                     0x4E684                                                                                                                                 
#define strcpy                      0x4E630                                                                                                                                       
#define strlen                      0x4E658                                                                                                                                       
#define alloc                       0x60B24                                                                                                                                      
#define free                        0x60F60
#define mount                       0x2B2B54

#define memory_patch_func           0x2B3334
#define pathdup_from_user           0x18DCFC                                                                                                                                           
#define open_mapping_table_ext      0x7fff00                                                                                                                                           

/* Common Symbols PL3 */

#define memcpy                      0x7C440
#define memset                      0x4E484
#define copy_from_user              0x0F8D8
#define copy_to_user                0x0F6BC

#define alloc_and_copy_from_user    0x18DEBC
#define strdup_from_user            0x192C20
#define strdup                      0x192C20

#define perm_patch_func             0x0E810
#define rtoc_entry_1		        0x1030
#define rtoc_entry_2		       -0x5b80

