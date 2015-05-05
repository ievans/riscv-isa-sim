#include <stdio.h>

void usage(void)
{
  printf("usage: this test tries to store data in the lowest bit of a function pointer and use it\n");
  printf("---------------------------------------------------------------------------------------\n");
}

void tester(void)
{
  printf("a test function was called\n");
}

int main(void)
{
  usage();

  // make the fptr and increment it (use lowest bit)
  void (*fp)(void) = &tester;
  fp++;

  // try to call it
  (fp)();

  // if we get here, then we have not crashed
  printf("test completed\n");

  return 0;
}
