#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <inttypes.h>

int main(void)
{
  uint64_t* ptr = (uint64_t*) malloc(4);
  
  printf("ptr is at: %p\n", ptr);
  sleep(3);

  *ptr = 3;
  printf("ptr now should hold 3\n");
  sleep(3);

  *ptr = 27;
  printf("ptr now should hold 27\n");
  sleep(3);  

  printf("freeing ptr\n");
  free(ptr);

  return 0;
}
