// test reads and writes to a large bss

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define ARRAYSIZE (1024*1024)

uint32_t bigarray[ARRAYSIZE];

int
main(int argc, char **argv)
{
	int i;

	printf("Making sure bss works right...\n");
	for (i = 0; i < ARRAYSIZE; i++)
		if (bigarray[i] != 0) {
			printf("bigarray[%d] isn't cleared!\n", i);
			exit(1);
		}
	for (i = 0; i < ARRAYSIZE; i++)
		bigarray[i] = i;
	for (i = 0; i < ARRAYSIZE; i++)
		if (bigarray[i] != i) {
			printf("bigarray[%d] didn't hold its value!\n", i);
			exit(2);
		}

	printf("Yes, good.  Now doing a wild write off the end...\n");
	bigarray[ARRAYSIZE+1024] = 0;
	printf("SHOULD HAVE TRAPPED!!!\n");
}
