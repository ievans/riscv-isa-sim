#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "libspike.h"

#define MAX_FILEPATH_LEN 64
#define EXPECT_RETURN_0 0
#define EXPECT_SIGBUS 1
#define EXPECT_SIGBUS_PREFIX "trap"
#define EXIT_TEST_FAILED 1
#define EXIT_TEST_SUCCEED 0
#define EXIT_ERROR_PARENT -1
#define EXIT_ERROR_CHILD -2

void usage(void)
{
  printf( "usage: run_gcc_torture_tests FOLDER\n"
      "will run all executables in the folder provided,\n"
      "printing out the return values of each executable run.\n"
      "note that FOLDER is an ABSOLUTE path.\n" );
}

int main(int argc, char *argv[])
{
  // Check if args are provided.
  if (argc <= 1 || argc > 2) {
    usage();
    return 0;
  }

  // Parse argument.
  char* path_to_executables = argv[1];

  // Keep track of tests that passed.
  size_t tests_passed = 0;

  // Open the directory.
  DIR* dir = opendir(path_to_executables);
  if (dir == NULL) {
    perror("opendir");
    return EXIT_ERROR_PARENT;
  }

  // Loop through directory entries, looking for executable files
  struct dirent* dent;
  size_t testno = 0;

  while ((dent = readdir(dir)) != NULL) {
    // Ignore dot files.
    if (strncmp(dent->d_name, ".", 1) == 0) {
      continue;
    }

    // Control testing mode.
    int testmode = EXPECT_RETURN_0;
    size_t prefix_len = sizeof(EXPECT_SIGBUS_PREFIX);
    if (strlen(dent->d_name) >= prefix_len && strncmp(dent->d_name,
      EXPECT_SIGBUS_PREFIX, prefix_len) == 0) {
      testmode = EXPECT_SIGBUS;
    }

    // Try to execute this entry; use exec failure as indication
    // that file is not executable.
    pid_t pid = fork();
    int retcode;
    char filepath[MAX_FILEPATH_LEN];
    switch (pid) {
    case -1:
      perror("fork");
      return EXIT_ERROR_PARENT;

    case 0:
      // Child, make filename for executable and call exec().
      snprintf(filepath, MAX_FILEPATH_LEN,
        "%s/%s", path_to_executables, dent->d_name);
      fprintf(stderr, "child executing: %s as test %lu\n", filepath, testno);

      if (execl(filepath, dent->d_name, (char *) 0) < 0) {
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
          fprintf(stderr, "Test %lu failed, returned %d\n",
            testno, WEXITSTATUS(retcode));
        } else {
          fprintf(stderr, "Test %lu got expected return 0\n", testno);
          tests_passed++;
        }
      } else if (WIFSIGNALED(retcode)) {
        // Test was killed by signal (if program did something illegal),
        // count this as test failure if we're not expecting it.
        if (testmode == EXPECT_SIGBUS && WTERMSIG(retcode) == SIGBUS) {
          fprintf(stderr, "Test %lu got expected sigbus\n", testno);
          tests_passed++;
        } else {
          fprintf(stderr, "Test %lu failed, killed by signal %d\n",
            testno, WTERMSIG(retcode));
        }
      }
        break;
    }
    testno++;
  }

  // Close directory, and if a test failed, tell spike to quit with that status.
  closedir(dir);
  fprintf(stderr, "%lu / %lu tests passed\n", tests_passed, testno);

  libspike_args_t* args = libspike_get_args();
  if (tests_passed < testno) {
    args->arg0 = EXIT_TEST_FAILED;
  } else {
    args->arg0 = EXIT_TEST_SUCCEED;
  }
  libspike_exit_with_retcode();

  // We shouldn't get here.
  return 0;
}
