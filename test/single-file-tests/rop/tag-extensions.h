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
  // Set the tag on the register that mem is located.
  // Additionally, store this register so that when it is
  // reloaded (which is likely to happen immediately)
  // the tag is correct. -24(s0) is what is generated
  // on the store, so we match it unintelligently here.
  if (is_fptr) {
    __asm__ __volatile__ (
      "settag %0, 2\n\t"
      "sd %0, -24(s0)\n\t"
      "fence" // req'd only with mult. threads on this ptr
      :: "r"(mem)
       : "memory");
  } else {
    __asm__ __volatile__ (
      "settag %0, 1\n\t"
      "sd %0, -24(s0)\n\t"
      "fence" // req'd only with mult. threads on this ptr
      :: "r"(mem)
       : "memory");
  }
#else
  printf(NOT_AVAILABLE);
#endif

  return mem;
}

// This seems like it needs to be inlined, because
// the register holding the pointer passed in is
// not modified otherwise.
static __inline void __attribute__((always_inline))
free_tagged(void* ptr) {
  // Check for null ptrs.
  if (!ptr)
    return;

#ifdef __riscv
  // No matter the type of pointer, set to uninitialized.
  __asm__ __volatile__ (
    "settag %0, 0\n\t"
    "sd %0, -24(s0)\n\t"
    "fence"
    :: "r"(ptr)
     : "memory");
#else
  printf(NOT_AVAILABLE);
#endif

  free(ptr);
}

