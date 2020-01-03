/*my mini shell
시스템 프로그래밍
2017012197 여채린*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>

#define LINE_SIZE 1024
#define CMD_SIZE 16
#define HISTORY_SIZE 1024

char lineCommands[LINE_SIZE];
char commands[CMD_SIZE][LINE_SIZE];
char pipedCommands[CMD_SIZE][LINE_SIZE];

char history[HISTORY_SIZE][LINE_SIZE];
int curHistorySize = 0;
int cmdSize = 0;

void trim(char * str) {
    char *strPoint = str;
    int strLen = strlen(strPoint);

    while(isspace(strPoint[strLen - 1])) strPoint[--strLen] = 0;
    while(*strPoint && isspace(*strPoint)) ++strPoint, --strLen;

    memmove(str, strPoint, strLen + 1);
}

void init() {
    bzero(lineCommands, LINE_SIZE);
    bzero(commands, CMD_SIZE*LINE_SIZE);
    bzero(pipedCommands, CMD_SIZE*LINE_SIZE);
}

void insertHistory()
{
    strcpy(history[curHistorySize++], lineCommands);
}

void showHistory()
{
    int i;
    for (i = 0; i < curHistorySize; i++) {
        if (history[i] == NULL) {
            break;
        } else {
            printf("%d\t%s\n", i+1, history[i]);
        }
    }
}

void changeDirectory(char *cmd) {
    char commandArgs[16][LINE_SIZE];
    bzero(commandArgs, 16*LINE_SIZE);

    char delim[] = " ";
    int i;
    char *ptr;
    for (i = 0, ptr = strtok(cmd, delim); ptr != NULL; ptr = strtok(NULL, delim), i++) {
        strcpy(commandArgs[i], ptr);
        trim(commandArgs[i]);
        //printf("[LOG] argv[%d]=%s\n", i, commandArgs[i]);
    }

    //printf("[LOG] change directory=%s\n", commandArgs[1]);
    if (commandArgs[1] == NULL) {
        printf("argument is NULL\n");
        return;
    } else {
        if (chdir(commandArgs[1]) != 0) {
            perror("smsh");
        }
    }
}

void execPipedCommand(char *cmd) 
{
    //printf("[LOG] execute piped command=%s\n", cmd);
    char pipeDelim[] = "|";
    int i;
    char *ptr;
    int pipeCmdCnt;
    for (i = 0, ptr = strtok(cmd, pipeDelim); ptr != NULL; ptr = strtok(NULL, pipeDelim), i++) {
        strcpy(pipedCommands[i], ptr);
        trim(pipedCommands[i]);
    }
    pipeCmdCnt = i;
    for (i = 0; i < pipeCmdCnt; i++) {
        //printf("[LOG] piped command=%s\n", pipedCommands[i]);
    }

    int fd[2];
    pid_t pid;
    int fdd = 0;
    for (i = 0; i < pipeCmdCnt; i++) {  
       //printf("[LOG] pipid loop %d\n", i);
        if (pipe(fd) < 0) {
            printf("[ERROR] pipe fail!\n");
            return;
        }
        pid = fork();
        if (pid < 0) {
            printf("[ERROR] fold fail!\n");
        }

        //printf("[LOG] piped created pid=%d\n", pid);
        if (pid == 0) {
            //printf("[LOG] piped child pid=%d, cmd=%s\n", pid, pipedCommands[i]);
            dup2(fdd, 0);
            if (i+1 > pipeCmdCnt) {
                dup2(fd[1], 1);
            }
            close(fd[0]);

            char commandArgs[16][LINE_SIZE];
            bzero(commandArgs, 16*LINE_SIZE);

            char delim[] = " ";
            int argCnt;
            for (i = 0, ptr = strtok(pipedCommands[i], delim); ptr != NULL; ptr = strtok(NULL, delim), i++) {
                strcpy(commandArgs[i], ptr);
                trim(commandArgs[i]);
                //printf("[LOG] piped argv[%d]=%s\n", i, commandArgs[i]);
            }
            argCnt = i;

            char **argv = NULL;
            argv = (char **)malloc(sizeof(char *)*(argCnt + 1));
            for (i = 0; i < argCnt; i++) {
                *(argv + i) = (char *)malloc(sizeof(char)*strlen(commandArgs[i]+1));
                strcpy(*(argv + i), commandArgs[i]);
            }
            argv[argCnt] = NULL;
            //printf("[LOG] piped argv=%s\n", commandArgs[0]);
            execvp(commandArgs[0], argv);
            exit(1);
        } else {
            wait(NULL);
            close(fd[1]);
            fdd = fd[0];
        }
    }
}

void makeCommands()
{
    char delim[] = ";";
    int i;
    char *ptr;
    for (i = 0, ptr = strtok(lineCommands, delim); ptr != NULL; ptr = strtok(NULL, delim), i++) {
        strcpy(commands[i], ptr);
        trim(commands[i]);
    }
    cmdSize = i;
}

void execCommand(char *cmd) {
    char commandArgs[16][LINE_SIZE];
    bzero(commandArgs, 16*LINE_SIZE);

    int isBg = 0;
    int cmdLen = strlen(cmd);
    if (cmd[cmdLen-1] == '&') {
        //printf("[LOG] commad background!\n");
        isBg = 1;
        cmd[cmdLen-1] = '\0';
    }

    char delim[] = " ";
    int i;
    int argCnt;
    char *ptr;
    for (i = 0, ptr = strtok(cmd, delim); ptr != NULL; ptr = strtok(NULL, delim), i++) {
        strcpy(commandArgs[i], ptr);
        trim(commandArgs[i]);
        //printf("[LOG] argv[%d]=%s\n", i, commandArgs[i]);
    }
    argCnt = i;

    for (i = 0; i < argCnt; i ++) {
        if (!strcmp(commandArgs[i], "|")) {
            execPipedCommand(lineCommands);
            return;
        }
    }

    char **argv = NULL;
    argv = (char **)malloc(sizeof(char *)*(argCnt + 1));
    for (i = 0; i < argCnt; i++) {
        *(argv + i) = (char *)malloc(sizeof(char)*strlen(commandArgs[i]+1));
        strcpy(*(argv + i), commandArgs[i]);
    }
    argv[argCnt] = NULL;

    pid_t pid;
    int status;
    pid = fork();
    if (pid == 0) {
        if (!strcmp(commandArgs[0], "history")) {
            showHistory();
        } else {
            execvp(commandArgs[0], argv);
        }

        for (i = 0; i < argCnt; i++) {
            free(*(argv + i));
        }
        free(argv);

        exit(0);
    } else {
        if (isBg) {
            printf("background pid=%d\n", pid);
            return;
        } else {
            wait(NULL);
        }
    }
}

  

int main(int argc, char const *argv[])
{
    printf("︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿\n");
    printf("\n");
    printf("  (∩ •ω•)⊃━☆ﾟ.*･｡ﾟ Welcome to My Shell! ☆ﾟ.*･｡ﾟ\n");
    printf("\n");
    printf("︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵‿︵\n");
    printf("\n");
    init();
    while(read(STDIN_FILENO, lineCommands, LINE_SIZE)) {
        int lineLen = strlen(lineCommands);
        if (lineLen == 1 && lineCommands[0] == '\n') {
            init();
            continue;
        }
        lineCommands[lineLen] = '\0';
        trim(lineCommands);
        //printf("[LOG] input line=%s\n", lineCommands);

        // insert history
        insertHistory();

        // make commands
        makeCommands();

        int i;
        for (i = 0; i < cmdSize; i++) {
            //printf("[LOG] %d -> command=%s\n", i+1, commands[i]);
            if (!strncmp(commands[i], "cd ", 3)) {
                changeDirectory(commands[i]);
            } else {
                execCommand(commands[i]);
            }
        }

        init();
    }
    
    return 0;
}