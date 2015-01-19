require_xpr64;
tagged_reg_t v = MMU.load_tagged_uint64(RS1);
MMU.store_tagged_uint64(RS1, RS2 | v.val, TAG_LOGIC(v.tag, TAG_S2));
WRITE_RD_AND_TAG(v.val, v.tag);
