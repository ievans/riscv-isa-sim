#include <stdio.h>
#include <stdint.h>
#include "../../tag-extensions.h"

// This test casts a function pointer to an integral type and
// casts it back to a function pointer type.
// The point of this test is to make whatever the defined
// behavior of casting function pointer types is clear.

void usage(void)
{
  printf("desired output is:\n");
  printf("16\n");
  printf("then if fptr casts do nothing, 36\n");
  printf("else, a tag trap\n");
  printf("---------------------------------\n");
}

int aTestFunc(int a) {
  return a * a;
}

int main(void)
{
  // Print usage message.
  usage();

  tag_enforcement_on();

  // Create function pointer and call it.
  int (*fp)(int) = &aTestFunc;
  printf("%d\n", (*fp)(4));

  // Now, cast the function pointer away.
  uintptr_t fp_addr = (uintptr_t) fp;
  fp_addr += 2;

  // Cast it back to a function pointer.
  int (*fp2)(int) = (int (*)(int)) (fp_addr - 2);

  // Try to call it again:
  printf("%d\n", (*fp2)(6));

  tag_enforcement_off(); // only if no tag trap

  return 0;
}
