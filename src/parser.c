#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

struct parser_t {
    token_t * tokens;
    token_t * token;
    char * error;
};

static int parser_parse_pipeline(parser_t * parser);
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
}

void parser_delete(parser_t * parser)
{
    free(parser);
}

int parser_parse(parser_t * parser)
{
    return parser_parse_pipeline(parser) && (NULL == parser->token);
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
    if (parser_match(parser, TOKEN_TYPE_STR_LIT)) {
        // Arguments.
        while (parser_match(parser, TOKEN_TYPE_STR_LIT));
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
    parser_parse_file_descriptor(parser);
    if (!parser_match(parser, TOKEN_TYPE_PIPE)) {
        parser->error = "Missing '|'.";
        return 0;
    }
    parser_parse_file_descriptor(parser);
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
    if (parser_match(parser, TOKEN_TYPE_INT_LIT)) {
        return 1;
    } else if (parser_match(parser, TOKEN_TYPE_AT)) {
        return 1;
    } else {
        parser->error = "Missing file descriptor.";
        return 0;
    }
}
