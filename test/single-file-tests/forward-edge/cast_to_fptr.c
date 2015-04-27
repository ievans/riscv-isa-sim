#include <stdio.h>
#include <assert.h>
#include "../../tag-extensions.h"

// This test casts an address (stringified to lose any
// possible tag) to a function pointer and tries to call it.
// If we're enforcing strict semantics, RISCV-tagged should allow this.

void usage(void) {
  printf("desired output is: function call success and return 0\n");
  printf("-----------------------------------------------------\n");
}

int iAmAFunction(void) {
  printf("function call success\n");
  return 111;
}

int main(void)
{
  // Print usage message.
  usage();

  tag_enforcement_on();  

  // Set up fptrs.
  int (*fp1)(void) = &iAmAFunction;
  int (*fp2)(void);

  // Stringify the fptr.
  char str[16];
  sprintf(str, "%p", fp1);
  unsigned long str_num = strtoul(str, NULL, 16);
  fp2 = (int(*)(void)) str_num; 
  
  // Make sure the function call succeeds.
  assert((*fp2)() == 111);

  tag_enforcement_off(); // only if no tag trap
  return 0;
}
