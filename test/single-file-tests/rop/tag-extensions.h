#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define NOT_AVAILABLE "warning: tag enforcement not available for this architecture"

void tag_enforcement_on() {
#ifdef __riscv
  __asm__ ("tagenforce ra,1");
#else
  printf(NOT_AVAILABLE);   
#endif
}

// For now...
void tag_enforcement_off() {
#ifdef __riscv
  __asm__ ("tagenforce ra,0");
#else
  printf(NOT_AVAILABLE);
#endif
}

// I don't understand GCC inline assembly...
void set_tag_t3() {
#ifdef __riscv
  __asm__ ("settag t3,537");
#else
  printf(NOT_AVAILABLE);
#endif
}

void clear_tag_t3() {
#ifdef __riscv
  __asm__ ("settag t3,0");
#else
  printf(NOT_AVAILABLE);
#endif
}

void* malloc_tagged(size_t size, bool is_fptr) {
  void* mem = malloc(size);

  // Check for null ptrs.
  if (!mem)
    return NULL;

#ifdef __riscv
  // Return register is a0, mem should be there.
  if (is_fptr) {
    __asm__ __volatile__ ("settag a5, 2");
  } else {
    __asm__ __volatile__ ("settag a5, 1");
  }
#else
  printf(NOT_AVAILABLE);
#endif

  return mem;
}

void free_tagged(void* ptr) {
  // Check for null ptrs.
  if (!ptr)
    return;

#ifdef __riscv
  // No matter the type of pointer, set to uninitialized.
  __asm__ __volatile__ ("settag a5, 0");
#else
  printf(NOT_AVAILABLE);
#endif

  free(ptr);
}


