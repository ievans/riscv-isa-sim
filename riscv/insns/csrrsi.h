int csr = validate_csr(insn.csr(), true);
reg_t old = p->get_pcr(csr);
p->set_pcr(csr, old | insn.rs1());
WRITE_RD_AND_TAG(sext_xprlen(old), TAG_CSR);
