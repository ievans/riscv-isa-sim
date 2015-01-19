int csr = validate_csr(insn.csr(), insn.rs1() != 0);
reg_t old = p->get_pcr(csr);
p->set_pcr(csr, old | RS1);
WRITE_RD_AND_TAG(sext_xprlen(old), TAG_CSR);
