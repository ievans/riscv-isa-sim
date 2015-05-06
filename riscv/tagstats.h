// See LICENSE for license details.

#ifndef _TAGSTATS_H
#define _TAGSTATS_H

#include "memtracer.h"
#include "libspike.h"

#include <stdint.h>
#include <string.h>

class tag_memtracer_t : public memtracer_t
{
 public:
  tag_memtracer_t(void *base, int sz)
  {
    this->base = (uint64_t) base;
    this->sz = sz;
    last_tag = (uint8_t*) malloc(sz);
    reset();
  }

  ~tag_memtracer_t()
  {
    delete last_tag;
  }

  void print_stats() {
    uint64_t n_accesses = 0;
    int i;
    for(i = 0; i < 256; i++)
      n_accesses += count[i];
    printf("Active tags: %u\n", (uint32_t) n_accesses);
    for(i = 0; i < 256; i++) {
      if(count[i] > 0) {
        double f = (double) count[i] / n_accesses;
        printf("Tag %d: %05f\n", i, f);
      }
    }
  }

  void reset() {
    memset(last_tag, 0xff, sz);
    memset(count, 0, sizeof(count));
  }

  bool interested_in_range(uint64_t begin, size_t bytes, bool store, bool fetch, bool is_tag)
  {
    return is_tag && store;
  }

  void trace(uint64_t addr, size_t bytes, bool store, bool fetch, bool is_tag, uint64_t val) {
    if(is_tag && store) {
      addr -= base;
      // fprintf(stderr, "address 0x%016" PRIx64 ", tag 0x%016" PRIx64 "\n", addr, val);
      if(last_tag[addr] != 0xff) {
        count[last_tag[addr]]--;
        fprintf(stderr, "Last tag: %d\n", last_tag[addr]);
      }
      count[(uint8_t) val]++;
    }
  }

 protected:
  uint64_t base;
  uint64_t sz;
  uint64_t count[256];
  uint8_t *last_tag;
};

#endif
