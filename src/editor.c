#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "editor.h"
#include <utlist.h>
#include <curses.h>
#include <term.h>

static void kb_action_backspace(ed_t * ed)
{
    if (ed->buffer_sz > 0) {
        ed->buffer_sz--;
    }
}

static void kb_action_enter(ed_t * ed)
{
    ed->editing = 0;
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

    return bindings;
}

void ed_init(ed_t * ed)
{
    memset(ed, 0, sizeof(ed_t));
    ed->buffer = malloc(ED_BUFFER_MAX_SIZE);
    ed->bindings = kb_load_bindings();
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
        read(STDIN_FILENO, sequence + i, 1);
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

const char * ed_readline(ed_t * ed)
{
    ed->buffer_sz = 0;
    ed->editing = 1;
    while (ed->editing) {
        const char * bol = tparm(column_address, 0);
        write(STDOUT_FILENO, bol, strlen(bol));
        write(STDOUT_FILENO, clr_eol, strlen(clr_eol));
        const char * prompt = "nephesh> ";
        write(STDOUT_FILENO, prompt, strlen(prompt));
        write(STDOUT_FILENO, ed->buffer, ed->buffer_sz);
        char xxstatus[32];
        snprintf(xxstatus, 32, "  (size=%u)", ed->buffer_sz);
        //write(STDOUT_FILENO, enter_bold_mode, strlen(enter_bold_mode));
        const char * colorbro1 = "\e[91m";
        const char * colorbro2 = "\e[0m";
        write(STDOUT_FILENO, colorbro1, strlen(colorbro1));
        write(STDOUT_FILENO, xxstatus, strlen(xxstatus));
        write(STDOUT_FILENO, colorbro2, strlen(colorbro2));
        //write(STDOUT_FILENO, exit_bold_mode, strlen(exit_bold_mode));
        // TODO: count UTF-8 bytes as single character
        const char * lineloc = tparm(column_address, ed->buffer_sz + strlen(prompt));
        write(STDOUT_FILENO, lineloc, strlen(lineloc));

        char sequence[KB_SEQUENCE_MAX_SIZE];
        unsigned int sequence_sz;
        kb_t * key_binding = kb_find_match(sequence, KB_SEQUENCE_MAX_SIZE, &sequence_sz, ed);
        if (NULL == key_binding) {
            // If there is no matching key binding, just copy characters to the
            // editing buffer until we run out of room. When out of room,
            // silently discard remaining characters. TODO: this should be
            // fixed to never copy a partial UTF-8 character if there isn't
            // enough room.
            for (unsigned i = 0; i < sequence_sz && ed->buffer_sz < ED_BUFFER_MAX_SIZE - 1; ++i) {
                ed->buffer[ed->buffer_sz] = sequence[i];
                ed->buffer_sz++;
            }
        } else {
            key_binding->action(ed);
        }
    }
    write(STDOUT_FILENO, "\n", 1);
    ed->buffer[ed->buffer_sz] = '\0';
    return ed->buffer;
}
