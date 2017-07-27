#ifndef COMMAND_H_
#define COMMAND_H_

#define COMMAND_MAX_ARGS 32
#define COMMAND_MAX_PIPES 32

typedef struct command_t {
    char * argv[COMMAND_MAX_ARGS];
    unsigned int argc;
    int pipes[COMMAND_MAX_PIPES][2];
    unsigned int pipec;
    struct command_t * prev;
    struct command_t * next;
} command_t;

command_t * command_new(void);
void command_delete(command_t * command);
void command_debug_dump(command_t * commands);

#endif
