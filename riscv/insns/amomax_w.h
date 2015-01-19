// 'v' was an int32_t
tagged_reg_t v = MMU.load_tagged_int32(RS1);
MMU.store_tagged_uint32(RS1, std::max(int32_t(RS2), int32_t(v.val)), TAG_LOGIC(v.tag, TAG_S2));
WRITE_RD_AND_TAG(int32_t(v.val), v.tag);
