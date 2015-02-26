reg_t addr = RS1 + insn.i_imm();
tagged_reg_t v = MMU.load_tagged_uint16(addr);
#ifdef TAG_POLICY_NO_RETURN_COPY
tag_t tag = v.tag;
if ((TAG_ENFORCE_ON)
  && (insn.rd() != RETURN_REGISTER)
)
{
  // should we clear this tag?
  // CLEAR_TAG(insn.rd(), v.tag);
  MMU.store_tag_value(CLEAR_PC_TAG(tag), addr);
}
#endif
WRITE_RD_AND_TAG(v.val, v.tag);
