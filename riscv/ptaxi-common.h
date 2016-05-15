#ifndef _PTAXI_COMMON_H
#define _PTAXI_COMMON_H

#include <stdint.h>

enum ptaxi_insn_type_t {
  INSN_TYPE_UNKNOWN,
  INSN_TYPE_LOAD,
  INSN_TYPE_LOAD64,
  INSN_TYPE_STORE,
  INSN_TYPE_STORE64,
  INSN_TYPE_COPY,
  INSN_TYPE_OP,
  INSN_TYPE_OPIMM,
  INSN_TYPE_JAL,
  INSN_TYPE_JALR,
  INSN_TYPE_RETURN,
  INSN_TYPE_TAGCMD,
  INSN_TYPE_TAGPOLICY,
};

enum {
  PTAXI_ACTION_CONTINUE = 0, // = no action
  PTAXI_ACTION_ALLOW = 1,
  PTAXI_ACTION_BLOCK = 2,
  PTAXI_ACTION_GC = 4,
  PTAXI_ACTION_CALL = 8,
  PTAXI_ACTION_DEBUG_LINE = 16,
  PTAXI_ACTION_DEBUG_DETAIL = 32,
};

typedef uint8_t ptaxi_action_t;

struct ptaxi_policy_t {
  // Filter
  enum ptaxi_insn_type_t insn_type :8;
  uint8_t rs1_mask;
  uint8_t rs1_match;
  uint8_t rs2_mask;
  uint8_t rs2_match;
  uint8_t tag_arg1_mask;
  uint8_t tag_arg1_match;
  uint8_t tag_arg2_mask;
  uint8_t tag_arg2_match;
  uint8_t tag_out_mask;
  uint8_t tag_out_match;

  // Action
  ptaxi_action_t action :8;
  uint8_t tag_out_set;
  uint8_t tag_out_tomodify;
  uint8_t ignore_count;
};

union ptaxi_policy_serialized {
  struct ptaxi_policy_t policy;
  struct ptaxi_policy_serialized_result {
    uint64_t a;
    uint64_t b;
  } regs;
};
#endif
