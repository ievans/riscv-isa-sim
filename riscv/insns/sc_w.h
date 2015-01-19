if (RS1 == p->get_state()->load_reservation)
{
  MMU.store_tagged_uint32(RS1, RS2, TAG_S2);
  WRITE_RD_AND_TAG(0, TAG_NULL);
}
else
  WRITE_RD_AND_TAG(1, TAG_NULL);
