tagged_reg_t v = MMU.load_tagged_int16(RS1 + insn.i_imm());
WRITE_RD_AND_TAG(v.val, v.tag);
