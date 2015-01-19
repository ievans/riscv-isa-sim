reg_t lhs = zext_xprlen(RS1);
reg_t rhs = zext_xprlen(RS2);
if(rhs == 0)
  WRITE_RD_AND_TAG(UINT64_MAX, TAG_ARITH(TAG_S1, TAG_S2));
else
  WRITE_RD_AND_TAG(sext_xprlen(lhs / rhs), TAG_ARITH(TAG_S1, TAG_S2));
