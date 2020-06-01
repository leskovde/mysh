#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sysexits.h>

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
	assert (buf != nullptr);

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
 * @param fd_source The file descriptor of command's source file.
 *
 * @param fd_target The file descriptor of command's target file.
 *
 * @param cmd Command to be executed in the child process.
 *
 * @param argv String array of the commands name and arguments.
 *
 * @return Return value is distributed via err.
 */
void
forked_process(int fd_source, int fd_target, command_object* cmd, char** argv)
{

#ifdef DEBUG
        printf("Forked a process\n");
#endif
	if (cmd->source != nullptr)
	{
#ifdef DEBUG
        	printf("Opening input descriptor\n");
#endif
		fd_source = open(cmd->source, O_RDONLY);

		if (fd_source == -1)
		{
			err(EX_IOERR, nullptr);
		}
	}
	
	dup2(fd_source, 0);

#ifdef DEBUG
        printf("Done opening input descriptor\n");
#endif

	if (cmd->target != nullptr)
	{

#ifdef DEBUG
        	printf("Opening output descriptor\n");
#endif

		if (cmd->append_flag == 0)
		{
			// Rewrite the target file with full permission
			fd_target = creat(cmd->target, S_IRWXO | S_IRWXU);
		}
		else
		{
			// Append (or create if needed) the target file
			// with full permission
			fd_target = open(cmd->target, O_WRONLY |
					O_APPEND | O_CREAT, S_IRWXO | S_IRWXU);
		}

		if (fd_target == -1)
		{
			err(EX_IOERR, nullptr);
		}
	}

	dup2(fd_target, 1);

#ifdef DEBUG
        printf("Done opening output descriptor\n");
#endif

        execvp(argv[0], argv);
        err(127, "%s", argv[0]);
}

/**
 * Forks a new process in which a command will be executed.
 *
 * @param fd_source The file descriptor of command's source file.
 *
 * @param fd_target The file descriptor of command's target file.
 *
 * @param Command to be executed in the child process.
 *
 * @return The PID of the child process.
 */
int
run_command(int fd_source, int fd_target, command_object* cmd, int* return_val)
{
#ifdef DEBUG
        printf("Running a command (%d): ", cmd->arg_count);
#endif
        char** argv = get_arguments(cmd);

#ifdef DEBUG
	printf("%s\n", argv[0]);
#endif

        if (argv == nullptr || return_val == nullptr)
        {
                errno = ENOMEM;
                return (-1);
        }

        if (find_in_custom_list(argv[0]))
        {
                *return_val = find_and_execute(cmd->arg_count, argv);
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
                forked_process(fd_source, fd_target, cmd, argv);
        }

        free(argv);
        return (child_pid);
}

/**
 * Runs a sequence of pipelines separated by semicolon.
 *
 * @param sequence A list of pipelines to be executed.
 */
void
run_pipeline_sequence(pipeline_sequence_object* sequence)
{
#ifdef DEBUG
	printf("Running sequence\n");
#endif

	pipeline_entry* pipeline;
	
	// Runs individual pipelines
	STAILQ_FOREACH(pipeline, &sequence->pipelines, entries)
	{

#ifdef DEBUG
		printf("Running pipeline\n");
#endif
		int lock;
		int return_val;
		int pid;

		int fd[2] = { STDIN_FILENO, STDOUT_FILENO };

		command_entry* command;

		// Runs individual commands
		STAILQ_FOREACH(command, &pipeline->pipeline->commands, entries)
		{
			int fd_source, fd_target;
			fd_source = fd[0];

			if (STAILQ_NEXT(command, entries) == nullptr)
			{
				fd_target = STDOUT_FILENO;
			}
			else
			{
				if (find_in_custom_list(command->command->
							args.stqh_first->item))
				{
					continue;
				}

				if (pipe(fd) == -1)
				{
					err(EX_OSERR, "Pipe Construction Fail");
				}

				fd_target = fd[1];
			}

			pid = run_command(fd_source, fd_target,
					command->command, &return_val);

			if (fd_source != STDIN_FILENO)
			{
				close(fd_source);
			}

			if (fd_target != STDOUT_FILENO)
			{
				close(fd_target);
			}
		}

		if (pid > 0)
		{
			waitpid(pid, &lock, 0);
			if (WIFEXITED(lock))
			{
				return_val = WEXITSTATUS(lock);
			}
			else if (WIFSIGNALED(lock))
			{
				fprintf(stderr, "Killed by signal %d\n", 
						WTERMSIG(lock));

				return_val = 128 + WTERMSIG(lock);
			}
			else if (WIFSTOPPED(lock))
			{
				fprintf(stderr, "Stopped by signal %d\n",
						WSTOPSIG(lock));

				return_val = 128 + WSTOPSIG(lock);
			}
			else
			{
				fprintf(stderr, "Unknown Exit Status");
				return_val = 126;
			}
		}

		while (wait(NULL) != -1);

		env_.return_val = return_val;

	}
}
