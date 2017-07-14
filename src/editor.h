#ifndef EDITOR_H_
#define EDITOR_H_

#define ED_BUFFER_SIZE 10
#define KB_SEQUENCE_MAX_SIZE 32

typedef void (*kb_callback)(void);

typedef struct kb_t {
    char sequence[KB_SEQUENCE_MAX_SIZE];
    kb_callback action;
    struct kb_t * next;
} kb_t;

void ed_init(void);
const char * ed_readline(void);

#endif
