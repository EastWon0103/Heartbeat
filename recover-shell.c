#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/time.h>

int main(int argc, char *argv[]) {
    pid_t pid;
    int fd[2];
    struct sigaction chldAct;
    sigfillset(&chldAct.sa_mask);
    chldAct.sa_flags = 0;
    chldAct.sa_handler = SIG_IGN;//함수적용 다른걸로
    if(sigaction(SIGCHLD, &chldAct, NULL) == -1 ) {
        perror("sig action error");
        exit(1);
    }

    if(pipe(fd) == -1) {
        perror("pipe error");
        exit(23);
    }
    
    // for(int i=0;i<argc;i++) {
    //     printf("%s:%d\n", argv[i],i);
    // }

    char *newArg[2] = {argv[argc-1],(char*)NULL};
    switch(pid=fork()) {
        case -1:
            perror("recover fork error");
            exit(1);
        case 0:
            dup2(fd[1], 1);
            close(fd[1]);
            close(fd[0]);
            if (execv(argv[argc-1], newArg) == -1) {
                perror("execve error");
                exit(1);
            }
        default:
            close(fd[1]);
            close(fd[0]);
            exit(0);
    }
    return 0;
}