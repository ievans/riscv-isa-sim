reg_t tag = TAG_S2;
reg_t addr = RS1 + insn.s_imm();

#ifdef TAG_POLICY_NO_PARTIAL_COPY
tag = TAG_DATA;

if((TAG_S1 & TAG_DATA) && !(addr & 0x8000000000000000L)) {
    printf("halfword store trap at addr %016" PRIx64 ", pc %08lx: \n", addr, npc-4);
    MMU.monitor();
}
#endif

MMU.store_tagged_uint16(addr, RS2, tag);
// If we're storing the return address into memory...
#ifdef TAG_POLICY_NO_RETURN_COPY
if ((TAG_ENFORCE_ON)
  && (insn.rs2() == RETURN_REGISTER) && (!IS_SUPERVISOR)
)
{
  // ensure we have one live PC tag by
  // clearing the tag in the source register.
  CLEAR_TAG(RETURN_REGISTER, TAG_PC);
}
#endif
