#ifndef _PAYLOAD_H
#define _PAYLOAD_H


#include <unistd.h>

#define ZERO_PAYLOAD 0
#define SYS36_PAYLOAD 1
#define SKY10_PAYLOAD 2

#define WANIN_PAYLOAD -1

extern uint64_t peekq(uint64_t addr);
extern void pokeq(uint64_t addr, uint64_t val);
extern void pokeq32(uint64_t addr, uint32_t val);
extern void load_payload(int mode);
extern int lv2_unpatch_bdvdemu(void);
extern int lv2_patch_bdvdemu(uint32_t flags);
extern void sys36_memcpy( uint64_t to, const uint64_t from, size_t sz);

extern int map_lv1(void);
extern void unmap_lv1(void);
extern void patch_lv2_protection(void);
extern void install_new_poke(void);
extern void remove_new_poke(void);
extern int is_payload_loaded(void);

#endif

/* vim: set ts=4 sw=4 sts=4 tw=120 */
