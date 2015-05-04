#include <sys/types.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

int i, j;
long T0;
jmp_buf Env;

// WARNING: printf and longjmp are not functions
// that are safe to call in a signal handler.
// May the lord have mercy on our souls.
void alarm_handler(int dummy)
{
  long t1;

  t1 = time(0) - T0;
  printf("%lu second%s has passed: j = %d.  i = %d\n", t1, 
	 (t1 == 1) ? "" : "s", j, i);
  if (t1 >= 8) {
    printf("Giving up\n");
    longjmp(Env, 1);
  }
  alarm(1);
  signal(SIGALRM, alarm_handler);
}

int main(void)
{
  signal(SIGALRM, alarm_handler);

  // We should come back here after we "give up".
  if (setjmp(Env) != 0) {
    printf("Gave up:  j = %d, i = %d\n", j, i);
    exit(1);
  }

  T0 = time(0);
  alarm(1);

  // Code to prevent the main thread from exiting...
  for (j = 0; j < 10000; j++) {
    for (i = 0; i < 1000; i++) {
      sleep(1);
    }
  }

  return 0; // Probably won't execute.
}
