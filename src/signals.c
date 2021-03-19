#include <stdio.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "lib/signals.h"

/**
 * Redraws the prompt after getting interrupted when a command is running.
 */
void
interrupt_cmd(int signo)
{
        if (signo == SIGINT)
        {
                rl_on_new_line_with_prompt();
                rl_replace_line("", 0);
                rl_redisplay();
        }
}

/**
 * Redraws the prompt after getting interrupted in it by the user.
 */
void
interrupt_prompt(int signo)
{
        if (signo == SIGINT)
        {
                rl_crlf();
                rl_on_new_line();
                rl_replace_line("", 0);
                rl_redisplay();
        }
}
