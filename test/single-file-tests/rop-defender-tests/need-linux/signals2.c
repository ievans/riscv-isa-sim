// modified from http://www.alexonlinux.com/signal-handling-in-linux

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

struct sigaction act;
char recv_sig[32];
#define RECVSIG "Received signal %d\n"
char process_sig[64];
#define PROCSIG "Signal originates from process %lu\n"

void sighandler(int signum, siginfo_t *info, void *ptr)
{
    snprintf(recv_sig, 32, RECVSIG, signum);
    write(1, recv_sig, sizeof(RECVSIG));
    snprintf(process_sig, 64, PROCSIG, (unsigned long)info->si_pid);
    write(1, process_sig, sizeof(PROCSIG));
}

int main()
{
    printf("I am %lu\n", (unsigned long)getpid());

    memset(&act, 0, sizeof(act));

    act.sa_sigaction = sighandler;
    act.sa_flags = SA_SIGINFO;

    sigaction(SIGINT, &act, NULL);

    raise(SIGINT);

    // Wait for signal handler to run.
    sleep(5);

    printf("\n");
    return 0;
}
