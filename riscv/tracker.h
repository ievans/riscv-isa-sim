// See LICENSE for license details.
#ifndef _RISCV_TRACKER_H
#define _RISCV_TRACKER_H

#include <stdint.h>
#include "decode.h"

class processor_t;
class disassembler_t;

typedef struct node_t {
  node_t *c1, *c2;
  uint64_t pc;
  uint64_t val;
  int refc;
} node_t;

class tracker_t {
public:
  tracker_t(processor_t* _proc, size_t _sz);
  void reset();
  void track(insn_t insn);
  void track_load(uint64_t paddr);
  void track_store(uint64_t paddr, uint64_t addr);

  void print(node_t *node) {
    print(node, 0);
  }
  void print(node_t *node, int depth);
  void cleanup(node_t *node);
private:
  processor_t *proc;
  disassembler_t *disasm;
  node_t **mem;
  node_t **regs;
  insn_t last_insn;
  size_t sz;
};

#endif
