#include <stdio.h>

char buf[8192];

void
cat(FILE* f, char *s)
{
	char *str;
	int r;

	while ((str = fgets(buf, (long)sizeof(buf), f)) != 0)
		printf("%s", buf);
}

int
main(int argc, char **argv)
{
	FILE *f;
	int i;

	if (argc == 1)
		printf("cat: stdin not supported\n");
	else
		for (i = 1; i < argc; i++) {
			f = fopen(argv[i], "r");
			if (f <= 0)
				printf("cat: can't open %s\n", argv[i]);
			else {
				cat(f, argv[i]);
				fclose(f);
			}
		}

	return 0;
}
