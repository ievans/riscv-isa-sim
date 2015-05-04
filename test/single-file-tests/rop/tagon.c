#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "../../tag-extensions.h"

int main(int argc, char *argv[])
{
    // turn tag enforcement on then execute target program
    tag_enforcement_on();
    execvp(argv[1], &argv[1]);
    perror("exec failure");
    exit(1);
}
