#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../riscv/libspike.h"

typedef void (*fp_t)();
#define N 128
// char 'h' has value 0x68, divisible by 8
char buf[N] = "hhhhhhhhhhhhhhh|hhhhhhhhhhhhhhh|hhhhhhhhhhhhhhh|hhhhhhhhhhhhhhh|hhhhhhhhhhhhhhh|hhhhhhhhhhhhhhh|hhhhhhhhhhhhhhh|hhhhhhhhhhhhhhh";

void test() {
  printf("Executing function test()\n");
}

typedef struct _big_fptr_t {
  char a[N-8];
  int* ptr;
} big_fptr_t;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("usage: %s type\n", argv[0]);
    return;
  }
  int type = atoi(argv[1]);
  printf("Executing with type %d\n", type);

  big_fptr_t x;
  int n = 4;
  x.a[0] = 0;
  x.ptr = &n;
  *x.ptr = 8;
  printf("n = %d\n", n);

  if (type == 0)
    memcpy(x.a, buf, sizeof(buf));
  else if (type == 1)
    gets(x.a);
  else if (type == 2)
    strcpy(x.a, buf);
  else if (type == 3)
  {
    size_t i;
    for(i = 0; i < sizeof(buf); i++) {
      x.a[i] = buf[i];
    }
  }
  else if (type == 4)
    strncpy(x.a, buf, sizeof(buf));
  else if (type == 5)
    strcat(x.a, buf);
  else if (type == 6)
    strncat(x.a, buf, sizeof(buf));
  else if (type == 7)
    memmove(x.a, buf, sizeof(buf));
  else if (type == 8)
    read(1, x.a, N);

  printf("New value of str: %s\n", x.a);
  libspike_monitor();
  *x.ptr = 3;
  printf("n = %d\n", n);
  return 0;
}
