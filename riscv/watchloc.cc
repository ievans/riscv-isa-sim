#include "watchloc.h"
#include <iostream>

watch_loc::watch_loc(state_t * st) {
      state = st;
}

void watch_loc::update_addr(uint64_t addr) {
  this->addr = addr;
  this->reset();
}

void watch_loc::access(reg_t addr) {
  // check addr
  if (this->addr != addr) {
    return;
  }

  // Update counter, then get pc and store it.
  ++(this->last_access);
  this->last_access %= 1024;
  this->last_accesses[this->last_access] = state->pc;
}

reg_t watch_loc::get_nth_recent_access(size_t n) {
  // make sure n is in range
  n = n % 1024;

  // can normally subtract to get index, but might underflow
  size_t idx = this->last_access - n;
  if (n < idx) {
    // % shouldn't be necessary but is here to prevent corruption
    idx = (1023 - n + this->last_access + 1) % 1024;
  }

  return this->last_accesses[idx];
} 

void watch_loc::reset() {
  memset(this->last_accesses, 0, 1024);
  this->last_access = 0;
}

