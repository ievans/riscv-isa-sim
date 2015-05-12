#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define SIGCAUGHT "Caught tag violation signal!\n"
 
static void catch_function(int signo) {
  write(1, SIGCAUGHT, sizeof(SIGCAUGHT));
  _Exit(EXIT_SUCCESS);
}
 
int main(void) {
  if (signal(SIGUSR2, catch_function) == SIG_ERR) {
    fputs("An error occurred while setting a signal handler.\n", stderr);
    return EXIT_FAILURE;
  }

  // Generate tag violation signal with raise().
  puts("Raising the tag violation signal with raise().\n");
  if (raise(SIGUSR2) != 0) {
    fputs("Error raising the signal.\n", stderr);
    return EXIT_FAILURE;
  }

  // Shouldn't get here.
  puts("Exiting.\n");
  return EXIT_SUCCESS;
}
