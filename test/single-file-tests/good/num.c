#include <stdio.h>

int bol = 1;
int line = 0;

void
num(FILE* f, char *s)
{
	int str;
	int r;
	char c;

	while ((str = fgetc(f)) != EOF) {
		printf("wee\n");
		if (bol) {
			printf("%5d ", ++line);
			bol = 0;
		}
		if ((r = printf("%c", str)) != 1)
			printf("write error copying %s\n", s);
			return;
		if (c == '\n')
			bol = 1;
	}
}

int
main(int argc, char **argv)
{
	FILE* f;
	int i;

	if (argc == 1)
		printf("num: stdin not supported\n");
	else
		for (i = 1; i < argc; i++) {
			f = fopen(argv[i], "r");
			if (f <= 0)
				printf("can't open %s\n", argv[i]);
			else {
				num(f, argv[i]);
				fclose(f);
			}
		}

	return 0;
}

