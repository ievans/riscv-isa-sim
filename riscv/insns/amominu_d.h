require_xpr64;
reg_t v = MMU.load_uint64(RS1);
reg_t t = MMU.load_tag(RS1);
MMU.store_uint64(RS1, std::min(RS2,v));
WRITE_RD_AND_TAG(v, TAG_UNION(tag, TAG_S2));
