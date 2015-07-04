// See LICENSE for license details.

#include <string.h>
#include <stdio.h>
#include "tracker.h"
#include "disasm.h"
#include "processor.h"
#include "decode.h"
#include <assert.h>
#include <vector>

#define REG_SP 2
#define MAX_ADDR_DEPTH 0
#define MAX_DEPTH 20

tracker_t::tracker_t(processor_t* _proc, size_t _sz)
  : proc(_proc), sz(_sz) {

  disasm = proc->get_disassembler();

  mem = (node_t**) malloc(sizeof(node_t*) * sz);
  memset(mem, 0, sizeof(node_t*) * sz);

  regs = (node_t**) malloc(sizeof(node_t*) * NXPR);
  memset(regs, 0, sizeof(node_t*) * NXPR);

  debug = 0;
}

void tracker_t::track_load(uint64_t paddr) {
  if(!is_mem_insn) return;
  is_mem_insn = false;
  int dst = last_insn.rd();
  if(dst == 0) return;
  uint64_t ind = paddr / MEM_TO_TAG_RATIO;

  node_t* node = (node_t*) malloc(sizeof(node_t));
  node->val = dst;
  node->pc = last_pc;
  node->insn = last_insn;
  node->is_mem = 1;
  node->c1 = mem[ind];
  node->c2 = regs[last_insn.rs1()];
  if(node->c1)
    node->c1->refc++;
  if(node->c2)
    node->c2->refc++;

  // Replace the old node for this register
  cleanup(regs[dst]);
  node->refc = 1;
  regs[dst] = node;
  if(debug) {
    print(node);
  }
}

void tracker_t::track_store(uint64_t paddr, uint64_t addr) {
  if(!is_mem_insn) return;
  is_mem_insn = false;
  uint64_t ind = paddr / MEM_TO_TAG_RATIO;
  int reg = last_insn.rs2();

  node_t* node = (node_t*) malloc(sizeof(node_t));
  node->val = addr;
  node->pc = last_pc;
  node->insn = last_insn;
  node->is_mem = 1;
  node->c1 = regs[reg];
  node->c2 = regs[last_insn.rs1()];
  if(node->c1)
    node->c1->refc++;
  if(node->c2)
    node->c2->refc++;

  // Replace the old node for this register
  cleanup(mem[ind]);
  node->refc = 1;
  mem[ind] = node;

  if(debug) {
    print(node);
  }
}

void tracker_t::track(insn_t insn, reg_t pc) {
  last_insn = insn;
  last_pc = pc;
  is_mem_insn = false;
  int *buf = disasm->lookup_args(insn);
  int n = buf[0];
  buf = buf + 1;

  // Check that instruction is meaningful
  if(n == 0 || buf[0] == 0) return;
  assert(buf[0] > 0);

  // Optimization: ignore changes to stack pointer
  if(buf[0] == REG_SP) return;

  // Check for memory as argument
  // For loads/stores, we process insn later because we need the physical address
  for(int i = 1; i < n; i++) {
    if(buf[i] == -1) {
      is_mem_insn = true;
      return;
    }
  }

  // Check for destination register
  int dst = insn.rd();
  if(dst <= 0 || dst != buf[0]) return;

  // Check for duplicate pc
  if((n >= 2 && regs[buf[1]] && regs[buf[1]]->pc == pc) ||
     (n >= 3 && regs[buf[2]] && regs[buf[2]]->pc == pc))
  {
    return;
  }

  // Check for null propogation
  if(n == 2 && buf[1] == dst)
    return;

  // Only registers here, set up a node
  node_t* node = (node_t*) malloc(sizeof(node_t));
  node->val = dst;
  node->insn = insn;
  node->is_mem = 0;
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
  node->pc = pc;
  node->refc = 1;

  // Replace the old node for this register

  cleanup(regs[dst]);
  regs[dst] = node;
  if(debug) {
    print(node);
  }
}

void tracker_t::print(node_t *node, int depth, int addr_depth) {
  if(node == NULL) return;
  for(int i = 0; i < depth; i++)
    fprintf(stderr, "  ");
  if(depth == MAX_DEPTH) {
    fprintf(stderr, "...\n");
    return;
  }
  if(node->val < NXPR)
    fprintf(stderr, "(insn = %s, reg = %s, pc = 0x%016" PRIx64 ")\n", disasm->lookup_name(node->insn), xpr_name[node->val], node->pc);
  else
    fprintf(stderr, "(insn = %s, addr = 0x%016" PRIx64 ", pc = 0x%016" PRIx64 ")\n", disasm->lookup_name(node->insn), node->val, node->pc);
  print(node->c1, depth+1, addr_depth);
  // Keep track of number of times we go to addr subtrees
  addr_depth += node->is_mem;
  if(addr_depth <= MAX_ADDR_DEPTH)
    print(node->c2, depth+1, addr_depth);
}

void tracker_t::monitor() {
  proc->monitor();
}

void tracker_t::cleanup(node_t *node) {
  if(node == NULL) return;
  std::vector<node_t*> queue = std::vector<node_t*>();
  queue.push_back(node);

  while(!queue.empty()) {
    node = queue.back();
    queue.pop_back();
    if(node == NULL) continue;
    node->refc--;
    if(node->refc == 0) {
      queue.push_back(node->c1);
      queue.push_back(node->c2);
      free(node);
    }
  }
}
