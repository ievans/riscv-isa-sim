reg_t tag = TAG_S2;

//#ifdef TAG_POLICY_NO_PARTIAL_COPY
if (tag_policy_no_partial_copy)
{
  tag = CLEAR_PC_TAG(tag);
}
//#endif

MMU.store_tagged_uint32(RS1 + insn.s_imm(), RS2, tag);
// If we're storing the return address into memory...
if (tag_policy_no_return_copy) {
  //#ifdef TAG_POLICY_NO_RETURN_COPY
  if ((TAG_ENFORCE_ON)
    && (insn.rs2() == RETURN_REGISTER) && (!IS_SUPERVISOR)
  )
  {
    // ensure we have one live PC tag by
    // clearing the tag in the source register.
    CLEAR_TAG(RETURN_REGISTER, TAG_PC);
  }
  //#endif
}
