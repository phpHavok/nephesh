#ifndef EDITOR_H_
#define EDITOR_H_

#define ED_LINE_MAX_SIZE 10
#define ED_BUFFER_MAX_SIZE 10

typedef struct ed_t ed_t;

typedef void (*kb_callback)(struct ed_t * ed);

typedef struct kb_t {
    char * sequence;
    kb_callback action;
    struct kb_t * next;
} kb_t;


ed_t * ed_new(int input,
              int output);
void ed_delete(ed_t * ed);
const char * ed_readline(ed_t * ed);

#endif
