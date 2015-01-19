require_xpr64;
WRITE_RD_AND_TAG(sext32(RS1 << (RS2 & 0x1F)), TAG_LOGIC(TAG_S1, TAG_S2));
