// See LICENSE for license details.

#ifndef _MEMTRACER_H
#define _MEMTRACER_H

#include <cstdint>
#include <string.h>
#include <vector>
#include <algorithm>
#include "libspike.h"

class cache_sim_t;

class memtracer_t
{
 public:
  memtracer_t() {}
  virtual ~memtracer_t() {}

  virtual bool interested_in_range(uint64_t begin, size_t bytes, bool store, bool fetch, bool is_tag) = 0;
  virtual void trace(uint64_t addr, size_t bytes, bool store, bool fetch, bool is_tag, uint64_t val) = 0;
  virtual void print_stats() {}
  virtual void reset() {}
  virtual bool is_list() {
    return false;
  }
  virtual cache_sim_t *get_cache() {
    return NULL;
  }
};

class memtracer_list_t : public memtracer_t
{
 public:
  bool empty() { return list.empty(); }
  bool interested_in_range(uint64_t begin, size_t bytes, bool store, bool fetch, bool is_tag)
  {
    for (std::vector<memtracer_t*>::iterator it = list.begin(); it != list.end(); ++it)
      if ((*it)->interested_in_range(begin, bytes, store, fetch, is_tag))
        return true;
    return false;
  }

  void trace(uint64_t addr, size_t bytes, bool store, bool fetch, bool is_tag, uint64_t val)
  {
    for (std::vector<memtracer_t*>::iterator it = list.begin(); it != list.end(); ++it) {
      if((*it)->interested_in_range(addr, bytes, store, fetch, is_tag))
        (*it)->trace(addr, bytes, store, fetch, is_tag, val);
    }
  }

  void hook(memtracer_t* h)
  {
    list.push_back(h);
  }

  void print_stats() {
    for(uint32_t i = 0; i < list.size(); i++) {
      list[i]->print_stats();
    }
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
  void update_stats(cache_info_t* cache_info);

 private:
  std::vector<memtracer_t*> list;
  std::vector<cache_sim_t*> cache_buf;
};

#endif
