#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/time.h>

void printSignal(int signo) {
    psignal(signo, "Received Signal");
}

int main() {
    pid_t pid = getpid();
    printf("PROCESS PID: %d\n",pid);
    struct sigaction sigUsr1;
    
    sigemptyset(&sigUsr1.sa_mask);
    sigaddset(&sigUsr1.sa_mask, SIGQUIT);
    sigaddset(&sigUsr1.sa_mask, SIGINT);

    // sigaction SIGALRM 핸들링
    sigUsr1.sa_flags = 0;
    sigUsr1.sa_handler = printSignal;

    if(sigaction(SIGUSR1, &sigUsr1, NULL) < 0) {
        perror("sigaction error");
        exit(1);
    }

    struct sigaction sighup;
    
    sigemptyset(&sighup.sa_mask);
    sigaddset(&sighup.sa_mask, SIGQUIT);
    sigaddset(&sighup.sa_mask, SIGINT);

    // sigaction SIGALRM 핸들링
    sighup.sa_flags = 0;
    sighup.sa_handler = printSignal;

    if(sigaction(SIGHUP, &sighup, NULL) < 0) {
        perror("sigaction error");
        exit(1);
    }
    
    while(1) {
        pause();
    }
    return 0;
}