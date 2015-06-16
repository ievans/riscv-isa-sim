#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "libspike.h"

#define EXIT_TEST_FAILED 1
#define EXIT_TEST_SUCCEED 0
#define EXIT_ERROR_PARENT -1
#define EXIT_ERROR_CHILD -2

void usage(void)
{
  printf( "usage: run_tests FILE\n"
      "will run the file whose path is provided,\n"
      "printing out the return value of the run.\n"
      "note that FILE is an ABSOLUTE path.\n" );
}

int main(int argc, char *argv[])
{
  // Check if args are provided.
  if (argc <= 1 || argc > 2) {
    usage();
    return 0;
  }

  // Parse argument.
  char* path_to_run_tests = argv[1];

  // Exec the run_tests.sh file.
  pid_t pid = fork();
  int retcode, exitcode = EXIT_TEST_FAILED;

  switch (pid) {
  case -1:
    perror("fork");
    return EXIT_ERROR_PARENT;

  case 0:
    if (execl(path_to_run_tests, (char *) 0) < 0) {
      printf("%s:0\n", path_to_run_tests);
      perror("exec");
      _exit(EXIT_ERROR_CHILD);
    }

  default:
    // Parent, wait for child to finish.
    if (waitpid(pid, &retcode, 0) == -1) {
      perror("waitpid");
      return EXIT_ERROR_PARENT;
    } else if (WIFEXITED(retcode)) {
      if (WEXITSTATUS(retcode) != 0) {
        // Test returned non-zero exit code, count as test failure.
        fprintf(stderr, "Test failed, returned %d\n",
          WEXITSTATUS(retcode));
      } else {
        fprintf(stderr, "Test got expected return 0\n");
        exitcode = EXIT_TEST_SUCCEED;
      }
    } else if (WIFSIGNALED(retcode)) {
      // Test was killed by signal (if program did something illegal),
      // count this as test failure if we're not expecting it.
      fprintf(stderr, "Test failed, killed by signal %d\n",
        WTERMSIG(retcode));
    }
    break;
  }

  // Attempt to sync data in buffers to disk before quitting.
  sync();

  // Quit spike with the appropriate return code.
  libspike_args_t* args = libspike_get_args();
  args->arg0 = exitcode;
  libspike_exit_with_retcode();

  // We shouldn't get here.
  return 0;
}
