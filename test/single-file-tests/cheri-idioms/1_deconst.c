#include <stdio.h>
#include <string.h>

#define STARTSTR "I am an immutable string."

void usage(void)
{
  printf("usage: test behavior on const violation of char *\n");
  printf("-------------------------------------------------\n"); 
}

int main(void)
{
  usage();

  const char str[] = STARTSTR;
  printf("str is: %s\n", str);

  // Let's get a non-const pointer.
  char* mutable_str = strchr(str, 'a');

  // Modify the string now and print it.
  mutable_str += 3;
  *(mutable_str)++ = 'n';
  *(mutable_str) = 'o';
  printf("modified str is: %s\n", str);

  return 0;
}
