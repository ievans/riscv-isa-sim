// 'v' was a uint32_t type
tagged_reg_t v = MMU.load_tagged_int32(RS1);
MMU.store_tagged_uint32(RS1, std::max(uint32_t(RS2),uint32_t(v.val)), TAG_LOGIC(v.val, TAG_S2));
WRITE_RD_AND_TAG((int32_t)v.val, v.tag);
