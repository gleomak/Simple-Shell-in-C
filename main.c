#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <glob.h>
#include "handlersAndHelpers.h"

pid_t pidArray[MAX_EXECUTABLES];
int fgChildren;
int childRunning = 0;

void handleSIGCHLD(int sig) {
    int status;
    // loop through child processes that have exited
    if(!childRunning) {
        while ((waitpid(-1, &status, WNOHANG)) > 0) {
        }
    }
}

void handleSIGINT(int sig){
    if(childRunning) {
        for(int i = 0 ; i <=fgChildren ; i++) {
            kill(pidArray[i], SIGINT); // Send SIGINT signal to all child processes
        }
    }
}

void handleSIGSTOP(int sig){
    if(childRunning) {
        for(int i = 0 ; i <= fgChildren ; i++){
            kill(pidArray[i], SIGTSTP);
            setpgid(pidArray[i], 0);
        }
    }
}


void run_Commands(argumentStruct *aStruct) {
    pid_t pid;
    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fork Failed");
    } else if (pid == 0) { /* child process */
        if (aStruct->boolInput) {
            int fd = open(aStruct->input, O_RDONLY, 0666);
            dup2(fd, READ);
            close(fd);
        }
        if (aStruct->boolOutput) {
            int fd2 = open(aStruct->output, O_WRONLY | O_CREAT | O_TRUNC, 0666);
            dup2(fd2, WRITE);
            close(fd2);
        } else if (aStruct->boolOutput2) {
            int fd2 = open(aStruct->output, O_WRONLY | O_CREAT | O_APPEND, 0666);
            dup2(fd2, WRITE);
            close(fd2);
        }
        if(aStruct->boolBackground){
            setpgid(pid , 0);
        }
        execvp(aStruct->argumentList[0], aStruct->argumentList);
        perror("execvp"); // Print an error message if execvp fails
        exit(0);
    } else {
        if (!aStruct->boolBackground) {
            int status;
            childRunning = 1;
            fgChildren = 0;
            pidArray[0] = pid;
            if (waitpid(pid, &status, WUNTRACED) == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
            childRunning = 0;
        }
    }
}

void redirectionHandler(char *tokenized) {
    argumentStruct *aStruct = structBasicFiller(tokenized);
    if(aStruct == NULL)
        return;
    run_Commands(aStruct);
    free(aStruct);
}

void pipeHandler(char *tokenized, int pipes) {
    argumentStruct *argumentStructArray = structPipeFiller(tokenized, pipes);
    pid_t pid;
    int previousPipe = 0;
    int p[2];
    int pipeFather = -1;
    int backgroundpgid = 0;
    int isBackground = 0;
    if(argumentStructArray[pipes].boolBackground) {
        isBackground = 1;
    }
    fgChildren = pipes;
    for (int z = 0; z <= pipes; z++) {
        if (pipe(p) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        if ((pid = fork()) == -1) {
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            if (argumentStructArray[z].boolInput == 1) {
                previousPipe = open(argumentStructArray[0].input, O_RDONLY, 0666);
                dup2(previousPipe, READ);
                close(previousPipe);
            } else if (previousPipe != STDIN_FILENO && z > 0) {
                if (dup2(previousPipe, STDIN_FILENO) < 0)
                    perror("DUP2");
                if (close(previousPipe) < 0)
                    perror("CLOSE");
            }
            close(p[0]);
            if (z != pipes) {
                if (dup2(p[1], WRITE) < 0) {
                    perror("DUP22");
                }

            } else {
                if (argumentStructArray[z].boolOutput) {
                    int output = open(argumentStructArray[z].output, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    if (dup2(output, STDOUT_FILENO) == -1) {
                        perror("dup2");
                    }
                    close(output);
                } else if (argumentStructArray[z].boolOutput2) {
                    int output = open(argumentStructArray[z].output, O_WRONLY | O_CREAT | O_APPEND, 0666);
                    if (dup2(output, STDOUT_FILENO) == -1) {
                        perror("dup2");
                    }
                    close(output);
                }
            }
            if (close(p[1]) < 0) {
                perror("CLOSE22");
            }
            if(argumentStructArray[z].boolBackground){
                setpgid(pid, backgroundpgid);
            }


            execvp(argumentStructArray[z].argumentList[0], argumentStructArray[z].argumentList);
            exit(EXIT_FAILURE);
        } else {
            if(isBackground){
                if(!backgroundpgid){
                    backgroundpgid = pid;
                }
                setpgid(pid, backgroundpgid);
            }else {
                pidArray[z] = pid;
            }
            if (close(p[1]) < 0)
                perror("CLOSE222");
            previousPipe = p[0];
            if (pipeFather != -1) {
                if (close(pipeFather) < 0)
                    perror("CloseFatherPipe");
            }
            pipeFather = p[0];
        }
    }
    if (close(pipeFather) < 0)
        perror("CloseFatherPipe");
    int status;
    if(!isBackground){
        childRunning = 1;
        for (int i = 0; i <= pipes; i++) {
            waitpid(-1, &status, WUNTRACED);
        }
        childRunning = 0;
    }
    free(argumentStructArray);
}

int main() {
    char *executables[MAX_EXECUTABLES];
    char cmdLine[MAX_LINE];
    char cmdHistory[MAX_LINE * 2];
    char *tokenized = (char *) malloc((MAX_LINE * 2) * sizeof(char));
    alias *aliases = (alias *) malloc(MAX_ALIASES * sizeof(alias));
    char historyArray[MAX_HISTORY][MAX_LINE];
    clearAndInitHistory(historyArray);
    clearAliases(aliases);

    static struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handleSIGCHLD;
    sa.sa_flags = SA_NOCLDSTOP | SA_RESTART;
    sigaction(SIGCHLD,&sa,NULL);


    static struct sigaction sa2;
    memset(&sa2, 0, sizeof(sa2));
    sa2.sa_handler = handleSIGINT;
    sa2.sa_flags = SA_NOCLDSTOP | SA_RESTART;
    sigaction(SIGINT,&sa2,NULL);

    static struct sigaction sa3;
    memset(&sa3, 0, sizeof(sa3));
    sa3.sa_handler = handleSIGSTOP;
    sa3.sa_flags = SA_RESTART;
    sigaction(SIGTSTP,&sa3,NULL);

    while (1) {
        /*Read*/
        printf("in-mysh-now:> ");
        fflush(stdout);
        fgets(cmdLine, MAX_LINE, stdin);

        int pipesOrNo = 0;
        char *arg = strtok(cmdLine, ";");
        int execs = 0;
        while (arg) {
            executables[execs++] = arg;
            arg = strtok(NULL, ";");
        }
        int alias = -1;
        if(execs == 0)
            continue;
        for (int i = 0; i < execs; i++) {
            strcpy(tokenized,"");
            tokenized = tokenizer(executables[i] , tokenized, &pipesOrNo);
            if(strcmp(tokenized,"") == 0 ){
                break;
            }
            isAlias(tokenized , aliases, &alias);
            if(alias >= 0) {
                tokenized = tokenizer(aliases[alias].command, tokenized, &pipesOrNo);
            }
            strcpy(cmdHistory, tokenized);
            if(builtInsHandler(tokenized, aliases, historyArray, &pipesOrNo)){
                if(strcmp(tokenized,"history") != 0)
                    addCommandInHistory(cmdHistory, historyArray);
                continue;
            }
            if(strcmp(tokenized,"") != 0){
                addCommandInHistory(cmdHistory, historyArray);
            }
            if (pipesOrNo != 0) {
                pipeHandler(tokenized, pipesOrNo);
            }
            else {
                redirectionHandler(tokenized);
            }
            alias = -1;
            pipesOrNo = 0;
        }
        if (feof(stdin))
            break;
        for (int i = 0; i < execs; i++) {
            executables[i] = NULL;
        }
    }
    return 0;
}
