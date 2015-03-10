reg_t addr = RS1 + insn.i_imm();
tagged_reg_t v = MMU.load_tagged_int16(addr);
#ifdef TAG_POLICY_NO_RETURN_COPY
// Clear PC_TAG from the memory location if present.
tag_t cleared_tag = CLEAR_PC_TAG(v.tag);
if (cleared_tag != v.tag) {
  MMU.store_tag_value(cleared_tag, addr);
}
if ((TAG_ENFORCE_ON)
  && (insn.rd() != RETURN_REGISTER)
)
{ 
  // Loading into a non-return register,
  // must clear PC tag in destination register.
  v.tag = cleared_tag;
}
#endif // TAG_POLICY_NO_RETURN_COPY
WRITE_RD_AND_TAG(v.val, v.tag);
