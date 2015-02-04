#include <stdio.h>
#include <unistd.h>

int
main(int argc, char **argv)
{
	int i;

	printf("starting count down:\n");
	for (i = 5; i >= 0; i--) {
		printf("%d\n", i);
		sleep(1);
	}

	return 0;
}
