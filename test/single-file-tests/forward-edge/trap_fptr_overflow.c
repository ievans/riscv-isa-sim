#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "../../tag-extensions.h"

// This test executes a simple stack buffer overflow on
// a function pointer to point it to the address of an evil
// function, evilFunc.
// On RISCV-tagged this should throw a trap.
// Note: on gcc you may need to use the -fno-stack-protector flag
// for this test to actually work.

#define GOODSTR "you have saved us all.\n"

void usage(void)
{
  printf("desired output is:\n");
  printf(GOODSTR);
  printf("and a tag trap, with no assertion failure\n");
  printf("-----------------------------------------\n");
}


int evilFunc(void)
{
  abort();
}

int goodFunc(void)
{
  printf(GOODSTR);
  return 1337;
}

uint64_t globalBuf[2];

int main(void)
{
  // Print usage message.
  usage();

  tag_enforcement_on();

  // Set up the good function pointer.
  int (*fp)(void) = &goodFunc;

  // Small buffer to overflow.
  char buf[8];

  // Get the addr of our evil func.
  globalBuf[1] = (uint64_t) &evilFunc;
  // Some other garbage.
  globalBuf[0] = 1337;
  
  // This execution should work always.
  (*fp)();

  // Do the buffer overwrite (size is too large here).
  memcpy(buf, globalBuf, sizeof(globalBuf));
 
  // Try and call the function again...
  assert((*fp)() != 666);

  tag_enforcement_off(); // only if no tag trap
  return 0;
}
