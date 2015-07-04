#include <stdio.h>
#include <stdlib.h>

#define N_ADDRS 28
#define PRINT_DUMP(k) for(i=0; i<N_ADDRS; i++) { printf("%08x ", dump_buffer[N_ADDRS*k + i]); if(i % 4 == 3) {printf("\n");} } printf("\n");
#define DUMP(addr) for(i=0; i<N_ADDRS; i++) { dump_buffer[n + i] = ((int*)(addr))[i]; } n += N_ADDRS;
int i;
int n = 0;
int dump_buffer[10000];

int main();

void b(int arg1) {
	printf("Calling harmless function b() with argument %x\n", arg1);
}

int a(int arg1) {
	int k = arg1 & 0x0f0f0f0f;
	b(0xaaaaaaaa);
	DUMP(&arg1 - N_ADDRS);

	DUMP(&arg1);
	if(k != 0x0e0d0e0f) {
		abort();
	}
}

void do_stuff(int arg1) {
	int local = 0;
	a(arg1);
	// Simulate a replay attack on function a
	int *p = &local;
	DUMP(p - N_ADDRS);
	DUMP(p);
	// Setup the return address
	p[5] = p[-19];
	// Setup the argument too
	p[2] = 0x87654321;
}

int main() {
	do_stuff(0xdeadbeef);
	printf("Addresses b: %x, a: %x, do_stuff: %x, main: %x\n", &b, &a, &do_stuff, &main);
	return 0;
}



