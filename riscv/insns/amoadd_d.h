require_xpr64;
reg_t v = MMU.load_uint64(RS1);
reg_t tag = MMU.load_tag(RS1);
MMU.store_uint64(RS1, RS2 + v);
MMU.store_tag(TAG_UNION(tag, RS2_TAG); RS1, RS2 + v);
WRITE_RD_AND_TAG(v, TAG_UNION(tag, RS2_TAG);
