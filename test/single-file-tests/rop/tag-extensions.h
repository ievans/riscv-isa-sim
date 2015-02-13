void tag_enforcement_on() {
#ifdef __riscv
    __asm__ ("tagenforce ra,1");
#else
    printf("warning: tag enforcement not available for this architecture");
#endif
}

// For now...
void tag_enforcement_off() {
#ifdef __riscv
	__asm__ ("tagenforce ra,0");
#else
    printf("warning: tag enforcement not available for this architecture");
#endif
}

// I don't understand GCC inline assembly...
void set_tag_t3() {
#ifdef __riscv
	__asm__ ("settag t3,537");
#else
    printf("warning: tag enforcement not available for this architecture");
#endif
}

void clear_tag_t3() {
#ifdef __riscv
	__asm__ ("settag t3,0");
#else
    printf("warning: tag enforcement not available for this architecture");
#endif
}
