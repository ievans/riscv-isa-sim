if (xpr64)
  WRITE_RD_AND_TAG(mulh(RS1, RS2), TAG_ARITH(TAG_S1, TAG_S2));
else
  WRITE_RD_AND_TAG(sext32((sext32(RS1) * sext32(RS2)) >> 32), TAG_ARITH(TAG_S1, TAG_S2));
