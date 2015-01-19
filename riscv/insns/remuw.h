require_xpr64;

tagged_reg_t lhs;
lhs.val = zext32(RS1);
lhs.tag = TAG_S1;
tagged_reg_t rhs;
rhs.val = zext32(RS2);
rhs.tag = TAG_S2;

if(rhs.val == 0)
  WRITE_RD_AND_TAG(sext32(lhs.val), TAG_ARITH_IMMEDIATE(lhs.tag));
else
  WRITE_RD_AND_TAG(sext32(lhs.val % rhs.val), TAG_ARITH(lhs.tag, rhs.tag));
