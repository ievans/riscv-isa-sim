p->get_state()->load_reservation = RS1;
tagged_reg_t v = MMU.load_tagged_int32(RS1);
WRITE_RD_AND_TAG(sreg_t(v.val), v.tag);
