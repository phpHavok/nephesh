#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "editor.h"
#include <utlist.h>
#include <curses.h>
#include <term.h>
#include "utf8.h"

static void _ed_reset(ed_t * ed);
static void _ed_draw(ed_t * ed);
static void _ed_insert(ed_t * ed);
static kb_t * _kb_load_bindings(void);
static unsigned int _kb_reduce(ed_t * ed,
                               kb_t ** potential_bindings);
static kb_t * _kb_copy(const kb_t * first);

/**
 * Key binding actions.
 */
static void _kb_action_end_editing(ed_t * ed);
static void _kb_action_cursor_right(ed_t * ed);
static void _kb_action_cursor_left(ed_t * ed);
static void _kb_action_cursor_bol(ed_t * ed);
static void _kb_action_cursor_eol(ed_t * ed);
static void _kb_action_backspace(ed_t * ed);
static void _kb_nop(ed_t * ed);

struct ed_t {
    /**
     * Vetting area for strings before they are considered a part of the line.
     * Examples include partial UTF-8 characters and partial key binding
     * sequences.
     */
    char * buffer;
    /**
     * A boolean indicating whether or not the buffer is being populated.
     */
    int buffering;
    /**
     * The size of the buffer.
     */
    size_t buffer_sz;
    /**
     * The editable line that is visible to the user. Should always be null
     * terminated.
     */
    char * line;
    /**
     * The logical cursor position within the line. Multi-byte characters are
     * treated as one cursor position. This must ultimately be translated into
     * an (x,y) coordinate pair for display to the screen based upon the
     * terminal's dimensions. Soft wrapping may apply.
     */
    unsigned int cursor_pos;
    /**
     * The terminal coordinates where we should begin editing.
     */
    unsigned int offset_x;
    unsigned int offset_y;
    /**
     * A linked list of valid key bindings.
     */
    kb_t * key_bindings;
    /**
     * A boolean indicating whether or not the line is being edited.
     */
    int editing;
    /**
     * The file descriptor to expect user input on.
     */
    int input;
    /**
     * The file descriptor to display the user's line on.
     */
    int output;
    /**
     * The user's prompt, to be displayed before their editable line.
     */
    char * prompt;
};

ed_t * ed_new(int input,
              int output)
{
    ed_t * ed = malloc(sizeof(ed_t));
    memset(ed, 0, sizeof(ed_t));
    ed->buffer = malloc(ED_BUFFER_MAX_SIZE);
    ed->buffer[0] = '\0';
    ed->line = malloc(ED_LINE_MAX_SIZE);
    ed->line[0] = '\0';
    ed->input = input;
    ed->output = output;
    ed->key_bindings = _kb_load_bindings();
    ed->prompt = "nephesh/\xd7\xa9\xd7\xa4\xd7\xa0> ";
    return ed;
}

void ed_delete(ed_t * ed)
{
    free(ed->buffer);
    free(ed->line);
    free(ed);
}

const char * ed_readline(ed_t * ed)
{
    _ed_reset(ed);
    while (ed->editing) {
        ed->buffering = 1;
        ed->buffer_sz = 0;
        kb_t * potential_bindings = _kb_copy(ed->key_bindings);
        _ed_draw(ed);
        while (ed->buffering) {
            char u8_char[U8_MAX_BYTES];
            size_t u8_char_sz = u8_getc(ed->input, u8_char);
            if (0 == u8_char_sz) {
                continue;
            } else if (1 == u8_char_sz) {
                unsigned int target = (unsigned int) u8_char[0];
                if (target < 0x0AU || (target > 0x0AU && target <= 0x1FU) || 0x7FU == target) {
                    continue;
                }
            }
            if (ed->buffer_sz + u8_char_sz >= ED_BUFFER_MAX_SIZE - 1) {
                // Don't know what to do in this case. Just throw away the
                // buffer and restart.
                break;
            }
            // Copy character to buffer.
            memcpy(ed->buffer + ed->buffer_sz, u8_char, u8_char_sz);
            ed->buffer_sz += u8_char_sz;
            // Try to match a key binding.
            unsigned int potentials_sz = _kb_reduce(ed, &potential_bindings);
            if (0 == potentials_sz) {
                _ed_insert(ed);
                ed->buffering = 0;
            } else if (1 == potentials_sz &&
                       strlen(potential_bindings->sequence) == ed->buffer_sz) {
                potential_bindings->action(ed);
                ed->buffering = 0;
            }
        }
    }
    write(ed->output, "\n", 1);
    return ed->line;
}

static void _ed_reset(ed_t * ed)
{
    ed->buffer_sz = 0;
    ed->buffering = 1;
    ed->line[0] = '\0';
    ed->cursor_pos = 0;
    ed->editing = 1;
    const char * get_cursor = "\x1b[6n";
    write(ed->output, get_cursor, strlen(get_cursor));
    char cursor_position[32];
    if (0 != read(ed->input, cursor_position, 32)) {
        if (EOF == sscanf(cursor_position, "\x1b[%u;%u", &ed->offset_y, &ed->offset_x)) {
            ed->offset_x = 0;
            ed->offset_y = 0;
        } else {
            // The offsets are 1-based, so we need to make them 0-based.
            ed->offset_x--;
            ed->offset_y--;
        }
    }
}

static void _ed_insert(ed_t * ed)
{
    unsigned int len = strlen(ed->line);
    if (len + ed->buffer_sz > ED_LINE_MAX_SIZE - 1) {
        return;
    }
    unsigned int index = u8_byte_offset(ed->line, ed->cursor_pos);
    ed->line[len + ed->buffer_sz] = '\0';
    for (unsigned int i = len + ed->buffer_sz; i > index; --i) {
        ed->line[i - 1] = ed->line[i - 1 - ed->buffer_sz];
    }
    memcpy(ed->line + index, ed->buffer, ed->buffer_sz);
    ed->cursor_pos += u8_strlen_b(ed->buffer, ed->buffer_sz);
}

static void _ed_delete(ed_t * ed)
{
    unsigned int index = u8_byte_offset(ed->line, ed->cursor_pos);
    if (index < 1) {
        return;
    }
    unsigned int preindex = u8_byte_offset(ed->line, ed->cursor_pos - 1);
    for (unsigned int i = index; i > preindex; --i) {
        for (unsigned int j = i - 1; j < strlen(ed->line); ++j) {
            ed->line[j] = ed->line[j + 1];
        }
    }
    ed->cursor_pos--;
}

static void _ed_draw(ed_t * ed)
{
    const char * move_beginning = tparm(cursor_address, ed->offset_y, ed->offset_x);
    write(ed->output, move_beginning, strlen(move_beginning));
    write(ed->output, clr_eol, strlen(clr_eol));
    write(ed->output, ed->prompt, strlen(ed->prompt));
    write(ed->output, ed->line, strlen(ed->line));
    const char * move_final = tparm(cursor_address, ed->offset_y, ed->offset_x +
                                    u8_strlen(ed->prompt) + ed->cursor_pos);
    write(ed->output, move_final, strlen(move_final));
}

static unsigned int _kb_reduce(ed_t * ed,
                               kb_t ** potential_bindings)
{
    kb_t * potential_binding, * temp;
    LL_FOREACH_SAFE(*potential_bindings, potential_binding, temp) {
        if (strlen(potential_binding->sequence) < ed->buffer_sz ||
                   0 != strncmp(potential_binding->sequence, ed->buffer, ed->buffer_sz)) {
            LL_DELETE(*potential_bindings, potential_binding);
            free(potential_binding);
        }
    }
    unsigned int potentials_sz = 0;
    LL_COUNT(*potential_bindings, temp, potentials_sz);
    return potentials_sz;
}

static kb_t * _kb_load_bindings(void)
{
    kb_t * bindings = NULL;
    kb_t * temp = NULL;

    temp = malloc(sizeof(kb_t));
    temp->sequence = "\n";
    temp->action = _kb_action_end_editing;
    LL_PREPEND(bindings, temp);

    temp = malloc(sizeof(kb_t));
    temp->sequence = key_right;
    temp->action = _kb_action_cursor_right;
    LL_PREPEND(bindings, temp);

    temp = malloc(sizeof(kb_t));
    temp->sequence = key_left;
    temp->action = _kb_action_cursor_left;
    LL_PREPEND(bindings, temp);

    temp = malloc(sizeof(kb_t));
    temp->sequence = "\x01";
    temp->action = _kb_action_cursor_bol;
    LL_PREPEND(bindings, temp);

    temp = malloc(sizeof(kb_t));
    temp->sequence = "\x05";
    temp->action = _kb_action_cursor_eol;
    LL_PREPEND(bindings, temp);

    temp = malloc(sizeof(kb_t));
    temp->sequence = key_backspace;
    temp->action = _kb_action_backspace;
    LL_PREPEND(bindings, temp);

    // TEMP
    temp = malloc(sizeof(kb_t));
    temp->sequence = "\xd7\xa2x";
    temp->action = _kb_nop;
    LL_PREPEND(bindings, temp);

    temp = malloc(sizeof(kb_t));
    temp->sequence = "\xd7\xa2\xd7\x91x";
    temp->action = _kb_nop;
    LL_PREPEND(bindings, temp);

    return bindings;
}

static kb_t * _kb_copy(const kb_t * first)
{
    kb_t * new = NULL;
    const kb_t * element = NULL;
    LL_FOREACH(first, element) {
        kb_t * temp = malloc(sizeof(kb_t));
        memcpy(temp, element, sizeof(kb_t));
        LL_PREPEND(new, temp);
    }
    return new;
}

static void _kb_action_end_editing(ed_t * ed)
{
    ed->editing = 0;
}

static void _kb_action_cursor_right(ed_t * ed)
{
    if (ed->cursor_pos + 1 <= u8_strlen(ed->line)) {
        ed->cursor_pos++;
    }
}

static void _kb_action_cursor_left(ed_t * ed)
{
    if (ed->cursor_pos > 0) {
        ed->cursor_pos--;
    }
}

static void _kb_action_cursor_bol(ed_t * ed)
{
    ed->cursor_pos = 0;
}

static void _kb_action_cursor_eol(ed_t * ed)
{
    ed->cursor_pos = u8_strlen(ed->line);
}

static void _kb_action_backspace(ed_t * ed)
{
    _ed_delete(ed);
}

static void _kb_nop(ed_t * ed)
{
}
