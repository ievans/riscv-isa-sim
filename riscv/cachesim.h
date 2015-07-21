// See LICENSE for license details.

#ifndef _RISCV_CACHE_SIM_H
#define _RISCV_CACHE_SIM_H

#include "memtracer.h"
#include "libspike.h"
#include <cstring>
#include <string>
#include <map>
#include <cstdint>

class lfsr_t
{
 public:
  lfsr_t() : reg(1) {}
  lfsr_t(const lfsr_t& lfsr) : reg(lfsr.reg) {}
  uint32_t next() { return reg = (reg>>1)^(-(reg&1) & 0xd0000001); }
 private:
  uint32_t reg;
};

class cache_sim_t
{
 public:
  cache_sim_t(size_t sets, size_t ways, size_t linesz, const char* name);
  cache_sim_t(const cache_sim_t& rhs);
  virtual ~cache_sim_t();

  void access(uint64_t addr, size_t bytes, bool store);
  const char* get_name() { return name.c_str(); }
  void print_stats();
  float get_miss_rate();
  void write_stats(cache_stats_t *dst);
  void set_miss_handler(cache_sim_t* mh) { miss_handler = mh; }
  cache_sim_t* get_miss_handler() { return miss_handler; }
  void set_tag_mode(bool mode) { tag_mode = mode; }
  bool is_tag_cache() { return tag_mode; }

  static cache_sim_t* construct(const char* config, const char* name);

  void reset() {
    read_accesses = 0;
    read_misses = 0;
    bytes_read = 0;
    write_accesses = 0;
    write_misses = 0;
    bytes_written = 0;
    writebacks = 0;

    memset(tags, 0, sizeof(uint64_t) * sets*ways);

    if(miss_handler)
      miss_handler->reset();
  }

 protected:
  static const uint64_t VALID = 1ULL << 63;
  static const uint64_t DIRTY = 1ULL << 62;

  virtual uint64_t* check_tag(uint64_t addr);
  virtual uint64_t victimize(uint64_t addr);

  lfsr_t lfsr;
  cache_sim_t* miss_handler;

  size_t sets;
  size_t ways;
  size_t linesz;
  size_t idx_shift;

  uint64_t* tags;
  
  uint64_t read_accesses;
  uint64_t read_misses;
  uint64_t bytes_read;
  uint64_t write_accesses;
  uint64_t write_misses;
  uint64_t bytes_written;
  uint64_t writebacks;
  bool tag_mode = false;

  std::string name;

  void init();
};

class fa_cache_sim_t : public cache_sim_t
{
 public:
  fa_cache_sim_t(size_t ways, size_t linesz, const char* name);
  uint64_t* check_tag(uint64_t addr);
  uint64_t victimize(uint64_t addr);
 private:
  static bool cmp(uint64_t a, uint64_t b);
  std::map<uint64_t, uint64_t> tags;
};

class cache_memtracer_t : public memtracer_t
{
 public:
  cache_memtracer_t(const char* config, const char* name)
  {
    cache = cache_sim_t::construct(config, name);
  }
  ~cache_memtracer_t()
  {
    delete cache;
  }
  void set_miss_handler(cache_sim_t* mh)
  {
    cache->set_miss_handler(mh);
  }
  void print_stats() {
    cache->print_stats();
  }
  void reset() {
    cache->reset();
  }
  cache_sim_t *get_cache() {
    return cache;
  }

 protected:
  cache_sim_t* cache;
};

class icache_sim_t : public cache_memtracer_t
{
 public:
  icache_sim_t(const char* config) : cache_memtracer_t(config, "I$") {}
  bool interested_in_range(uint64_t begin, uint64_t end, bool store, bool fetch, bool is_tag)
  {
    return fetch;
  }
  void trace(uint64_t addr, size_t bytes, bool store, bool fetch, bool is_tag, uint64_t val)
  {
    if (fetch) cache->access(addr, bytes, false);
  }
};

class dcache_sim_t : public cache_memtracer_t
{
 public:
  dcache_sim_t(const char* config) : cache_memtracer_t(config, "D$") {}
  bool interested_in_range(uint64_t begin, uint64_t end, bool store, bool fetch, bool is_tag)
  {
    return !fetch && !is_tag;
  }
  void trace(uint64_t addr, size_t bytes, bool store, bool fetch, bool is_tag, uint64_t val)
  {
    if (!fetch && !is_tag) cache->access(addr, bytes, store);
  }
};

#endif
