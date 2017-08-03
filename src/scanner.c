#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utlist.h>
#include "scanner.h"

struct scanner_t {
    char * str;
    unsigned int index;
};

/**
 * Returns the next byte in the stream.
 */
static char scanner_peek(scanner_t * scanner);
/**
 * Returns the next byte in the stream and advances the stream by one byte.
 */
static char scanner_advance(scanner_t * scanner);
/**
 * Returns 1 and advances the stream by one byte if that byte matches the
 * supplied byte. Otherwise, returns 0 and does not advance the stream.
 */
/*
static int scanner_match(scanner_t * scanner,
                         char byte);
*/
static void scanner_scan_string(scanner_t * scanner,
                                token_t * token,
                                char first_byte);
static void scanner_scan_string_quoted(scanner_t * scanner,
                                       token_t * token);

scanner_t * scanner_new(const char * str)
{
    scanner_t * scanner = malloc(sizeof(scanner_t));
    size_t len = strlen(str) + 1;
    scanner->str = malloc(sizeof(char) * len);
    memcpy(scanner->str, str, len);
    scanner->index = 0;
    return scanner;
}

void scanner_delete(scanner_t * scanner)
{
    free(scanner->str);
    free(scanner);
}

token_t * scanner_scan(scanner_t * scanner)
{
    token_t * tokens = NULL;
    while (1) {
        char next_byte = scanner_advance(scanner);
        token_t * next_token = malloc(sizeof(token_t));
        switch (next_byte) {
            case '\0':
                free(next_token);
                return tokens;

            case '<':
                next_token->type = TOKEN_TYPE_LT;
                next_token->aux[0] = next_byte;
                next_token->aux[1] = '\0';
                DL_APPEND(tokens, next_token);
                break;

            case '>':
                next_token->type = TOKEN_TYPE_GT;
                next_token->aux[0] = next_byte;
                next_token->aux[1] = '\0';
                DL_APPEND(tokens, next_token);
                break;

            case '|':
                next_token->type = TOKEN_TYPE_PIPE;
                next_token->aux[0] = next_byte;
                next_token->aux[1] = '\0';
                DL_APPEND(tokens, next_token);
                break;

            case '@':
                next_token->type = TOKEN_TYPE_AT;
                next_token->aux[0] = next_byte;
                next_token->aux[1] = '\0';
                DL_APPEND(tokens, next_token);
                break;

            case ' ':
            case '\t':
                break;

            case '\'':
                scanner_scan_string_quoted(scanner, next_token);
                DL_APPEND(tokens, next_token);
                break;

            default:
                scanner_scan_string(scanner, next_token, next_byte);
                DL_APPEND(tokens, next_token);
                break;
        }
    }
}

void token_debug_dump(token_t * tokens)
{
    token_t * token = NULL;
    DL_FOREACH(tokens, token) {
        fprintf(stderr, "Token: (type=%d) (aux=%s)\n", token->type, token->aux);
    }
}

static void scanner_scan_string(scanner_t * scanner,
                                token_t * token,
                                char first_byte)
{
    token->type = TOKEN_TYPE_STR;
    token->aux[0] = first_byte;
    unsigned int i;
    int keep_scanning = 1;
    for (i = 1; i < TOKEN_MAX_SIZE - 1 && keep_scanning; ++i) {
        char next_byte = scanner_peek(scanner);
        switch (next_byte) {
            case '\0':
            case '<':
            case '>':
            case '|':
            case '@':
            case ' ':
            case '\t':
            case '\'':
                keep_scanning = 0;
                break;

            default:
                scanner_advance(scanner);
                token->aux[i] = next_byte;
                break;
        }
    }
    token->aux[i - 1] = '\0';
}

static void scanner_scan_string_quoted(scanner_t * scanner,
                                       token_t * token)
{
    token->type = TOKEN_TYPE_STR;
    unsigned int i;
    int keep_scanning = 1;
    for (i = 0; i < TOKEN_MAX_SIZE - 1 && keep_scanning; ++i) {
        char next_byte = scanner_peek(scanner);
        switch (next_byte) {
            case '\'':
                scanner_advance(scanner);
                keep_scanning = 0;
                break;

            default:
                scanner_advance(scanner);
                token->aux[i] = next_byte;
                break;
        }
    }
    token->aux[i - 1] = '\0';
}

static char scanner_peek(scanner_t * scanner)
{
    return scanner->str[scanner->index];
}

static char scanner_advance(scanner_t * scanner)
{
    char next_byte = scanner->str[scanner->index];
    if ('\0' != next_byte) {
        scanner->index++;
    }
    return next_byte;
}

/*
static int scanner_match(scanner_t * scanner,
                         char byte)
{
    if (byte == scanner_peek(scanner)) {
        scanner_advance(scanner);
        return 1;
    } else {
        return 0;
    }
}
*/
