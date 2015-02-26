#include <stdio.h>
#include <stdlib.h>

int main(void) {
  void *ptr = malloc(4);

  printf("ptr is at: %p\n", ptr);

  free(ptr);
  return 0;
}
