require_xpr64;
reg_t lhs = zext32(RS1);
reg_t rhs = zext32(RS2);
if(rhs == 0)
  WRITE_RD_AND_TAG(UINT64_MAX, TAG_ARITH(TAG_S1, TAG_S2));
else
  WRITE_RD_AND_TAG(sext32(lhs / rhs), TAG_ARITH(TAG_S1, TAG_S2));
