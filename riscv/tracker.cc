// See LICENSE for license details.

#include <string.h>
#include <stdio.h>
#include "tracker.h"
#include "disasm.h"
#include "processor.h"
#include "decode.h"

tracker_t::tracker_t(processor_t* _proc, size_t _sz)
  : proc(_proc), sz(_sz) {

  disasm = proc->get_disassembler();

  mem = (node_t**) malloc(sizeof(node_t*) * sz);
  memset(mem, 0, sizeof(node_t*) * sz);

  regs = (node_t**) malloc(sizeof(node_t*) * 32);
  memset(regs, 0, sizeof(node_t*) * 32);
}

void tracker_t::track_load(uint64_t paddr) {
  int dst = last_insn.rd();
  if(dst == 0) return;
  uint64_t ind = paddr / MEM_TO_TAG_RATIO;

  node_t* node = (node_t*) malloc(sizeof(node_t));
  node->val = dst;
  node->pc = proc->state.pc;
  node->c1 = mem[ind];
  node->c2 = 0;
  if(node->c1)
    node->c1->refc++;

  // Replace the old node for this register
  cleanup(regs[dst]);
  node->refc = 1;
  regs[dst] = node;
}

void tracker_t::track_store(uint64_t paddr, uint64_t addr) {
  uint64_t ind = paddr / MEM_TO_TAG_RATIO;
  int reg = last_insn.rs2();

  node_t* node = (node_t*) malloc(sizeof(node_t));
  node->val = addr;
  node->pc = proc->state.pc;
  node->c1 = mem[reg];
  node->c2 = 0;
  if(node->c1)
    node->c1->refc++;

  // Replace the old node for this register
  cleanup(mem[ind]);
  node->refc = 1;
  mem[ind] = node;
}

void tracker_t::track(insn_t insn) {
  last_insn = insn;
  int *buf = disasm->lookup_args(insn);
  int n = buf[0];
  buf = buf + 1;

  // Check for destination register
  int dst = insn.rd();
  if(n == 0 || dst <= 0 || dst != buf[0]) return;

  // Check for memory as argument
  // For loads/stores, we process insn later because we need the physical address
  for(int i = 1; i < n; i++) {
    if(buf[i] == -1) return;
  }

  // Check for duplicate pc
  if((n >= 2 && regs[buf[1]] && regs[buf[1]]->pc == proc->state.pc) ||
     (n >= 3 && regs[buf[2]] && regs[buf[2]]->pc == proc->state.pc))
  {
    return;
  }

  // Check for null propogation
  if(n == 2 && regs[buf[1]] && regs[buf[1]]->val == (uint64_t) dst)
    return;

  // Only registers here, set up a node
  node_t* node = (node_t*) malloc(sizeof(node_t));
  node->val = dst;
  node->c1 = 0;
  node->c2 = 0;
  if(n >= 2) {
    node->c1 = regs[buf[1]];
    if(node->c1)
      node->c1->refc++;
    if(n >= 3) {
      node->c2 = regs[buf[2]];
      if(node->c2)
        node->c2->refc++;
      if(n >= 4) {
        proc->monitor();
        printf("help\n");
      }
    }
  }
  node->pc = proc->state.pc;
  node->refc = 1;

  // Replace the old node for this register

  cleanup(regs[dst]);
  regs[dst] = node;
  /*
  print(node);
  proc->monitor();
  */
}

void tracker_t::print(node_t *node, int depth) {
  if(node == NULL) return;
  for(int i = 0; i < depth; i++)
    printf("  ");
  printf("(%016" PRIx64 ", %016" PRIx64 "):\n", node->val, node->pc);
  print(node->c1, depth+1);
  print(node->c2, depth+1);
}

void tracker_t::cleanup(node_t *node) {
  if(node == NULL) return;
  node->refc--;
  if(node->refc == 0) {
    cleanup(node->c1);
    cleanup(node->c2);
    free(node);
  }
}
