#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <err.h>
#include <errno.h>
#include <sysexits.h>
#include <unistd.h>

#include "lib/utilities.h"

extern int yylex_destroy();

const int prompt_limit = 3 * PATH_MAX;

SLIST_HEAD(slist_head, command_wrapper) custom_command_list =
                SLIST_HEAD_INITIALIZER(custom_command_list);

/**
 * Defines the environment variables and custom commands (redundant).
 */
void
initialize()
{
        name_command_tuple* cd = malloc(sizeof(name_command_tuple));
	if (cd == nullptr)
	{
		err(EX_OSERR, nullptr);
	}

        cd->name = "cd";
        cd->command = custom_cd;
        SLIST_INSERT_HEAD(&custom_command_list, cd, entries);

        name_command_tuple* exit = malloc(sizeof(name_command_tuple));
	if (exit == nullptr)
	{
		err(EX_OSERR, nullptr);
	}

        exit->name = "exit";
        exit->command = custom_exit;
        SLIST_INSERT_HEAD(&custom_command_list, exit, entries);

        env_.prompt_text = "mysh";
        env_.return_val = 0;
        env_.file_run = false;
        env_.actual_path = get_env_path();
        env_.previous_path = get_env_path();
}

/**
 * Calculates the length of the command prompt.
 *
 * @return A number of bytes to be allocated for the prompt.
 */
int
get_cmdlen()
{
        return (PATH_MAX + prompt_limit + strlen(env_.prompt_text));
}

/**
 * Terminates the shell while distributing its return value.
 */
int
quit()
{
        exit(env_.return_val);
}

/**
 * Deallocates environmental variables and custom commands.
 */
void
free_resources()
{
#ifdef DEBUG
        printf("Freeing resources\n");
#endif
        free(env_.actual_path);
        free(env_.previous_path);

        name_command_tuple* entry;

        while (!SLIST_EMPTY(&custom_command_list))
        {
                entry = SLIST_FIRST(&custom_command_list);
                SLIST_REMOVE_HEAD(&custom_command_list, entries);
                free(entry);
        }

        (void)yylex_destroy();
}

/**
 * Traverses a list of commands arguments and returns its copy.
 *
 * @param cmd Command to be traversed.
 *
 * @return A list of commands parameteres.
 */
char**
get_arguments(command_object* cmd)
{
#ifdef DEBUG
        printf("Running get_arguments\n");
#endif
        char** argv;
        int i = 0;
        literal_entry* arg;
#ifdef DEBUG
        printf("Attempting malloc in get_arguments\n");
#endif
        argv = malloc(sizeof(char*) * (cmd->arg_count + 1));
        if (argv == nullptr)
        {
                errno = ENOMEM;
                return (nullptr);
        }

        STAILQ_FOREACH(arg, &cmd->args, entries)
        {
                argv[i++] = arg->item;
        }
        argv[i] = nullptr;

        return (argv);
}

/**
 * Checks the current working directory.
 *
 * @return The full path to the current directory.
 */
char*
get_env_path()
{
#ifdef DEBUG
        printf("Attempting malloc in get_env_path\n");
#endif
        char* buffer = malloc(PATH_MAX);
	if (buffer == nullptr)
	{
		err(EX_OSERR, nullptr);
	}

        if (getcwd(buffer, PATH_MAX) == nullptr)
        {
                err(EX_SOFTWARE, "PATH_MAX limit reached");
        }

        return (buffer);
}

/**
 * Imitation of the 'cd' command.
 *
 * @param argc Argument count.
 *
 * @param argv A string array of arguments provided by the prompt.
 *
 * @return The return value of 'cd' given the same parameters.
 */
int
custom_cd(int argc, char* argv[])
{
#ifdef DEBUG
        printf("Running cd\n");
#endif
        char* target_path = nullptr;

        if (argc == 1)
        {
                target_path = getenv("HOME");
		if (target_path == nullptr)
		{
			err(EX_OSERR, nullptr);
		}
        }
        else if (argc == 2 && !(strcmp(argv[1], "-")))
        {
                fprintf(stderr, "%s\n", env_.previous_path);
                target_path = env_.previous_path;
        }
        else if (argc == 2)
        {
                target_path = argv[1];
        }
        else
        {
                fprintf(stderr, "cd: too many arguments\n");
                return (1);
        }

        if (chdir(target_path))
        {
                fprintf(stderr, "cd: %s: No such file or directory\n", target_path);
                return (1);
        }

        free(env_.previous_path);
        env_.previous_path = env_.actual_path;
        env_.actual_path = get_env_path();
        return (0);
}

/**
 * User utility used to terminate the shell.
 */
int
custom_exit()
{
#ifdef DEBUG
        printf("Running Exit\n");
#endif
        free_resources();
        return (quit());
}

/**
 * Traverses through custom commands and finds the one with a matching name.
 * (redundant)
 *
 * @param name The name of the command;
 *
 * @return 1 if successful, 0 otherwise
 */
int
find_in_custom_list(char* name)
{
#ifdef DEBUG
        printf("Calling find on custom commands list\n");
#endif
        name_command_tuple* entry;
        SLIST_FOREACH(entry, &custom_command_list, entries)
        {
                if (!strcmp(entry->name, name))
                {
                        return (1);
                }
        }

        return (0);
}

/**
 * Executes a custom command with provided parameters based on its name.
 *
 * @param argc Argument count.
 *
 * @param argv A string array of the commands arguments.
 *
 * @return The return value of the executed command.
 */
int
find_and_execute(int argc, char* argv[])
{
#ifdef DEBUG
        printf("Calling execute on %s\n", argv[0]);
#endif
name_command_tuple* entry;
        SLIST_FOREACH(entry, &custom_command_list, entries)
        {
                if (!strcmp(entry->name, argv[0]))
                {
                        return ((*entry->command)(argc, argv));
                }
        }

        err(EX_SOFTWARE, "No such command");
}
