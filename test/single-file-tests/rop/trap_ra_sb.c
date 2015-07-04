#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#define DEBUG 0

int main();

void fake_syscall() {
	abort();
}

void c() {}

void a() {
	c();
	char x[16];
	int64_t i;
	char *str1, *str2;

#if DEBUG
        printf("%016" PRIx64 " %016" PRIx64 " %016" PRIx64 "\n", (uint64_t) &fake_syscall, (uint64_t) &a, (uint64_t) &main);
        for(i = -10; i <= 10; i++) {
                printf("%d:\t%016" PRIx64 "\n", i, ((uint64_t*) x)[i]);
        }
	printf("\n");
#endif

	// Set the return address of a() to be the address of fake syscall
	uint64_t addr = (uint64_t) &fake_syscall;
	#define RA_OFFSET 56
	for(i = 0; i < 8; i++)
		x[RA_OFFSET + i] = (addr >> 8 * i) & 0xff;
	// Also overflow a and b
	str1 = x + RA_OFFSET + 7;
	str2 = str1 + 16;

	// Now attempt to fix the tag
	*str1 = *str2;

#if DEBUG
	printf("%016" PRIx64 " %016" PRIx64 " %016" PRIx64 "\n", (uint64_t) &fake_syscall, (uint64_t) &a, (uint64_t) &main);
	for(i = -10; i <= 10; i++) {
		printf("%d:\t%016" PRIx64 "\n", i, ((uint64_t*) x)[i]);
	}
	printf("\n");
#endif
}

void b() {
	a();
}

int main() {
	b();
	return 0;
}
