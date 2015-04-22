// See LICENSE for license details.

#ifndef _MEMTRACER_H
#define _MEMTRACER_H

#include <cstdint>
#include <string.h>
#include <vector>
#include <algorithm>
#include "cachelib.h"

class cache_sim_t;

class memtracer_t
{
 public:
  memtracer_t() {}
  virtual ~memtracer_t() {}

  virtual bool interested_in_range(uint64_t begin, uint64_t end, bool store, bool fetch) = 0;
  virtual void trace(uint64_t addr, size_t bytes, bool store, bool fetch) = 0;
  virtual void print_stats() = 0;
  virtual void reset() = 0;
  virtual bool is_list() = 0;
  virtual cache_sim_t *get_cache() {
    return NULL;
  }
};

class memtracer_list_t : public memtracer_t
{
 public:
  bool empty() { return list.empty(); }
  bool interested_in_range(uint64_t begin, uint64_t end, bool store, bool fetch)
  {
    for (std::vector<memtracer_t*>::iterator it = list.begin(); it != list.end(); ++it)
      if ((*it)->interested_in_range(begin, end, store, fetch))
        return true;
    return false;
  }

  void trace(uint64_t addr, size_t bytes, bool store, bool fetch)
  {
    for (std::vector<memtracer_t*>::iterator it = list.begin(); it != list.end(); ++it)
      (*it)->trace(addr, bytes, store, fetch);
  }

  void hook(memtracer_t* h)
  {
    list.push_back(h);
  }

  void print_stats() {
    for(uint32_t i = 0; i < list.size(); i++) {
      list[i]->print_stats();
    }
    update_page();
  }

  void reset() {
    for(uint32_t i = 0; i < list.size(); i++) {
      list[i]->reset();
    }
  }

  bool is_list() {
    return true;
  }

  void add_cache(cache_sim_t* cache);
  void get_all_caches();
  void update_page();

  uint8_t* get_page() {
    update_page();
    return page_buf.buf;
  }

 private:
  std::vector<memtracer_t*> list;
  std::vector<cache_sim_t*> cache_buf;
  cache_info_u page_buf;
};

#endif
