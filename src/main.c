#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <locale.h>
#include <curses.h>
#include <term.h>
#include "editor.h"

int main(int argc, char * argv[])
{
    const char * locale = setlocale(LC_ALL, "");

    // TODO: verify that locale is UTF-8.

    if (!isatty(STDIN_FILENO)) {
        fputs("Not a terminal.\n", stderr);
        return 1;
    }

    int term_status;
    if (OK != setupterm(NULL, STDOUT_FILENO, &term_status)) {
        fprintf(stderr, "Unable to setup terminal: setupterm returned %d\n", term_status);
        return 1;
    }

    struct termios term_settings;
    if (0 != tcgetattr(STDIN_FILENO, &term_settings)) {
        return 1;
    }
    
    struct termios nfsh_term_settings;
    memcpy(&nfsh_term_settings, &term_settings, sizeof term_settings);
    nfsh_term_settings.c_iflag &= ~(IXON | IXOFF | IXANY | INLCR) | ICRNL;
    nfsh_term_settings.c_lflag &= ~(ISIG | ICANON | ECHO | ECHOE | IEXTEN);
    nfsh_term_settings.c_cc[VMIN] = 1;
    nfsh_term_settings.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &nfsh_term_settings);

    ed_t ed;
    ed_init(&ed);

    printf("nephesh editing demo\n");
    printf("type 'exit' to quit\n");

    while (1) {
        const char * line = ed_readline(&ed);
        if (NULL == line) {
            break;
        }
        printf("Got line: %s\n", line);
        if (0 == strcmp(line, "exit")) {
            break;
        }
    }

    // Restore terminal settings.
    tcsetattr(STDIN_FILENO, TCSANOW, &term_settings);
    
    return 0;
}
