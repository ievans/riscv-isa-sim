#include <stdio.h>
#include "../../tag-extensions.h"

int main(void) {
  tag_enforcement_on();

  void* mem = malloc_tagged(4, false);
  printf("mem is located at: %p\n", mem);
  free_tagged(mem);

  return 0;
}


