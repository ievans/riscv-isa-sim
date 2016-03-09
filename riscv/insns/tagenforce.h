reg_t old = p->get_pcr(CSR_STATUS);
p->set_pcr(CSR_STATUS, old | SR_TAG);
WRITE_RD_AND_TAG(sext_xprlen(old), TAG_CSR);
