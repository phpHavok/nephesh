#ifndef PARSER_H_
#define PARSER_H_

#include "scanner.h"
#include "command.h"

typedef struct parser_t parser_t;

parser_t * parser_new(token_t * tokens);
void parser_delete(parser_t * parser);
command_t * parser_parse(parser_t * parser);
const char * parser_get_error(parser_t * parser);

#endif
