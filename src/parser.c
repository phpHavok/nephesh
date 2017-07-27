#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <utlist.h>

struct parser_t {
    token_t * tokens;
    token_t * token;
    char * error;
    command_t * commands;
    command_t * command;
    int fd;
};

static int parser_parse_pipeline(parser_t * parser);
static int parser_parse_command(parser_t * parser);
static int parser_parse_argument(parser_t * parser);
static int parser_parse_pipe(parser_t * parser);
static int parser_parse_unary_pipe(parser_t * parser);
static int parser_parse_nary_pipe(parser_t * parser);
static int parser_parse_file_descriptor(parser_t * parser);

static token_t * parser_peek(parser_t * parser);
static token_t * parser_advance(parser_t * parser);
static int parser_match(parser_t * parser,
                        token_type_t token_type);

parser_t * parser_new(token_t * tokens)
{
    parser_t * parser = malloc(sizeof(parser_t));
    parser->tokens = tokens;
    parser->token = tokens;
    parser->error = "";
    parser->commands = NULL;
}

void parser_delete(parser_t * parser)
{
    command_t * t1, * t2;
    DL_FOREACH_SAFE(parser->commands, t1, t2) {
        DL_DELETE(parser->commands, t1);
        command_delete(t1);
    }
    free(parser);
}

command_t * parser_parse(parser_t * parser)
{
    if (parser_parse_pipeline(parser) && NULL == parser->token) {
        return parser->commands;
    } else {
        return NULL;
    }
}

const char * parser_get_error(parser_t * parser)
{
    return parser->error;
}

static token_t * parser_peek(parser_t * parser)
{
    return parser->token;
}

static token_t * parser_advance(parser_t * parser)
{
    token_t * token = parser->token;
    if (NULL != token) {
        parser->token = token->next;
    }
    return token;
}

static int parser_match(parser_t * parser,
                        token_type_t token_type)
{
    token_t * token = parser_peek(parser);
    if (NULL == token) {
        return 0;
    } else if (token->type == token_type) {
        parser_advance(parser);
        return 1;
    } else {
        return 0;
    }
}

static int parser_parse_pipeline(parser_t * parser)
{
    if (parser_parse_command(parser)) {
        // Arguments.
        while (parser_parse_argument(parser));
        if (parser_parse_pipe(parser)) {
            return parser_parse_pipeline(parser);
        } else {
            return 1;
        }
    } else {
        parser->error = "Missing command.";
        return 0;
    }
}

static int parser_parse_command(parser_t * parser)
{
    token_t * lookahead = parser_peek(parser);
    if (NULL == lookahead) {
        return 0;
    } else if (TOKEN_TYPE_STR_LIT == lookahead->type) {
        parser_advance(parser);
        command_t * command = command_new();
        command->argv[0] = lookahead->aux;
        command->argv[1] = NULL;
        command->argc = 1;
        DL_APPEND(parser->commands, command);
        parser->command = command;
        return 1;
    } else {
        return 0;
    }
}

static int parser_parse_argument(parser_t * parser)
{
    token_t * lookahead = parser_peek(parser);
    if (NULL == lookahead) {
        return 0;
    } else if (TOKEN_TYPE_STR_LIT == lookahead->type) {
        parser_advance(parser);
        command_t * command = parser->command;
        if (command->argc + 1 >= COMMAND_MAX_ARGS) {
            parser->error = "Out of arguments.";
            return 0;
        }
        command->argv[command->argc] = lookahead->aux;
        command->argv[command->argc + 1] = NULL;
        command->argc++;
        return 1;
    } else {
        return 0;
    }
}

static int parser_parse_pipe(parser_t * parser)
{
    if (parser_parse_unary_pipe(parser)) {
        return 1;
    } else if (parser_parse_nary_pipe(parser)) {
        return 1;
    } else {
        parser->error = "Missing pipe(s)."; 
        return 0;
    }
}

static int parser_parse_unary_pipe(parser_t * parser)
{
    int pipe[2] = { 1, 0 };
    if (parser_parse_file_descriptor(parser)) {
        pipe[0] = parser->fd;
    }
    if (!parser_match(parser, TOKEN_TYPE_PIPE)) {
        parser->error = "Missing '|'.";
        return 0;
    }
    if (parser_parse_file_descriptor(parser)) {
        pipe[1] = parser->fd;
    }
    command_t * command = parser->command;
    if (command->pipec + 1 >= COMMAND_MAX_PIPES) {
        parser->error = "Out of pipes.";
        return 0;
    }
    command->pipes[command->pipec][0] = pipe[0];
    command->pipes[command->pipec][1] = pipe[1];
    command->pipec++;
    return 1;
}

static int parser_parse_nary_pipe(parser_t * parser)
{
    if (!parser_match(parser, TOKEN_TYPE_LT)) {
        parser->error = "Missing '<'.";
        return 0;
    }
    if (!parser_parse_unary_pipe(parser)) {
        parser->error = "Missing pipe.";
        return 0;
    }
    while (parser_parse_unary_pipe(parser));
    if (!parser_match(parser, TOKEN_TYPE_GT)) {
        parser->error = "Missing '>'.";
        return 0;
    }
    return 1;
}

static int parser_parse_file_descriptor(parser_t * parser)
{
    token_t * lookahead = parser_peek(parser);
    if (NULL == lookahead) {
        parser->error = "Missing file descriptor.";
        return 0;
    } else if (TOKEN_TYPE_INT_LIT == lookahead->type) {
        parser_advance(parser);
        parser->fd = atoi(lookahead->aux);
        return 1;
    } else if (TOKEN_TYPE_AT == lookahead->type) {
        parser_advance(parser);
        parser->fd = -1;
        return 1;
    } else {
        parser->error = "Missing file descriptor.";
        return 0;
    }
}
