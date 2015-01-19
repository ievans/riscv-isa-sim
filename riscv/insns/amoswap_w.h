tagged_reg_t v = MMU.load_tagged_int32(RS1);
MMU.store_tagged_uint32(RS1, RS2, TAG_S2);
WRITE_RD_AND_TAG(v.val, v.tag);
