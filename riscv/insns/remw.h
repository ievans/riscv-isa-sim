require_xpr64;

tagged_sreg_t lhs;
lhs.val = sext32(RS1);
lhs.tag = TAG_S1;
tagged_sreg_t rhs;
rhs.val = sext32(RS2);
rhs.tag = TAG_S2;

if(rhs.val == 0)
  WRITE_RD_AND_TAG(lhs.val, TAG_ARITH_IMMEDIATE(lhs.tag));
else
  WRITE_RD_AND_TAG(sext32(lhs.val % rhs.val), TAG_ARITH(lhs.tag, rhs.tag));
