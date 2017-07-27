#include <stdio.h>
#include <stdlib.h>
#include "command.h"
#include <utlist.h>

command_t * command_new(void)
{
    command_t * command = malloc(sizeof(command_t));
    command->argv[0] = NULL;
    command->argc = 0;
    command->pipec = 0;
    return command;
}

void command_delete(command_t * command)
{
    free(command);
}

void command_debug_dump(command_t * commands)
{
    command_t * command;
    DL_FOREACH(commands, command) {
        printf("Command: #args = %u, #pipes = %u\n", command->argc, command->pipec);
        for (unsigned int i = 0; i < command->argc; ++i) {
            printf("    arg%u = %s\n", i, command->argv[i]);
        }
        for (unsigned int i = 0; i < command->pipec; ++i) {
            printf("    pipe%u = %d -> %d\n", i, command->pipes[i][0], command->pipes[i][1]);
        }
    }
}
