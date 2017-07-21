#ifndef SCANNER_H_
#define SCANNER_H_

#define TOKEN_MAX_SIZE 1024

typedef enum token_type_t {
    TOKEN_TYPE_LT,
    TOKEN_TYPE_GT,
    TOKEN_TYPE_PIPE,
    TOKEN_TYPE_AT,
    TOKEN_TYPE_INT_LIT,
    TOKEN_TYPE_STR_LIT
} token_type_t;

typedef struct token_t {
    char aux[TOKEN_MAX_SIZE];
    token_type_t type;
    struct token_t * prev;
    struct token_t * next;
} token_t;

typedef struct scanner_t scanner_t;

scanner_t * scanner_new(const char * str);
void scanner_delete(scanner_t * scanner);

/**
 * Returns a list of tokens as the result of scanning a byte stream.
 */
token_t * scanner_scan(scanner_t * scanner);

void token_debug_dump(token_t * tokens);

#endif
