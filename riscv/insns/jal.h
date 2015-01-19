reg_t tmp = npc;
set_pc(JUMP_TARGET);
WRITE_RD_AND_TAG(tmp, TAG_PC);
