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
static int parser_parse_str_more(parser_t * parser);
static int parser_parse_pipeline_more(parser_t * parser);
static int parser_parse_unary_pipe(parser_t * parser);
static int parser_parse_nary_pipe(parser_t * parser);
static int parser_parse_nary_pipe_more(parser_t * parser);
static int parser_parse_maybe_fd(parser_t * parser);

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
    return parser;
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
    token_t * backtrack = parser->token;
    // STR
    if (!parser_match(parser, TOKEN_TYPE_STR)) {
        parser->token = backtrack;
        return 0;
    }
    // <str-more>
    if (!parser_parse_str_more(parser)) {
        parser->token = backtrack;
        return 0;
    }
    // <pipeline-more>
    if (!parser_parse_pipeline_more(parser)) {
        parser->token = backtrack;
        return 0;
    }
    return 1;
}

static int parser_parse_str_more(parser_t * parser)
{
    // STR
    if (parser_match(parser, TOKEN_TYPE_STR)) {
        // <str-more>
        return parser_parse_str_more(parser);
    } else {
        // LAMBDA
        return 1;
    }
}

static int parser_parse_pipeline_more(parser_t * parser)
{
    token_t * backtrack = parser->token;
    // <nary-pipe>
    if (parser_parse_nary_pipe(parser)) {
        // <pipeline>
        if (parser_parse_pipeline(parser)) {
            return 1;
        }
    }
    parser->token = backtrack;
    // LAMBDA
    return 1;
}

static int parser_parse_unary_pipe(parser_t * parser)
{
    token_t * backtrack = parser->token;
    // <maybe-fd>
    if (!parser_parse_maybe_fd(parser)) {
        parser->token = backtrack;
        return 0;
    }
    // PIPE
    if (!parser_match(parser, TOKEN_TYPE_PIPE)) {
        parser->token = backtrack;
        return 0;
    }
    // <maybe-fd>
    if (!parser_parse_maybe_fd(parser)) {
        parser->token = backtrack;
        return 0;
    }
    return 1;
}

static int parser_parse_nary_pipe(parser_t * parser)
{
    token_t * backtrack = parser->token;
    // LT
    if (!parser_match(parser, TOKEN_TYPE_LT)) {
        parser->token = backtrack;
        return 0;
    }
    // <unary-pipe>
    if (!parser_parse_unary_pipe(parser)) {
        parser->token = backtrack;
        return 0;
    }
    // <nary-pipe-more>
    if (!parser_parse_nary_pipe_more(parser)) {
        parser->token = backtrack;
        return 0;
    }
    // GT
    if (!parser_match(parser, TOKEN_TYPE_GT)) {
        parser->token = backtrack;
        return 0;
    }
    return 1;
}

static int parser_parse_nary_pipe_more(parser_t * parser)
{
    token_t * backtrack = parser->token;
    // <unary-pipe>
    if (parser_parse_unary_pipe(parser)) {
        // <nary-pipe-more>
        if (parser_parse_nary_pipe_more(parser)) {
            return 1;
        }
    }
    parser->token = backtrack;
    // LAMBDA
    return 1;
}

static int parser_parse_maybe_fd(parser_t * parser)
{
    // STR
    if (parser_match(parser, TOKEN_TYPE_STR)) {
        return 1;
    }
    // AT
    if (parser_match(parser, TOKEN_TYPE_AT)) {
        return 1;
    }
    // LAMBDA
    return 1;
}
