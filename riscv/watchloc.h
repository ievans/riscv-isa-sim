#ifndef _WATCHLOC_H
#define _WATCHLOC_H

#include "decode.h"
#include "processor.h"
#include <cstdint>
#include <string.h>

class watch_loc
{
  public:
    watch_loc(state_t * st);

    void update_addr(uint64_t addr);
    reg_t get_nth_recent_access(size_t n);
    void access(reg_t addr);
    void reset();

  private:
    state_t * state;
    uint64_t addr;
    size_t last_access;
    reg_t last_accesses[1024];
};
#endif // _WATCHLOC_H
