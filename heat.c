#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>

#define SET 1
#define UNSET 0
#define DEFAULT_BUF_SIZE 256
#define BUF_MAXSIZE 2048
#define LOG_FILE "./heat.verbose.log"

// #define HEAT_FAIL_CODE 0
// #define HEAT_FAIL_TIME 1
// #define HEAT_FAIL_TIME_LAST 2
// #define HEAT_FAIL_INTERVAL 3
// #define HEAT_FAIL_PID 4
// #define HEAT_FAIL_CNT 5

// 전역변수
int LOGFD;
pid_t TARGET_PID;
int TARGET_SIGNAL = SIGHUP;
char FAIL_SCRIPT[DEFAULT_BUF_SIZE];
int FAILCODE = UNSET;

int INTERVAL_TIME = 0;
int FAIL_INTERVAL = 0; // 몇 번 연속 틀렸는가
int FAIL_PID;
char FAIL_TIME[DEFAULT_BUF_SIZE];
int FAIL_STATUS_CODE;

char FAIL_SCRIPT[DEFAULT_BUF_SIZE];
char RECOVER_SCRIPT[DEFAULT_BUF_SIZE];
int THRESHOLD = 5;
int RECOVERY_TIME = 0;




extern char *optarg;
extern int optind;
extern int opterr;

enum OptionType {NO=0, BASIC=1, BASIC_SHELL=2, BASIC_SIGNAL=3, BASIC_FAIL=4, RECOVER=5};

void noHandle(int signo) {
    //알람시그널을 받으면 깨어나게
    // printf("handling\n");
}

void closeLogfd(int signo) {
    printf("[Process Exit]\n");
    close(LOGFD);
    exit(0);
}

/*
    기본 or -s만 
*/
void basicChildHandler(int signo) {
    pid_t pid_chld;
    int status;

    struct tm *tm;
    time_t t;
    char buf[DEFAULT_BUF_SIZE];
    time(&t);
    tm = localtime(&t);
    strftime(buf, sizeof(buf), "%x %X", tm);
    printf("%s:  ",buf);


    if(write(LOGFD, buf, strlen(buf))!=strlen(buf)){
        perror("write error");
    }
    while(1) {
        if((pid_chld = waitpid(-1, &status, WNOHANG)) > 0) {
            int fixStatus = status >> 8;
            if(fixStatus == 0) {
                // 정상 출력;
                printf("OK\n");
            } else {
                printf("Failed: Exit Code %d, details in heat.log\n", fixStatus);
            }
        } else {
            break;
        }
    }
}

/*
    -s --pid --signal 옵션
*/
void signalOptionChildHandler(int signo) {
    pid_t pid_chld;
    int status;

    struct tm *tm;
    time_t t;
    char buf[DEFAULT_BUF_SIZE];
    time(&t);
    tm = localtime(&t);
    strftime(buf, sizeof(buf), "%x %X", tm);
    printf("%s:  ",buf);


    if(write(LOGFD, buf, strlen(buf))!=strlen(buf)){
        perror("write error");
    }
    while(1) {
        if((pid_chld = waitpid(-1, &status, WNOHANG)) > 0) {
            int fixStatus = status >> 8;
            if(fixStatus == 0) {
                // 정상 출력;
                printf("OK\n");
            } else {
                printf("Failed: Exit Code %d, details in heat.log\n", fixStatus);
                int result;
                if((result = kill(TARGET_PID, TARGET_SIGNAL))==-1) {
                    perror("NO PID, CANT KILL\n");
                    exit(1);
                };
            }
        } else {
            break;
        }
    }
}

void failureHandler(int signo) {
    pid_t pid_chld;
    int status;

    struct tm *tm;
    time_t t;
    char buf[DEFAULT_BUF_SIZE];
    time(&t);
    tm = localtime(&t);
    strftime(buf, sizeof(buf), "%x %X", tm);
    strcpy(FAIL_TIME,buf);
    printf("%s:  ",buf);


    if(write(LOGFD, buf, strlen(buf))!=strlen(buf)){
        perror("write error");
    }
    
    while(1) {
        if((pid_chld = waitpid(-1, &status, WNOHANG)) > 0) {
            int fixStatus = status >> 8;
            if(fixStatus == 0) {
                // 정상 출력;
                printf("OK\n");
                FAILCODE = UNSET;
            } else {
                printf("Failed: Exit Code %d, details in heat.log\n", fixStatus);
                FAILCODE = SET;
                FAIL_STATUS_CODE = fixStatus;
            }
        } else {
            break;
        }
    }
}

void failExtraHandler(int signo) {
    pid_t pid_chld;
    int status;

    while(1) {
        if((pid_chld = waitpid(-1, &status, WNOHANG)) > 0) {

        } else {
            break;
        }
    }
}

void failureCode () {
    pid_t pid;
    struct sigaction chldAct;
    sigfillset(&chldAct.sa_mask);
    chldAct.sa_flags = SA_NOCLDSTOP;
    chldAct.sa_handler = failExtraHandler;//함수적용 다른걸로
    if(sigaction(SIGCHLD, &chldAct, NULL) == -1 ) {
        perror("sig action error");
        exit(1);
    }

    char *argv[3];
    char *envp[5];

    argv[0] = "sh";
    argv[1] = FAIL_SCRIPT;
    argv[2] = NULL;

    char e1[DEFAULT_BUF_SIZE] = "HEAT_FAIL_CODE=";
    char er1[DEFAULT_BUF_SIZE];
    sprintf(er1, "%d", FAIL_STATUS_CODE);
    strcat(e1,er1);

    char e2[DEFAULT_BUF_SIZE] = "HEAT_FAIL_TIME=";
    strcat(e2,FAIL_TIME);

    char e3[DEFAULT_BUF_SIZE] = "HEAT_FAIL_INTERVAL=";
    char er3[DEFAULT_BUF_SIZE];
    sprintf(er3, "%d", INTERVAL_TIME);
    strcat(e3,er3);

    char e4[DEFAULT_BUF_SIZE] = "HEAT_FAIL_PID=";
    char er4[DEFAULT_BUF_SIZE];
    sprintf(er4, "%d", FAIL_PID);
    strcat(e4,er4);

    envp[0] = e1;
    envp[1] = e2;
    envp[2] = e3;
    envp[3] = e4;
    envp[4] = NULL;

    switch(pid=fork()) {
        case -1:
            perror("failure code fork error");
            exit(1);
        case 0:
            if (execve("/usr/bin/sh", argv, envp) == -1) {
                perror("execve error");
                exit(1);
            }
        default:
            pause();
    }

}

void recoverFunction(int signo) {
    pid_t pid_chld;
    int status;

    struct tm *tm;
    time_t t;
    char buf[DEFAULT_BUF_SIZE];
    time(&t);
    tm = localtime(&t);
    strftime(buf, sizeof(buf), "%x %X", tm);
    strcpy(FAIL_TIME,buf);
    printf("%s:  ",buf);


    if(write(LOGFD, buf, strlen(buf))!=strlen(buf)){
        perror("write error");
    }
    
    while(1) {
        if((pid_chld = waitpid(-1, &status, WNOHANG)) > 0) {
            int fixStatus = status >> 8;
            if(fixStatus == 0) {
                // 정상 출력;
                printf("OK\n");
                FAIL_INTERVAL = 0;
                FAILCODE = UNSET;
            } else {
                printf("Failed: Exit Code %d, details in heat.log\n", fixStatus);
                FAILCODE = SET;
                FAIL_INTERVAL++;
                FAIL_STATUS_CODE = fixStatus;
            }
        } else {
            break;
        }
    }
}

void recoverExtraHandler(int signo) {
    pid_t pid_chld;
    int status;
    int timeCount = 0;
    while(1) {
        if((pid_chld = waitpid(-1, &status, WNOHANG)) > 0) {
            int fixStatus = status >> 8;
            if(fixStatus == 0) {
                FAIL_INTERVAL = 0;
                FAILCODE = UNSET;
                break;
            } else {
                //실패는 정의하지 않았음
            }
        } else {
            break;
        }
    }
}

void recoverCode() {
    pid_t pid;
    struct sigaction chldAct;
    sigfillset(&chldAct.sa_mask);
    chldAct.sa_flags = SA_NOCLDSTOP;
    chldAct.sa_handler = recoverExtraHandler;//함수적용 다른걸로
    if(sigaction(SIGCHLD, &chldAct, NULL) == -1 ) {
        perror("sig action error");
        exit(1);
    }

    char *argv[3];
    
    char *envp[1];

    argv[0] = "./recover-shell";
    argv[1] = RECOVER_SCRIPT;
    argv[2] = (char*)NULL;

    envp[0] = (char*)NULL;

    if (FAIL_INTERVAL == THRESHOLD) {
        while(FAILCODE == SET) {
            switch(pid =fork()) {
                case -1:
                    perror("recover fail\n");
                    exit(1);
                case 0:
                    if (execve("./recover-shell", argv, envp) == -1) {
                        perror("execve recover error");
                        exit(1);
                    }
                default:
                    pause();
                    break;
            }
        }   
    }
}

void basicCommandFork(char **arg, int optionType) {
    INTERVAL_TIME++;
    pid_t pid;
    
    struct sigaction chldAct;
    sigfillset(&chldAct.sa_mask);

    chldAct.sa_flags = SA_NOCLDSTOP;
    if(optionType == BASIC || optionType == BASIC_SHELL) {
        chldAct.sa_handler = basicChildHandler;
    } else if(optionType == BASIC_SIGNAL) {
        chldAct.sa_handler = signalOptionChildHandler;
    } else if(optionType == BASIC_FAIL) {
        chldAct.sa_handler = failureHandler;
    } else if(optionType == RECOVER) {
        chldAct.sa_handler = recoverFunction;
    }
    else {
        chldAct.sa_handler = basicChildHandler;
    }
    
    if(sigaction(SIGCHLD, &chldAct, NULL) == -1 ) {
        perror("sig action error");
        exit(1);
    }

    int fd[2];
    if (pipe(fd) == -1) {
        perror("pipe error");
        exit(1);
    }
    
    switch(pid = fork()) {
        case -1:
            perror("Basic Command Fork Error");
            exit(1);
        case 0:
            
            close(fd[0]);
            dup2(fd[1],1);
            if(optionType == BASIC) {
                if(execv("/usr/bin/curl", arg) == -1) {
                    perror("curl execv error");
                    exit(2);
                }
            } else {
                if(execv("/usr/bin/sh", arg) == -1) {
                    perror("sh execv error");
                    exit(2);
                }
            }
            
            break;
        default:
            FAIL_PID = pid;
            pause();
            dup2(fd[0],0);
            close(fd[1]);

            char buf[BUF_MAXSIZE];
            int n = read(fd[0], buf, BUF_MAXSIZE);
            if(n>0) {
                if(write(LOGFD, buf, n)!=n){
                    perror("write error");
                }
            }           
            break;
    }

    if(FAILCODE == SET) {
        switch(optionType) {
            case BASIC_FAIL:
                failureCode();
                break;
            case RECOVER:
                recoverCode();
            default :
                break;
        }
    }
}

int main(int argc, char **argv) {
    if((LOGFD = open(LOG_FILE, O_CREAT|O_WRONLY|O_APPEND,0644))==-1) {
        perror("log error");
        exit(1);
    }
    
    int is_i = UNSET;
    int is_s = UNSET;
    int is_pid = UNSET;
    int is_signal = UNSET;
    int is_fail = UNSET;
    int is_recover = UNSET;
    int is_thres = UNSET;
    int is_recover_time = UNSET;
   
    int interval = 0;
    char shellScript[DEFAULT_BUF_SIZE];
    char shFileName[DEFAULT_BUF_SIZE];
    int param_opt;
    enum OptionType programOption = BASIC;

    struct option longOptions[] = {
        {"pid",1,0,0},
        {"signal",1,0,0},
        {"fail",1,0,0},
        {"recovery",1,0,0},
        {"threshold",1,0,0},
        {"recovery-timeout",1,0,0},
        {0,0,0,0}
    };
    /*
    가능한 시그널: SIGUSR1 , SIGHUP
    */

    int optionIndex;
    
    while((-1 != (param_opt = getopt_long(argc, argv, "i:s:",longOptions,&optionIndex)))) {
        switch(param_opt) {
            case 'i':
                is_i = SET;
                interval = atoi(optarg); // optarg 사용
                break;

            case 's':
                is_s = SET;
                programOption = BASIC_SHELL;
                if(access(optarg, F_OK|X_OK) != -1) {
                    strcpy(shFileName, optarg);
                } else {
                    perror("file doesn't exist or not excutable");
                    exit(1);
                }
                break;

            case 0:
                if(strcmp(longOptions[optionIndex].name, "pid") == 0) 
                {
                    is_pid = SET;
                    TARGET_PID = atoi(optarg);
                }
                else if(strcmp(longOptions[optionIndex].name, "signal") == 0)
                {
                    is_signal = SET;
                    printf("signal : %s\n", optarg);
                    if(strcmp(optarg, "SIGUSR1")==0) {
                        TARGET_SIGNAL = SIGUSR1;
                    } else if(strcmp(optarg, "SIGHUP")==0) {
                        TARGET_SIGNAL = SIGHUP;
                    } else {
                        perror("signal 처리할 수 없음 ");
                        exit(1);
                    }
                }
                else if(strcmp(longOptions[optionIndex].name, "fail") == 0)
                {
                    is_fail = SET;
                    if(access(optarg, F_OK | X_OK) != -1) {
                        strcpy(FAIL_SCRIPT, optarg);
                    } else {
                        perror("file doesn't exist or not EXCUTABLE");
                        exit(1);
                    }
                }
                else if(strcmp(longOptions[optionIndex].name, "recovery") == 0)
                {
                    is_recover = SET;
                    if(access(optarg, F_OK | X_OK) != -1) {
                        strcpy(RECOVER_SCRIPT, optarg);
                    } else {
                        perror("file doesn't exist or not EXCUTABLE");
                        exit(1);
                    }
                }
                else if(strcmp(longOptions[optionIndex].name, "threshold") == 0)
                {
                    is_thres = SET;
                    THRESHOLD = atoi(optarg);
                }
                else if(strcmp(longOptions[optionIndex].name, "recovery-timeout") == 0)
                {
                    is_recover_time = SET;
                    RECOVERY_TIME = atoi(optarg);
                }
            
            case '?':  
                // printf("알 수 없는 옵션: %c\n", optopt); // optopt 사용
                break;
        }
    }

   

    // 추가적인 명령행 인자 처리;
    char **commandArg;
    if(programOption==BASIC) {
        strcpy(shellScript, argv[argc-1]);
        // 명령어 개수 세기
        int argSize = 0;
        char countTmpStr[DEFAULT_BUF_SIZE];
        strcpy(countTmpStr, shellScript);
        char *tmp = strtok(countTmpStr, " ");
        while(tmp!=NULL) {
            argSize++;
            tmp = strtok(NULL," ");
        }
        //명령어 배열 만들기
        
        commandArg = (char **)malloc(sizeof(char*)*(++argSize));
        int idx = 0;

        char *strToken = strtok(shellScript, " ");
        while(strToken!=NULL) {
            commandArg[idx++] = strToken;
            strToken = strtok(NULL, " ");
        }
        commandArg[idx] = NULL;
    } else if(programOption == BASIC_SHELL) {
        commandArg = (char **)malloc(sizeof(char*)*(3));
        commandArg[0] = "sh";
        commandArg[1] = shFileName;
        commandArg[2] = (char*) NULL;
    }

    // fail과 recover과 같이 쓰이면 안됨
    if(is_fail == SET) {
        programOption = BASIC_FAIL;
    } else if (is_pid==SET) {
        programOption = BASIC_SIGNAL;
    } else if (is_recover == SET && is_thres == SET) {
        // printf("%s\n", RECOVER_SCRIPT);
        // printf("%d\n", THRESHOLD);
        // printf("%d\n", RECOVERY_TIME);
        programOption = RECOVER;
    }
    // else {
    //     perror("pid and signal set error");
    //     exit(1);
    // }
    
    


    // 알람 시그널
    struct sigaction alarmAct;
    
    //블록할 시그널 들
    sigemptyset(&alarmAct.sa_mask);
    sigaddset(&alarmAct.sa_mask, SIGQUIT);
    sigaddset(&alarmAct.sa_mask, SIGINT);

    // sigaction SIGALRM 핸들링
    alarmAct.sa_flags = 0;
    alarmAct.sa_handler = noHandle;

    if(sigaction(SIGALRM, &alarmAct, NULL) < 0) {
        perror("sigaction error");
        exit(1);
    }

    // 시그널 처리 하지만 테스트 단계임
    struct sigaction quitAct;

    sigfillset(&quitAct.sa_mask);

    quitAct.sa_flags = 0;
    quitAct.sa_handler = closeLogfd;

    if(sigaction(SIGINT, &quitAct, NULL) < 0) {
        perror("sigaction error");
        exit(1);
    }
    if(sigaction(SIGQUIT, &quitAct, NULL) < 0) {
        perror("sigaction error");
        exit(1);
    }
    


    //타이머 생성
    struct itimerval it;
    it.it_value.tv_sec = interval;
    it.it_value.tv_usec = 0;
    it.it_interval.tv_sec = interval;
    it.it_interval.tv_usec = 0;

    if(setitimer(ITIMER_REAL, &it, NULL) == -1) {
        perror("setitimer error");
        exit(1);
    }
    while(1) {
        basicCommandFork(commandArg, programOption);
        pause();
    }
    return 0;
}