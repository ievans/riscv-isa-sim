reg_t tmp = npc;
// check the tag on the jump target to make sure it is ok to execute 
// but only if RS1 is ra, the conventional return pointer register
if (!(TAG_S1 & TAG_PC) && (RS1 == 1)) {
  throw trap_tag_violation();
}
set_pc((RS1 + insn.i_imm()) & ~reg_t(1));
WRITE_RD_AND_TAG(tmp, TAG_PC);
