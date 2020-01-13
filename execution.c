#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <sys/wait.h>

#include "utilities.h"
#include "lex.yy.h"

extern FILE* yyin;
extern int yyparse();
extern void yy_delete_buffer(YY_BUFFER_STATE b);
extern YY_BUFFER_STATE yy_scan_string(const char* f);

/**
 * Parses a string and executes the command represented by the string.
 *
 * @param command String representing the name of the command to be launched.
 *
 * @return The return value of the executed command.
 */
int
execute(char* command)
{
#ifdef DEBUG
        printf("Attempting malloc in execute\n");
#endif
        char* buf = malloc(strlen(command) + 2);

        sprintf(buf, "%s\n", command);

#ifdef DEBUG
        printf("Parsing in execute\n");
#endif

        YY_BUFFER_STATE bs = yy_scan_string(buf);
        yyparse();
        yy_delete_buffer(bs);

#ifdef DEBUG
        printf("Parsing done in execute\n");
#endif

        free(buf);

        return (env_.return_val);
}

/**
 * Parses a file and executes its commands.
 *
 * @param in Name of the input file.
 */
void
execute_file(char* in)
{
        env_.file_run = true;

        FILE* f = fopen(in, "r");

        yyin = f;
        yyparse();

        fclose(f);

        free_resources();

        quit();
}

/* Runtime of the child process.
 *
 * @param cmd Command to be executed in the child process.
 *
 * @param argv String array of the commands name and arguments.
 *
 * @return Return value is distributed via err.
 */
void
forked_process(command_object* cmd, char** argv)
{
#ifdef DEBUG
        printf("Forked a process\n");
#endif
        execvp(argv[0], argv);
        err(127, "%s", argv[0]);
}

/**
 * Forks a new process in which a command will be executed.
 *
 * @param Command to be executed in the child process.
 *
 * @return The PID of the child process.
 */
int
run_command(command_object* cmd)
{
#ifdef DEBUG
        printf("Running a command\n");
#endif
        char** argv = get_arguments(cmd);

        if (argv == nullptr)
        {
                errno = ENOMEM;
                return (-1);
        }

        if (find_in_custom_list(argv[0]))
        {
                env_.return_val = find_and_execute(cmd->arg_count, argv);
                free(argv);
                return (0);
        }

        int child_pid = fork();

        if (child_pid == -1)
        {
                fprintf(stderr, "Fork Error");
                return (-1);
        }

        if (child_pid == 0)
        {
                forked_process(cmd, argv);
        }

        free(argv);
        return (child_pid);
}

/**
 * Runs a sequence of commands separated by semicolon.
 *
 * @param sequence A list of commands to be executed.
 */
void
run_command_sequence(command_sequence_object* sequence)
{
#ifdef DEBUG
        printf("Running sequence\n");
#endif
        int lock;
        int return_val;
        int pid;

        struct command_entry* command;
        STAILQ_FOREACH(command, &sequence->commands, entries)
        {
                pid = run_command(command->command);

                if (pid > 0)
                {
                        waitpid(pid, &lock, 0);
                        if (WIFEXITED(lock))
                        {
                                return_val = WEXITSTATUS(lock);
                        }
                        else if (WIFSIGNALED(lock))
                        {
                                fprintf(stderr, "Killed by signal %d\n", WTERMSIG(lock));
                                return_val = 128 + WTERMSIG(lock);
                        }
                        else if (WIFSTOPPED(lock))
                        {
                                fprintf(stderr, "Stopped by signal %d\n", WSTOPSIG(lock));
                                return_val = 128 + WSTOPSIG(lock);
                        }
                        else
                        {
                                fprintf(stderr, "Unkonwn exit status");
                                return_val = 126;
                        }
                }

                while (wait(NULL) != -1);

                env_.return_val = return_val;
        }
}
