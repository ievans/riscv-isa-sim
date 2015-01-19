sreg_t lhs = sext_xprlen(RS1);
sreg_t rhs = sext_xprlen(RS2);
if(rhs == 0)
  WRITE_RD_AND_TAG(UINT64_MAX, TAG_ARITH(TAG_S1, TAG_S2));
else if(lhs == INT64_MIN && rhs == -1)
  WRITE_RD_AND_TAG(lhs, TAG_ARITH(TAG_S1, TAG_S2));
else
  WRITE_RD_AND_TAG(sext_xprlen(lhs / rhs), TAG_ARITH(TAG_S1, TAG_S2));
