#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "../tag-extensions.h"

int main(int argc, char *argv[])
{
    execvp(argv[1], &argv[1]);

    return 0;
}
