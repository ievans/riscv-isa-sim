// See LICENSE for license details.

#ifndef _RISCV_PTAXI_SIM_H
#define _RISCV_PTAXI_SIM_H

#include "decode.h"
#include "mmu.h"
#include "processor.h"
#include "ptaxi-common.h"
#include <vector>
#include <utility>

struct ptaxi_policy_context_t {
  ptaxi_policy_t policy;
  size_t match_count;
};
struct ptaxi_context_state_t {
  std::vector<ptaxi_policy_context_t> policy_contexts;
  bool is_enable;
};

#define TAG_RET_FROM_JAL 1
#define TAG_RET_FROM_MEM 2

enum insn_var_type_t {
  INSN_OUT, INSN_ARG1, INSN_ARG2,
};

class ptaxi_sim_t {
public:
  ptaxi_sim_t();
  reg_t execute_insn(processor_t *p, reg_t pc, insn_fetch_t fetch);
  void add_policy(processor_t *p, uint64_t a, uint64_t b);
  void run_tag_command(processor_t *p, uint64_t cmd);
private:
  void print_policies(size_t context_id);
  ptaxi_insn_type_t get_insn_type(insn_t insn);
  size_t get_ptaxi_context_id(processor_t *p, bool add_if_needed);
  std::pair<ptaxi_action_t, int> determine_ptaxi_action(processor_t *p, insn_t insn, reg_t pc);
  uint8_t get_or_set_tag(processor_t *p, insn_t insn, reg_t pc, ptaxi_insn_type_t insn_type,
      insn_var_type_t var_type, bool set_tag, uint8_t tag_val);
  uint8_t load_tag_from_mem(processor_t *p, uint64_t addr, uint8_t rm);
  void store_tag_to_mem(processor_t *p, uint64_t addr, uint8_t rm, uint64_t val);
  tagged_reg_t v;
  // states[0] is a default policy template.
  std::vector<ptaxi_context_state_t> states;
};
#endif
