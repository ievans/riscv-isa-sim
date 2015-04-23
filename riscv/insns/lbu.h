reg_t addr = RS1 + insn.i_imm();
tagged_reg_t v = MMU.load_tagged_uint8(addr);
#ifdef TAG_POLICY_NO_RETURN_COPY
if (!IS_SUPERVISOR) {
  // Clear PC_TAG from the memory location if present.
  tag_t cleared_tag = CLEAR_PC_TAG(v.tag);
  if (cleared_tag != v.tag) {
    MMU.store_tag_value(cleared_tag, addr);
  }
}
#endif // TAG_POLICY_NO_RETURN_COPY
WRITE_RD_AND_TAG(v.val, v.tag);
