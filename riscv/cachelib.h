// See LICENSE for license details.

#ifndef _CACHELIB_H
#define _CACHELIB_H

#include <stdint.h>

#define CACHE_ADDR 0x200000000000ll
#define CACHE_RESET_OFFSET 0xff0ll

typedef struct {
  float miss_rates[16];
  char names[16][32];
  uint32_t n_caches;
} cache_info_t;

typedef union {
  cache_info_t cache_info;
  uint8_t buf[4096]; 
} cache_info_u;

inline cache_info_t* get_cache_info() {
  return &(((cache_info_u*) (CACHE_ADDR))->cache_info);
}

inline void cache_reset() {
  *((uint8_t*) (CACHE_ADDR + CACHE_RESET_OFFSET)) = 1;
}

#endif
