require_xpr64;
WRITE_RD_AND_TAG(sext32(insn.i_imm() + RS1), TAG_ADD_IMMEDIATE(TAG_S1));
