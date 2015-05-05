#include <stdio.h>
#include <stddef.h>

void usage(void)
{
  printf("usage: test the idiom of getting a pointer to a struct\n");
  printf("from a pointer to a member of that struct\n");
  printf("------------------------------------------------------\n");
}

void otherVoidFunc(void)
{
  printf("I'm a different void function.\n");
}

struct funStruct {
  int j;
  int k;
  void (*fptr)(void);
};

void print_funStruct(struct funStruct *fs)
{
  printf("j is: %d\nk is: %d\nfptr is: %p\n",
    fs->j, fs->k, (void *) fs->fptr);
}

int main(void)
{
  usage();

  // make the struct and print it
  struct funStruct fs;
  fs.j = 3;
  fs.k = 7;
  fs.fptr = &usage;
  print_funStruct(&fs);

  // now, do the offsetof code
  void* a_fptr = &fs.fptr;
  printf("a_fptr is: %p\n", a_fptr);
  a_fptr = a_fptr - offsetof(struct funStruct, fptr);
  
  // change data using this new pointer
  struct funStruct* fs2 = (struct funStruct *) a_fptr;
  fs2->k = 11;
  fs2->fptr = &otherVoidFunc;

  // print the struct again to see that if it has changed
  print_funStruct(&fs);

  return 0;
}
