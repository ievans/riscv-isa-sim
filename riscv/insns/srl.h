if(xpr64)
  WRITE_RD_AND_TAG(RS1 >> (RS2 & 0x3F), TAG_LOGIC(RS1, RS2));
else
  WRITE_RD_AND_TAG(sext32((uint32_t)RS1 >> (RS2 & 0x1F)), TAG_LOGIC(RS1, RS2));
