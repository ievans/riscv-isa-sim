// See LICENSE for license details.

#ifndef _RISCV_MMU_H
#define _RISCV_MMU_H

#include "decode.h"
#include "trap.h"
#include "common.h"
#include "config.h"
#include "processor.h"
#include "memtracer.h"
#include "libspike.h"
#include "watchloc.h"
#include <vector>
#include <iostream>
#include <assert.h>

// virtual memory configuration
typedef reg_t pte_t;
const reg_t LEVELS = sizeof(pte_t) == 8 ? 3 : 2;
const reg_t PTIDXBITS = 10;
const reg_t PGSHIFT = PTIDXBITS + (sizeof(pte_t) == 8 ? 3 : 2);
const reg_t PGSIZE = 1 << PGSHIFT;
const reg_t VPN_BITS = PTIDXBITS * LEVELS;
const reg_t PPN_BITS = 8*sizeof(reg_t) - PGSHIFT;
const reg_t VA_BITS = VPN_BITS + PGSHIFT;

struct insn_fetch_t
{
  insn_func_t func;
  insn_t insn;
};

struct icache_entry_t {
  reg_t tag;
  reg_t pad;
  insn_fetch_t data;
};

// this class implements a processor's port into the virtual memory system.
// an MMU and instruction cache are maintained for simulator performance.
class mmu_t
{
public:
  mmu_t(char* _mem, char* _tagmem, size_t _memsz);
  ~mmu_t();

  // template for functions that load an aligned value from memory
  #define load_func(type) \
    type##_t load_##type(reg_t addr) __attribute__((always_inline)) { \
      void* paddr = translate(addr, sizeof(type##_t), false, false); \
      if (unlikely(tracer.interested_in_range((uint64_t) paddr, sizeof(type##_t), false, false, false))) \
        tracer.trace((uint64_t) paddr, sizeof(type##_t), false, false, false, 0); \
      has_no_tag = false; \
      return *(type##_t*)paddr; \
    }

  #define load_func_tagged(type) \
    tagged_reg_t load_tagged_##type(reg_t addr) __attribute__((always_inline)) { \
      tagged_reg_t r; \
      void* paddr = translate(addr, sizeof(type##_t), false, false); \
      r.val = *(type##_t*)paddr; \
      if(likely(!has_no_tag)) { \
        if (unlikely(tracer.interested_in_range((uint64_t) paddr, sizeof(type##_t), false, false, false))) \
          tracer.trace((uint64_t) paddr, sizeof(type##_t), false, false, false, 0); \
        void *tagaddr = paddr_to_tagaddr(paddr); \
        r.tag = *(tag_t*)tagaddr; \
        if (unlikely(tracer.interested_in_range((uint64_t) tagaddr, 1, false, false, true))) \
          tracer.trace((uint64_t) tagaddr, 1, false, false, true, 0); \
      } \
      has_no_tag = false; \
      return r; \
    }

  // load value from memory at aligned address; zero extend to register width
  load_func(uint8)
  load_func(uint16)
  load_func(uint32)
  load_func(uint64)
  
  load_func_tagged(uint8)
  load_func_tagged(uint16)
  load_func_tagged(uint32)
  load_func_tagged(uint64)

  // load value from memory at aligned address; sign extend to register width
  load_func(int8)
  load_func(int16)
  load_func(int32)
  load_func(int64)

  load_func_tagged(int8)
  load_func_tagged(int16)
  load_func_tagged(int32)
  load_func_tagged(int64)

  // template for functions that store an aligned value to memory
  #define store_func(type) \
    void store_##type(reg_t addr, type##_t val) { \
      void* paddr = translate(addr, sizeof(type##_t), true, false); \
      if (unlikely(tracer.interested_in_range((uint64_t) paddr, sizeof(type##_t), true, false, false))) \
        tracer.trace((uint64_t) paddr, sizeof(type##_t), true, false, false, val); \
      *(type##_t*)paddr = val; \
      has_no_tag = false; \
    }

  #define store_func_tagged(type) \
    void store_tagged_##type(reg_t addr, type##_t val, tag_t tag) { \
      void* paddr = translate(addr, sizeof(type##_t), true, false); \
      *(type##_t*)paddr = val; \
      if(likely(!has_no_tag)) { \
        if (unlikely(tracer.interested_in_range((uint64_t) paddr, sizeof(type##_t), true, false, false))) \
          tracer.trace((uint64_t) paddr, sizeof(type##_t), true, false, false, val); \
        void *tagaddr = paddr_to_tagaddr(paddr); \
        *(tag_t*)tagaddr = tag; \
        if (unlikely(tracer.interested_in_range((uint64_t) tagaddr, 1, true, false, true))) \
          tracer.trace((uint64_t) tagaddr, 1, true, false, true, tag); \
      } \
      has_no_tag = false; \
    }

  void store_tag_value(tag_t value, reg_t addr) {
    void* paddr = translate(addr, 1, true, false);
    void* tagaddr = paddr_to_tagaddr(paddr);
    if (unlikely(tracer.interested_in_range((uint64_t) tagaddr, 1, true, false, true)))
      tracer.trace((uint64_t) tagaddr, 1, true, false, true, value);
    *(tag_t*)tagaddr = value;
  }

  // store value to memory at aligned address
  store_func(uint8)
  store_func(uint16)
  store_func(uint32)
  store_func(uint64)

  store_func_tagged(uint8)
  store_func_tagged(uint16)
  store_func_tagged(uint32)
  store_func_tagged(uint64)

  static const reg_t ICACHE_ENTRIES = 1024;

  inline size_t icache_index(reg_t addr)
  {
    // for instruction sizes != 4, this hash still works but is suboptimal
    return (addr / 4) % ICACHE_ENTRIES;
  }

  // load instruction from memory at aligned address.
  icache_entry_t* access_icache(reg_t addr) __attribute__((always_inline))
  {
    reg_t idx = icache_index(addr);
    icache_entry_t* entry = &icache[idx];
    if (likely(entry->tag == addr))
      return entry;

    bool rvc = false; // set this dynamically once RVC is re-implemented
    char* iaddr = (char*)translate(addr, rvc ? 2 : 4, false, true);
    insn_bits_t insn = *(uint16_t*)iaddr;

    if (unlikely(insn_length(insn) == 2)) {
      insn = (int16_t)insn;
    } else if (likely(insn_length(insn) == 4)) {
      if (likely((addr & (PGSIZE-1)) < PGSIZE-2))
        insn |= (insn_bits_t)*(int16_t*)(iaddr + 2) << 16;
      else
        insn |= (insn_bits_t)*(int16_t*)translate(addr + 2, 2, false, true) << 16;
    } else if (insn_length(insn) == 6) {
      insn |= (insn_bits_t)*(int16_t*)translate(addr + 4, 2, false, true) << 32;
      insn |= (insn_bits_t)*(uint16_t*)translate(addr + 2, 2, false, true) << 16;
    } else {
      static_assert(sizeof(insn_bits_t) == 8, "insn_bits_t must be uint64_t");
      insn |= (insn_bits_t)*(int16_t*)translate(addr + 6, 2, false, true) << 48;
      insn |= (insn_bits_t)*(uint16_t*)translate(addr + 4, 2, false, true) << 32;
      insn |= (insn_bits_t)*(uint16_t*)translate(addr + 2, 2, false, true) << 16;
    }

    insn_fetch_t fetch = {proc->decode_insn(insn), insn};
    icache[idx].tag = addr;
    icache[idx].data = fetch;

    reg_t paddr = iaddr - mem;
    if (!tracer.empty() && tracer.interested_in_range(paddr, 1, false, true, false))
    {
      icache[idx].tag = -1;
      tracer.trace(paddr, 1, false, true, false, 0);
    }
    return &icache[idx];
  }

  inline insn_fetch_t load_insn(reg_t addr)
  {
    return access_icache(addr)->data;
  }

  void set_processor(processor_t* p) { proc = p; flush_tlb(); }

  void flush_tlb();
  void flush_icache();

  void register_memtracer(memtracer_t*);
  watch_loc* get_watch_loc() { return wl; }
  void set_watch_loc(watch_loc* new_wl) { wl = new_wl; }
  void print_memtracer();
  void reset_memtracer();

  void monitor() {
    proc->monitor();
  }

private:
  char* mem;
  char* tagmem;
  size_t memsz;
  bool has_no_tag = false;
  processor_t* proc;
  memtracer_list_t tracer;
  watch_loc* wl;

  // implement an instruction cache for simulator performance
  icache_entry_t icache[ICACHE_ENTRIES];

  // implement a TLB for simulator performance
  static const reg_t TLB_ENTRIES = 256;
  char* tlb_data[TLB_ENTRIES];
  reg_t tlb_insn_tag[TLB_ENTRIES];
  reg_t tlb_load_tag[TLB_ENTRIES];
  reg_t tlb_store_tag[TLB_ENTRIES];

  // finish translation on a TLB miss and upate the TLB
  void* refill_tlb(reg_t addr, reg_t bytes, bool store, bool fetch);

  // perform a page table walk for a given virtual address
  pte_t walk(reg_t addr);

  // Libspike functions
  void reset_caches();
  void update_cachestats();

  // Libspike page buffer
  libspike_page_u libspike_page;

  // List of libspike functions
  typedef void (mmu_t::*libspike_func)();
  std::vector<libspike_func> libspike_funcs;
  void init_libspike();

  int get_libspike_fn_ind(reg_t addr) {
    int ind = (addr - LIBSPIKE_BASE_ADDR - LIBSPIKE_FN_OFFSET) / 8;
    if(ind >= 0 && (uint32_t) ind < libspike_funcs.size()) // shut up gcc
      return ind;
    return -1;
  }

  // wrapper for translate, but for watching a memory location
  void* translate(reg_t addr, reg_t bytes, bool store, bool fetch)
    __attribute__((always_inline))
  {
    // do evil things
    if (wl && store && !fetch) {
      // pass to watch_loc
      wl->access(addr);
    }
    return __translate(addr, bytes, store, fetch);
  }

  // translate a virtual address to a physical address
  void* __translate(reg_t addr, reg_t bytes, bool store, bool fetch)
    __attribute__((always_inline))
  {
    if (unlikely((addr >> PGSHIFT) == (LIBSPIKE_BASE_ADDR >> PGSHIFT))) {
      has_no_tag = true;
      int fn_ind = get_libspike_fn_ind(addr);
      if(fn_ind >= 0) {
        (this->*libspike_funcs[fn_ind])();
      }

      return (void*) (libspike_page.buf + (addr & ((1 << PGSHIFT) - 1)));
    }
    reg_t idx = (addr >> PGSHIFT) % TLB_ENTRIES;
    reg_t expected_tag = addr >> PGSHIFT;
    reg_t* tags = fetch ? tlb_insn_tag : store ? tlb_store_tag :tlb_load_tag;
    reg_t tag = tags[idx];
    void* data = tlb_data[idx] + addr;

    if (unlikely(addr & (bytes-1)))
      store ? throw trap_store_address_misaligned(addr) :
      fetch ? throw trap_instruction_address_misaligned(addr) :
      throw trap_load_address_misaligned(addr);

    if (likely(tag == expected_tag))
      return data;

    return refill_tlb(addr, bytes, store, fetch);
  }

  void *paddr_to_tagaddr(void *paddr)
    __attribute__((always_inline))
  {
    uint64_t out = (uint64_t) tagmem + ((uint64_t) paddr - (uint64_t) mem) / MEM_TO_TAG_RATIO;

    return (void*) out;
  }
  
  friend class processor_t;
};

#endif
