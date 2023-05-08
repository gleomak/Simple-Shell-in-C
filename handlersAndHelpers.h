#define MAX_LINE 500
#define MAX_EXECUTABLES 10
#define MAX_HISTORY 20
#define MAX_ARGS 10
#define MAX_ALIASES 10
#define READ 0
#define WRITE 1

typedef struct {
    char *argumentList[MAX_ARGS];
    int boolInput;
    int boolOutput;
    int boolOutput2;
    char *input;
    char *output;
    int boolBackground;
} argumentStruct;

typedef struct {
    char *name;
    char *command;
} alias;


void clearStruct(argumentStruct *argumentStruct);

int has_wildcard_char(const char *str);

char *tokenizer(char *Input, char* tokenized,int *pipesOrNo);

void isAlias(char *executable, alias* aliases, int *input);

void clearAliases(alias *aliases);

void clearAndInitHistory(char array[][MAX_LINE]);

void globHandler(char *argument, argumentStruct *aStruct, int *i);

int historyHandler(char *tokenized, char *argv, char historyArray[][MAX_LINE], int *pipes);

int builtInsHandler(char *tokenized, alias* aliases , char historyArray[][MAX_LINE], int *pipes);

void cdHandler(char *tokenized);

void destroyAliasHandler(char *tokenized, alias* aliases);

void createAliasHandler(char *tokenized, alias* aliases);

void addCommandInHistory(char *command, char historyArray[][MAX_LINE]);

argumentStruct *structBasicFiller(char *tokenized);

argumentStruct *structPipeFiller(char *tokenized,int pipes);