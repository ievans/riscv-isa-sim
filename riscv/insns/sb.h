tag_t tag = TAG_S2;

#ifdef TAG_POLICY_NO_RETURN_COPY
// If we're storing the value from RA into memory, clear the tag 
if (TAG_ENFORCE_ON && insn.rs2() == RETURN_REGISTER) {
    tag = CLEAR_RA_TAG(tag);
}
#endif

MMU.store_tagged_uint8(RS1 + insn.s_imm(), RS2, tag);
