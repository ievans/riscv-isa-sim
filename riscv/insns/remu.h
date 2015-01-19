tagged_reg_t lhs;
lhs.val = zext_xprlen(RS1);
lhs.tag = TAG_S1;
tagged_reg_t rhs;
rhs.val = zext_xprlen(RS2);
rhs.tag = TAG_S2;

if(rhs.val == 0)
  WRITE_RD_AND_TAG(sext_xprlen(RS1), TAG_ARITH_IMMEDIATE(TAG_S1));
else
  WRITE_RD_AND_TAG(sext_xprlen(lhs.val % rhs.val), TAG_ARITH(lhs.tag, rhs.tag));
