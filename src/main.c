#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <locale.h>
#include <curses.h>
#include <term.h>
#include <utlist.h>
#include "editor.h"
#include "scanner.h"
#include "parser.h"
#include <wait.h>

extern char **environ;

static int nfsh_execute_pipeline(command_t * commands);

int main(int argc, char * argv[])
{
    /*const char * locale = */setlocale(LC_ALL, "");

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

    ed_t * ed = ed_new(STDIN_FILENO, STDOUT_FILENO);

    fprintf(stdout, "Type 'exit' to quit.\n");
    fflush(stdout);

    while (1) {
        const char * line = ed_readline(ed);
        if (0 == strlen(line)) {
            continue;
        } else if (0 == strcmp(line, "exit")) {
            break;
        } else {
            scanner_t * scanner = scanner_new(line);
            if (NULL == scanner) {
                goto error0;
            }
            token_t * tokens = scanner_scan(scanner);
            parser_t * parser = parser_new(tokens);
            if (NULL == parser) {
                goto error1;
            }
            command_t * commands = parser_parse(parser);
            if (NULL == commands) {
                fprintf(stdout, "Parse error: %s\n", parser_get_error(parser));
                fflush(stdout);
                goto error2;
            }
            tcsetattr(STDIN_FILENO, TCSANOW, &term_settings);
            if (nfsh_execute_pipeline(commands) < 0) {
                fprintf(stdout, "Unable to execute one or more commands.\n");
                fflush(stdout);
            }
            tcsetattr(STDIN_FILENO, TCSANOW, &nfsh_term_settings);
        error2:
                parser_delete(parser);
        error1:
                scanner_delete(scanner);
                // Cleanup tokens.
                token_t * tt1, * tt2;
                DL_FOREACH_SAFE(tokens, tt1, tt2) {
                    DL_DELETE(tokens, tt1);
                    free(tt1);
                }
        error0:
                continue;
        }
    }

    ed_delete(ed);

    // Restore terminal settings.
    tcsetattr(STDIN_FILENO, TCSANOW, &term_settings);
    
    return 0;
}

static int nfsh_execute_pipeline(command_t * commands)
{
    command_t * command = NULL;
    command_t * command_prev = NULL;
    DL_FOREACH(commands, command) {
        command_prev = command->prev;
        // Create legitimate pipes.
        for (unsigned int i = 0; i < command->pipec; ++i) {
            pipe(command->pipes_legit[i]);
        }
        if (0 == fork()) {
            // Input pipes.
            if (NULL != command_prev) {
                for (unsigned int i = 0; i < command_prev->pipec; ++i) {
                    close(command_prev->pipes_legit[i][1]);
                    dup2(command_prev->pipes_legit[i][0], command_prev->pipes[i][1]);
                }
            }
            // Output pipes.
            for (unsigned int i = 0; i < command->pipec; ++i) {
                close(command->pipes_legit[i][0]);
                dup2(command->pipes_legit[i][1], command->pipes[i][0]);
            }
            execvpe(command->argv[0], command->argv, environ);
            exit(1);
        }
        // Cleanup previous pipes.
        if (NULL != command_prev) {
            for (unsigned int i = 0; i < command_prev->pipec; ++i) {
                close(command_prev->pipes_legit[i][0]);
                close(command_prev->pipes_legit[i][1]);
            }
        }
    }
    // Cleanup previous pipes (final run).
    if (NULL != command_prev) {
        for (unsigned int i = 0; i < command_prev->pipec; ++i) {
            close(command_prev->pipes_legit[i][0]);
            close(command_prev->pipes_legit[i][1]);
        }
    }
    // Wait for children.
    while (wait(NULL) > 0);
    return 0;
}
