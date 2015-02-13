#include <stdio.h>

int
main(int argc, char **argv)
{
	FILE* f;
	int i;

	// The following was inspired from cat.c
	if (argc == 1) {
		printf("usage: touch FILE1 [FILE2] [FILE3] ...\n");
	} else {
		for (i = 1; i < argc; i++) {
			f = fopen(argv[i], "w");
			if (f <= 0) {
				printf("can't create %s\n", argv[i]);
			} else {
				fclose(f);
			}

		}
	}

	return 0;
}