tagged_reg_t v = MMU.load_tagged_int32(RS1 + insn.i_imm());
WRITE_RD_AND_TAG(sreg_t(v.val), v.tag);
