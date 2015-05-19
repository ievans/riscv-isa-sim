#include <stdio.h>

int main(void) {
  char a[10];
  double* d;

  d=(double *) &a[1];
  *d=5.0;

  printf("%f\n", *d);

  return 0;
} 
