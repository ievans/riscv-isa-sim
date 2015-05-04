// Fork a binary tree of processes and display their structure.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define DEPTH 3

void forktree(const char *cur);

void
forkchild(const char *cur, char branch)
{
	char nxt[DEPTH+1];

	if (strlen(cur) >= DEPTH)
		return;

	snprintf(nxt, DEPTH+1, "%s%c", cur, branch);
	if (fork() == 0) {
		forktree(nxt);
		exit(0);
	}
}

void
forktree(const char *cur)
{
	printf("%04x: I am '%s'\n", getpid(), cur);

	forkchild(cur, '0');
	forkchild(cur, '1');
}

int
main(int argc, char **argv)
{
	forktree("");
	return 0;
}

