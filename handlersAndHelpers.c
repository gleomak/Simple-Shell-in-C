#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <glob.h>
#include "handlersAndHelpers.h"

void clearStruct(argumentStruct *argumentStruct) {
    argumentStruct->input = NULL;
    argumentStruct->output = NULL;
    argumentStruct->boolInput = 0;
    argumentStruct->boolOutput = 0;
    argumentStruct->boolOutput2 = 0;
    argumentStruct->boolBackground = 0;
    for (int i = 0; i < MAX_ARGS; i++) {
        argumentStruct->argumentList[i] = NULL;
    }
}

int has_wildcard_char(const char *str) {
    while (*str != '\0') {
        if (*str == '*' || *str == '?' || *str == '[') {
            return 1;
        }
        str++;
    }
    return 0;
}

void globHandler(char *argument, argumentStruct *aStruct, int *i) {
    // Expand wildcards in arguments
    glob_t glob_result;
    memset(&glob_result, 0, sizeof(glob_result));
    char *results = malloc(1);
    results[0] = '\0';
    int ret = glob(argument, GLOB_NOCHECK | GLOB_TILDE, NULL, &glob_result);
    if (ret == 0) {
        // Replace original argument with expanded filenames

        for (int j = 0; j < glob_result.gl_pathc; j++) {
            aStruct->argumentList[(*i)++] = glob_result.gl_pathv[j];
        }
    } else if (ret == GLOB_NOMATCH) {
        // No matching files found, do nothing
    } else {
        // Error occurred, print error message and skip this argument
        fprintf(stderr, "glob error: %s\n", strerror(errno));
    }
}

void cdHandler(char *tokenized) {
    argumentStruct *aStruct = structBasicFiller(tokenized);
    int ret;
    char* dir = getenv("PWD");
    if(aStruct->argumentList[1] != NULL){
        dir = aStruct->argumentList[1];
    }
    ret = chdir(dir);
    if (ret != 0) {
        perror("cd");
    }
    free(aStruct);
}

void createAliasHandler(char *tokenized, alias *aliases) {
    char *arg = strtok(tokenized, " ");
    for (int i = 0; i < MAX_ALIASES; i++) {
        if (strcmp(aliases[i].name, "") == 0) {
            arg = strtok(NULL, " ");
            strcpy(aliases[i].name, arg);
            arg = strtok(NULL, " "); // the " character
            arg = strtok(NULL, " "); // the command
            while (strcmp(arg, "\"") != 0) {
                strcat(aliases[i].command, arg);
                arg = strtok(NULL, " ");
                strcat(aliases[i].command, " ");
            }
            break;
        }
    }
}

void destroyAliasHandler(char *tokenized, alias *aliases) {
    char *arg = strtok(tokenized, " ");
    arg = strtok(NULL, " ");
    for (int i = 0; i < MAX_ALIASES; i++) {
        if (strcmp(aliases[i].name, "") != 0) {
            if (strcmp(aliases[i].name, arg) == 0) {
                strcpy(aliases[i].name, "");
                strcpy(aliases[i].command, "");
            }
        }
    }
}

int historyHandler(char *tokenized, char *argv, char historyArray[][MAX_LINE], int *pipes) {
    int x = 1;
    if (argv == NULL) {
        for (int i = 0; i < MAX_HISTORY; i++) {
            if (strcmp(historyArray[i], "") != 0)
                printf("%d : %s\n", x++, historyArray[i]);
        }
        return 1;
    }

    int z = atoi(argv);
    char *command = historyArray[z - 1];
    strcpy(tokenized, "");
    tokenizer(command, tokenized, pipes);
    return 0;
}

void addCommandInHistory(char *command, char historyArray[][MAX_LINE]) {
    int i = 0;
    while (i < MAX_HISTORY) {
        if (strcmp(historyArray[i], "") == 0) {
            strcpy(historyArray[i], command);
            return;
        }
        i++;
    }
    //if we reached here then array is maxed out
    strcpy(historyArray[0], "");
    for (int x = 0; x <= MAX_HISTORY - 2; x++) {
        strcpy(historyArray[x], historyArray[x + 1]);
    }
    strcpy(historyArray[MAX_HISTORY - 1], command);
}

int builtInsHandler(char *tokenized, alias *aliases, char historyArray[][MAX_LINE], int *pipes) {
    char *duplicate = (char *) malloc(strlen(tokenized) + 1);
    strcpy(duplicate, tokenized);
    char *name = strtok(duplicate, " ");

    int returnValue = 0;
    if (name == NULL) {
        strcpy(tokenized, "");
        return 0;
    }

    if (strcmp(name, "cd") == 0) {
        cdHandler(tokenized);
        returnValue = 1;
    } else if (strcmp(name, "history") == 0) {
        returnValue = historyHandler(tokenized, strtok(NULL, " "), historyArray, pipes);
    } else if (strcmp(name, "createalias") == 0) {
        createAliasHandler(tokenized, aliases);
        returnValue = 1;
    } else if (strcmp(name, "destroyalias") == 0) {
        destroyAliasHandler(tokenized, aliases);
        returnValue = 1;
    } else if (strcmp(name, "exit") == 0) {
        free(tokenized);
        for (int i = 0; i < MAX_ALIASES; i++) {
            free(aliases[i].name);
            free(aliases[i].command);
        }
        free(aliases);
        free(duplicate);
        exit(0);
    }

//    //Add to history the built ins that are not history
//    if(strcmp(name, "history") != 0 && returnValue){
//        addCommandInHistory(tokenized, historyArray);
//    }
    free(duplicate);
    return returnValue;
}

argumentStruct *structBasicFiller(char *tokenized) {
    char *arg2 = strtok(tokenized, " ");
    if (arg2 == NULL) {
        return NULL;
    }
    argumentStruct *aStruct = (argumentStruct *) malloc(sizeof(argumentStruct));
    clearStruct(aStruct);
    int i = 0;
    while (arg2) {
        if (has_wildcard_char(arg2)) {
            globHandler(arg2, aStruct, &i);
        } else if (strcmp(arg2, ">") == 0) {
            aStruct->boolOutput = 1;
            aStruct->output = strtok(NULL, " ");
        } else if (strcmp(arg2, "<") == 0) {
            aStruct->boolInput = 1;
            aStruct->input = strtok(NULL, " ");
        } else if (strcmp(arg2, ">>") == 0) {
            aStruct->boolOutput2 = 1;
            aStruct->output = strtok(NULL, " ");
        } else if (strcmp(arg2, "&") == 0) {
            aStruct->boolBackground = 1;
        } else {
            aStruct->argumentList[i++] = arg2;
        }
        arg2 = strtok(NULL, " ");
    }
    aStruct->argumentList[i] = NULL;
    return aStruct;
}

argumentStruct *structPipeFiller(char *tokenized, int pipes) {

    argumentStruct *argumentStructArray = (argumentStruct *) malloc((pipes + 1) * sizeof(argumentStruct));
    char *arg = strtok(tokenized, " ");
    int y = 0;
    int i = 0;
    for (int z = 0; z <= pipes; z++) {
        clearStruct(&argumentStructArray[y]);
        while (arg) {
            if (has_wildcard_char(arg)) {
                globHandler(arg, &argumentStructArray[y], &i);
                arg = strtok(NULL, " ");
                continue;
            } else if (strcmp(arg, "<") == 0) {
                argumentStructArray[y].boolInput = 1;
                arg = strtok(NULL, " ");
                argumentStructArray[y].input = arg;
                arg = strtok(NULL, " ");
                continue;
            } else if (strcmp(arg, ">") == 0) {
                argumentStructArray[y].boolOutput = 1;
                arg = strtok(NULL, " ");
                argumentStructArray[y].output = arg;
                arg = strtok(NULL, " ");
                continue;
            } else if (strcmp(arg, ">>") == 0) {
                argumentStructArray[y].boolOutput2 = 1;
                arg = strtok(NULL, " ");
                argumentStructArray[y].output = arg;
                arg = strtok(NULL, " ");
                continue;
            } else if (strcmp(arg, "|") == 0) {
                break;
            } else if (strcmp(arg, "&") == 0) {
                argumentStructArray[y].boolBackground = 1;
                arg = strtok(NULL, " ");
                continue;
            }
            argumentStructArray[y].argumentList[i++] = arg;
            arg = strtok(NULL, " ");
        }
        argumentStructArray[y++].argumentList[i] = NULL;
        i = 0;
        arg = strtok(NULL, " ");
    }
    return argumentStructArray;
}

char *tokenizer(char *Input, char *tokenized, int *pipesOrNo) {
    int j = 0;
    char *input = strtok(Input, "\t\n");
    if (input == NULL) {
        return tokenized;
    }
    while (input) {
        for (int i = 0; i < strlen(input); i++) {
            if (input[i] != '>' && input[i] != '<' && input[i] != '|' && input[i] != '&' && input[i] != '"') {
                tokenized[j++] = input[i];
            } else {
                if (input[i] == '|')
                    (*pipesOrNo)++;
                tokenized[j++] = ' ';
                tokenized[j++] = input[i];
                if (input[i] == '>' && input[i + 1] == '>') {
                    tokenized[j++] = input[++i];
                }
                tokenized[j++] = ' ';
            }
        }
        input = strtok(NULL, "\t\n");
    }
    tokenized[j] = '\0';
    return tokenized;
}

void isAlias(char *executable, alias *aliases, int *input) {
    for (int i = 0; i < MAX_ALIASES; i++) {
        if (strcmp(aliases[i].name, "") != 0) {
            if (strcmp(aliases[i].name, executable) == 0) {
                executable = aliases[i].command;
                (*input) = i;
            }
        }
    }
}

void clearAliases(alias *aliases) {
    for (int i = 0; i < MAX_ALIASES; i++) {
        aliases[i].name = (char *) malloc(100 * sizeof(char));
        aliases[i].command = (char *) malloc(MAX_LINE * sizeof(char));
        strcpy(aliases[i].name, "");
        strcpy(aliases[i].command, "");
    }
}

void clearAndInitHistory(char array[][MAX_LINE]) {
    for (int i = 0; i < MAX_HISTORY; i++) {
        strcpy(array[i], "");
    }
}