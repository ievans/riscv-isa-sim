#include <stdio.h>
#include <stdint.h>

void usage(void)
{
  printf("usage: this test tries to store data in the lowest bit of a function pointer and use it\n");
  printf("---------------------------------------------------------------------------------------\n");
}

void tester(void)
{
  printf("a test function was called\n");
}

typedef void (*fptr)(void);

int main(void)
{
  usage();

  // make the fptr and use lowest bit
  fptr fp = &tester;
  uint8_t data = 1;
  fp = (fptr) ((uint64_t) fp | (data & 1));

  // try to call it
  (fp)();

  fp = &tester;
  fp++;
  (fp)();

  // if we get here, then we have not crashed
  printf("test completed\n");

  return 0;
}
