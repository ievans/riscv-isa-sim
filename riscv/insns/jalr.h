reg_t tmp = npc;
// check the tag on the jump target to make sure it is ok to execute
// but only if RS1 is ra, the conventional return pointer register
if ((1 || TAG_ENFORCE_ON) &&
    (!(TAG_S1 & TAG_PC)) &&
    (insn.rs1() == RETURN_REGISTER) &&
    (IS_SUPERVISOR == false)) {
    // need to sub 4 from already incremented pc.
    printf("trap would have happened at pc %08lx: no ra tag\n", npc-4);
    // throw trap_tag_violation();
}
#ifdef TAG_POLICY_FPTR_TAGS
if ((1 || TAG_ENFORCE_ON) &&
    (!(TAG_S1 & TAG_FPTR)) &&
    (insn.rs1() != RETURN_REGISTER) &&
    (IS_SUPERVISOR == false)) {
    // tag on calling register is not fptr
    printf("trap would have happened at pc %08lx: no fptr tag\n", npc-4);
    // throw trap_tag_violation();
}
#endif
set_pc((RS1 + insn.i_imm()) & ~reg_t(1));
WRITE_RD_AND_TAG(tmp, TAG_PC);
