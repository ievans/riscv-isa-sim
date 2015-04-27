#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../../tag-extensions.h"

// This test checks that a fptr that can point to many functions
// does not generate any traps on our riscv-tagged machine.

void usage(void)
{
  printf("desired output is:\n");
  printf("no output\n");
  printf("------------------\n");
}

int firstIndexOf(const char ch, const char* str) {
  if (!str) {
    return -2;
  }

  size_t i;
  size_t len = strlen(str);
  for (i = 0; i < len; ++i) {
    if (str[i] == ch) {
      return i;
    }
  }

  // not found
  return -1;
}

int lastIndexOf(const char ch, const char* str) {
  if (!str) {
    return -2;
  }

  size_t i;
  size_t len = strlen(str);
  for (i = len - 1; i >= 0; --i) {
    if (str[i] == ch) {
      return i;
    }
  }

  return -1;
}

// Global fptr.
int (*gfp)(const char, const char*);

int main(void)
{
  // Print usage message.
  usage();

  tag_enforcement_on();

  // Local fptr.
  int (*lfp)(const char, const char*) = &lastIndexOf;

  #define LCHAR 's'
  #define LSTR "msonkeys"

  // Call fptr and then set it to 0.
  assert((*lfp)(LCHAR, LSTR) == 7);
  lfp = 0;
  assert(lfp == 0);

  // Set global fptr to 0.
  gfp = lfp;
  assert(gfp == 0);

  // Set gfp & lfp to different values.
  lfp = &firstIndexOf;
  gfp = &lastIndexOf;
  assert((*gfp)(LCHAR, LSTR) == 7);
  assert(gfp != lfp);

  // Set gfp & lfp to same value and test output.
  gfp = lfp;
  assert(gfp == lfp);
  assert((*gfp)(LCHAR, LSTR) == 1);
  assert((*lfp)(LCHAR, LSTR) == 1);

  tag_enforcement_off(); // only if no tag trap

  return 0;
}
