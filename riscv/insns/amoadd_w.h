tagged_reg_t v = MMU.load_tagged_int32(RS1);
MMU.store_tagged_uint32(RS1, RS2 + v);
WRITE_RD(v);
