// from wikipedia

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define SIGCAUGHT "Interactive attention signal caught."
 
static void catch_function(int signo) {
  write(1, SIGCAUGHT, sizeof(SIGCAUGHT));
}
 
int main(void) {
  if (signal(SIGINT, catch_function) == SIG_ERR) {
    fputs("An error occurred while setting a signal handler.\n", stderr);
    return EXIT_FAILURE;
  }
  puts("Raising the interactive attention signal.");
  if (raise(SIGINT) != 0) {
    fputs("Error raising the signal.\n", stderr);
    return EXIT_FAILURE;
  }
  puts("Exiting.\n");
  return EXIT_SUCCESS;
}
