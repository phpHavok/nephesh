#ifndef EDITOR_H_
#define EDITOR_H_

#define ED_BUFFER_MAX_SIZE 10
#define KB_SEQUENCE_MAX_SIZE 32

struct ed_t;

typedef void (*kb_callback)(struct ed_t * ed);

typedef struct kb_t {
    char sequence[KB_SEQUENCE_MAX_SIZE];
    kb_callback action;
    struct kb_t * next;
} kb_t;

typedef struct ed_t {
    char * buffer;
    unsigned int buffer_sz;
    unsigned int cursor_x;
    //unsigned int cursor_y;
    kb_t * bindings;
    int editing;
    int input;
    int output;
    char * prompt;
} ed_t;

void ed_init(ed_t * ed,
             int input,
             int output);
const char * ed_readline(ed_t * ed);

#endif
