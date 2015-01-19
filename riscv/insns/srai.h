if(xpr64)
  WRITE_RD_AND_TAG(sreg_t(RS1) >> SHAMT, TAG_LOGIC_IMMEDIATE(TAG_S1));
else
{
  if(SHAMT & 0x20)
    throw trap_illegal_instruction();
  WRITE_RD_AND_TAG(sext32(int32_t(RS1) >> SHAMT), TAG_LOGIC_IMMEDIATE(TAG_S1));
}
