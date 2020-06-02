#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <sysexits.h>
#include <limits.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "entry.h"
#include "signals.h"
#include "utilities.h"
#include "execution.h"
#include "parser.tab.h"
#include "lex.yy.h"

/**
 * Enters the interactive mode of the shell.
 */
static void
await_action()
{
        char* command;

        struct sigaction prompt_sig = {{0}};
        struct sigaction cmd_sig = {{0}};

        int buf_size = get_cmdlen();
#ifdef DEBUG
        printf("Attempting malloc in await_action (%d bytes)\n", buf_size);
#endif
        char* in = malloc(buf_size);
	assert(in != nullptr);

        cmd_sig.sa_handler = interrupt_cmd;
        cmd_sig.sa_flags = SA_RESTART;

        prompt_sig.sa_handler = interrupt_prompt;
        prompt_sig.sa_flags = SA_RESTART;

        int t = sigaction(SIGINT, &prompt_sig, nullptr);
        if (t == -1)
        {
                free(in);
                err(EX_OSERR, nullptr);
        }

	int prompt_length = strlen(env_.prompt_text) + PATH_MAX + 2;

        int prompt_written = snprintf(in, prompt_length, "%s:%s ",
			env_.prompt_text, env_.actual_path);
	
	if (prompt_written < 0 || prompt_written >= prompt_length)
	{
		free(in);
		err(EX_OSERR, nullptr);
	}

        while ((command = readline(in)) != nullptr)
        {
                t = sigaction(SIGINT, &cmd_sig, nullptr);
                if (t == -1)
                {
                        free(in);
                        err(EX_OSERR, nullptr);
                }

                if (strcmp(command, ""))
                {
                        add_history(command);
                }

                (void)execute(command);
                free(command);

                t = sigaction(SIGINT, &prompt_sig, nullptr);
                if (t == -1)
                {
                        free(in);
                        err(EX_OSERR, nullptr);
                }
#ifdef DEBUG
                printf("Printing prompt\n");
#endif
                prompt_written = snprintf(in, prompt_length, "%s:%s ",
				env_.prompt_text, env_.actual_path);

		if (prompt_written < 0 || prompt_written >= prompt_length)
        	{
                	free(in);
                	err(EX_OSERR, nullptr);
        	}

        }

        free(in);
        free_resources();
        quit();
}

/**
 * Entrypoint of the application.
 * Decides whether to run in interactive or file mode.
 *
 * @param argc The number of arguments provided on the command line
 * when launching the shell.
 *
 * @param argv An array of argc strings, each of which represents
 * a white space separated entry from the command line.
 */
int
launcher(int argc, char* argv[])
{
	initialize();
        int opt;

        while ((opt = getopt(argc, argv, "c:")) != -1)
        {
                if (opt == 'c')
                {
                        execute(optarg);
                        free_resources();
                        quit();
                }
                err(1, "Unknown option: %c", opt);
        }

        if (argc == 2)
        {
                execute_file(argv[1]);
        }

        await_action();

        // unreachable

        return (0);
}
