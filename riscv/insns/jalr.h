reg_t tmp = npc;
// check the tag on the jump target to make sure it is ok to execute
// but only if RS1 is ra, the conventional return pointer register
if ((1 || TAG_ENFORCE_ON) &&
    (!(TAG_S1 & TAG_PC)) &&
    (insn.rs1() == 1) &&
    (IS_SUPERVISOR == false)) {
    // need to sub 4 from already incremented pc.
    printf("trap would have happened at pc %08lx: \n", npc-4);
    //throw trap_tag_violation();
}
if(TAG_S1 & TAG_DATA) {
    printf("jumping to data at pc %08lx: \n", npc-4);
    MMU.monitor();
}
set_pc((RS1 + insn.i_imm()) & ~reg_t(1));
WRITE_RD_AND_TAG(tmp, TAG_PC);
