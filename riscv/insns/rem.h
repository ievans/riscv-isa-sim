tagged_sreg_t lhs;
lhs.val = sext_xprlen(RS1);
lhs.tag = TAG_S1;

tagged_sreg_t rhs;
rhs.val = sext_xprlen(RS2);
rhs.tag = TAG_S2;

if(rhs.val == 0)
  WRITE_RD_AND_TAG(lhs.val, TAG_ARITH_IMMEDIATE(lhs.tag));
else if(lhs.val == INT64_MIN && rhs.val == -1)
  WRITE_RD_AND_TAG(0, TAG_NULL);
else
  WRITE_RD_AND_TAG(sext_xprlen(lhs.val % rhs.val), TAG_ARITH(lhs.tag, rhs.tag));
