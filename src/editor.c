#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "editor.h"
#include <utlist.h>
#include <curses.h>
#include <term.h>
#include "utf8.h"

static void kb_action_backspace(ed_t * ed)
{
}

static void kb_action_enter(ed_t * ed)
{
    ed->editing = 0;
}

static void kb_cursor_right(ed_t * ed)
{
    if (ed->cursor_x < utf8_strlen(ed->buffer, ed->buffer_sz)) {
        ed->cursor_x++;
    }
}

static void kb_cursor_left(ed_t * ed)
{
    if (ed->cursor_x > 0) {
        ed->cursor_x--;
    }
}

static void kb_cursor_bol(ed_t * ed)
{
    ed->cursor_x = 0;
}

static void kb_cursor_eol(ed_t * ed)
{
    ed->cursor_x = utf8_strlen(ed->buffer, ed->buffer_sz);
}

static kb_t * kb_load_bindings(void)
{
    kb_t * bindings = NULL;
    kb_t * temp = NULL;

    temp = malloc(sizeof(kb_t));
    strncpy(temp->sequence, key_backspace, KB_SEQUENCE_MAX_SIZE);
    temp->action = kb_action_backspace;
    LL_PREPEND(bindings, temp);

    temp = malloc(sizeof(kb_t));
    strncpy(temp->sequence, "\n", KB_SEQUENCE_MAX_SIZE);
    temp->action = kb_action_enter;
    LL_PREPEND(bindings, temp);

    temp = malloc(sizeof(kb_t));
    strncpy(temp->sequence, key_right, KB_SEQUENCE_MAX_SIZE);
    temp->action = kb_cursor_right;
    LL_PREPEND(bindings, temp);

    temp = malloc(sizeof(kb_t));
    strncpy(temp->sequence, key_left, KB_SEQUENCE_MAX_SIZE);
    temp->action = kb_cursor_left;
    LL_PREPEND(bindings, temp);

    temp = malloc(sizeof(kb_t));
    strncpy(temp->sequence, "\x1", KB_SEQUENCE_MAX_SIZE);
    temp->action = kb_cursor_bol;
    LL_PREPEND(bindings, temp);

    temp = malloc(sizeof(kb_t));
    strncpy(temp->sequence, "\x5", KB_SEQUENCE_MAX_SIZE);
    temp->action = kb_cursor_eol;
    LL_PREPEND(bindings, temp);

    return bindings;
}

void ed_init(ed_t * ed,
             int input,
             int output)
{
    memset(ed, 0, sizeof(ed_t));
    ed->buffer = malloc(ED_BUFFER_MAX_SIZE);
    ed->input = input;
    ed->output = output;
    ed->bindings = kb_load_bindings();
    const char * prompt = "nephesh> ";
    ed->prompt = malloc(strlen(prompt) + 1);
    strncpy(ed->prompt, prompt, strlen(prompt) + 1);
}

static kb_t * kb_reduce(const char * sequence,
                        unsigned int index,
                        const kb_t * potentials)
{
    kb_t * reduced = NULL;
    const kb_t * potential = NULL;
    LL_FOREACH(potentials, potential) {
        if (sequence[index] == potential->sequence[index]) {
            kb_t * potential_copy = malloc(sizeof(kb_t));
            memcpy(potential_copy, potential, sizeof(kb_t));
            LL_PREPEND(reduced, potential_copy);
        }
    }
    return reduced;
}

static kb_t * kb_find_match(char * sequence,
                            unsigned int sequence_max_sz,
                            unsigned int * sequence_sz,
                            ed_t * ed)
{
    kb_t * potentials = ed->bindings;
    for (unsigned int i = 0; i < sequence_max_sz; ++i) {
        read(ed->input, sequence + i, 1);
        kb_t * reduced = kb_reduce(sequence, i, potentials);
        if (potentials != ed->bindings) {
            kb_t * potential;
            kb_t * temp;
            LL_FOREACH_SAFE(potentials, potential, temp) {
                LL_DELETE(potentials, potential);
                free(potential);
            }
        }
        potentials = reduced;
        unsigned int potentials_sz = 0;
        kb_t * potential = NULL;
        LL_COUNT(potentials, potential, potentials_sz);
        if ((1 == potentials_sz && '\0' == potentials->sequence[i + 1]) || 0 == potentials_sz) {
            *sequence_sz = i + 1;
            break;
        }
    }
    return potentials;
}

static void ed_refresh(ed_t * ed)
{
    // Move the cursor to the beginning of the line.
    const char * cursor_start = tparm(column_address, 0);
    write(ed->output, cursor_start, strlen(cursor_start));
    // Clear the line.
    write(ed->output, clr_eol, strlen(clr_eol));
    // Print the prompt.
    write(ed->output, ed->prompt, strlen(ed->prompt));
    // Print the contents of the editing buffer.
    write(ed->output, ed->buffer, ed->buffer_sz);
    // Move the cursor to wherever the user has placed it.
    unsigned int cursor_offset = strlen(ed->prompt)
                                 //+ utf8_strlen(ed->buffer, ed->buffer_sz)
                                 + ed->cursor_x;
    const char * cursor_user = tparm(column_address, cursor_offset);
    write(ed->output, cursor_user, strlen(cursor_user));
}

static void ed_insert(ed_t * ed,
                      char * buffer,
                      unsigned int buffer_sz)
{
    if (ed->buffer_sz + buffer_sz > ED_BUFFER_MAX_SIZE - 1) {
        return;
    }
    unsigned int offset = utf8_offset(ed->buffer, ed->buffer_sz, ed->cursor_x);
    if (ed->buffer_sz > 0) {
        for (unsigned i = ed->buffer_sz - 1; i >= offset; --i) {
            ed->buffer[i + buffer_sz] = ed->buffer[i];
            // Necessary, because we are using unsigned.
            if (0 == i) {
                break;
            }
        }
    }
    for (unsigned i = 0; i < buffer_sz; ++i) {
        //unsigned int offset = utf8_offset(ed->buffer, ed->cursor_x);
        ed->buffer[offset + i] = buffer[i];
        ed->buffer_sz++;
        // Move cursor only once for UTF-8 characters.
        if (0x80U != (0xC0U & buffer[i])) {
            ed->cursor_x++;
        }
    }
}

static void ed_reset(ed_t * ed)
{
    ed->buffer_sz = 0;
    ed->editing = 1;
    ed->cursor_x = 0;
}

const char * ed_readline(ed_t * ed)
{
    ed_reset(ed);
    while (ed->editing) {
        ed_refresh(ed);
        char sequence[KB_SEQUENCE_MAX_SIZE];
        unsigned int sequence_sz;
        kb_t * key_binding = kb_find_match(sequence, KB_SEQUENCE_MAX_SIZE, &sequence_sz, ed);
        if (NULL == key_binding) {
            ed_insert(ed, sequence, sequence_sz);
        } else {
            key_binding->action(ed);
        }
    }
    write(ed->output, "\n", 1);
    ed->buffer[ed->buffer_sz] = '\0';
    return ed->buffer;
}
