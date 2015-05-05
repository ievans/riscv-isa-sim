#include <stdio.h>

void usage(void)
{
  printf("usage: test pointer subtraction on stack variables\n");
  printf("warning: this test may require -fno-stack-protector to work\n");
  printf("-----------------------------------------------------------\n");
}

int main(void)
{
  usage();

  // declarations
  int j = 4, g = 16;

  printf("g is: %d\n", g);

  // use pointer subtraction on the stack to write to g
  int* g_ptr = &j - 1;
  *(g_ptr) = 94;

  // this printf makes sure the stack layout is what we want
  printf("g_ptr is: %p j_addr is: %p\n", g_ptr, &g);

  // print to see if g has changed
  printf("g is now: %d\n", g);

  return 0;
}
