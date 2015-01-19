require_xpr64;
// 'v' used to be an sreg_t
tagged_reg_t v = MMU.load_tagged_int64(RS1);
MMU.store_tagged_uint64(RS1, std::min(sreg_t(RS2),sreg_t(v.val)), TAG_LOGIC(v.val, TAG_S2));
WRITE_RD_AND_TAG(sreg_t(v.val), v.tag);
