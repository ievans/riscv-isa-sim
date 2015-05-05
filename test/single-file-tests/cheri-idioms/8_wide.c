#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/mman.h>

#define MAPFIXED_ADDR (void *) 0x37bfe000

void usage(void)
{
  printf("usage: this test tries to store a 64-bit pointer with a\n"
         " 32-bit value into a 32-bit numerical type, and get it\n"
         " back and use it\n");
  printf("-------------------------------------------------------\n");
}

int main(void)
{
  usage();

  // get the pointer
  uint64_t *memoryaddr = mmap(MAPFIXED_ADDR, sizeof(uint64_t *), PROT_WRITE | PROT_EXEC, 
    MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  *memoryaddr = 16;
  
  uint32_t abc;

  printf("original pointer: %p\n", memoryaddr);
  printf("original contents: %" PRIu64 "\n", *memoryaddr);

  abc = (uint32_t) (uintptr_t) memoryaddr;
  printf("after store in 32-bit type: %" PRIx32 "\n", abc);

  abc += 1;
  printf("after store + inc: %" PRIx32 "\n", abc);

  // try to use that pointer after putting it back into a 64-bit ptr
  memoryaddr = (uint64_t *) (uintptr_t) (abc - 1);

  printf("recovered pointer: %p\n", memoryaddr);
  printf("recovered contents: %" PRIu64 "\n", *memoryaddr);

  munmap(memoryaddr, sizeof(uint64_t *));
  return 0;
}

