#include <stdio.h>
#include <stdlib.h>

int a[10];
void swap ( int x, int y ) {
  int temp;
  temp = a[x];
  a[x] = a[y];
  a[y] = temp;
}

int main(void) {
  int i;
  srand(17);

  for (i = 0; i < 10; i++) {
    int temp;
    temp = rand();
    a[i] = temp;
  }
  for (i = 0; i < 10; i++) {
    printf("%d\n", a[i]);
  }
  printf ("\n");

  for (i = 0; i < 10; i++) {
    int j;
    j = i - 1;
    while ( j >= 0 && ( a[j] > a[j+1] ) ) {
      swap ( j+1, j );
      j -= 1;
    }
  }
  for (i = 0; i < 10; i++) {
    printf( "%d\n", a[i] );
  }

  return 0;
}
