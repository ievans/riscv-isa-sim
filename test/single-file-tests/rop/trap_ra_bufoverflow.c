#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <setjmp.h>
#include "../../tag-extensions.h"

// This test executes a simple stack buffer overflow on
// a return address, calling setjmp afterwards to see if
// setjmp allows tag violations.
// On RISCV-tagged with correct linearity this should throw a trap.
// Note: on gcc you may need to use the -fno-stack-protector flag
// for this test to actually work.

uint64_t globalBuf[4];
jmp_buf jmpb;

void usage(void)
{
  printf("desired output is:\n");
  printf("no output, and a tag trap\n");
  printf("-------------------------\n");
}

void evilFunc(void)
{
  abort();
}

void test(void)
{
  // Small buffer to overflow.
  char buf[8];

  // Get the addr of our evil func.
  globalBuf[3] = (uint64_t) &evilFunc;
  // Some other garbage.
  globalBuf[0] = 1337;
  
  // Do the buffer overwrite (size is too large here).
  memcpy(buf, globalBuf, sizeof(globalBuf));
  
  // We should not return to the evil function.
}

int main(void)
{
  // Print usage message.
  usage();

  tag_enforcement_on();

  test();

  tag_enforcement_off(); // only if no tag trap

  return 0;
}
