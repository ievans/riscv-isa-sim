void tag_enforcement_on() {
    __asm__ ("tagenforce ra,1");
}

// For now...
void tag_enforcement_off() {
	__asm__ ("tagenforce ra,0");
}

// I don't understand GCC inline assembly...
void set_tag_t3() {
	__asm__ ("settag t3,537");
}

void clear_tag_t3() {
	__asm__ ("settag t3,0");
}
