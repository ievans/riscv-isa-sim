// See LICENSE for license details.

#ifndef _LIBSPIKE_H
#define _LIBSPIKE_H

#include <stdint.h>

#define LIBSPIKE_BASE_ADDR 0x200000000000ll
#define LIBSPIKE_FN_OFFSET 0xe00
#define LIBSPIKE_TAG_OFFSET 0xf00

#define ADD_FN(fn_name) inline void fn_name() {*((uint8_t*) (LIBSPIKE_BASE_ADDR + LIBSPIKE_FN_OFFSET + 8*(__LINE__ - base))) = 1;}

// Don't add extra newlines!
static int base = __LINE__ + 1;
ADD_FN(cache_reset)
ADD_FN(update_cachestats)

typedef struct {
  float miss_rates[16];
  char names[16][32];
  uint32_t n_caches;
} cache_info_t;

typedef union {
  cache_info_t cache_info;
  uint8_t buf[4096]; 
} libspike_page_u;

inline cache_info_t* get_cache_info() {
  return &(((libspike_page_u*) (LIBSPIKE_BASE_ADDR))->cache_info);
}

#undef ADD_FN
#endif
