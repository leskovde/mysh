%{
        #include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <assert.h>
	#include <fcntl.h>
	#include <memory.h>
	#include <sysexits.h>
	#include <err.h>
        #include <string.h>
	
	#include "structs.h"
        #include "entry.h"
        #include "parser_helpfile.h"

        extern int yylex();
        extern int yylineno;
        extern int yyerror(const char * format);
%}

%{
	char delimiters[] = " \t\r\n\v\f";
%}

%union{
        char * str;
        command_object * command;
        pipeline_sequence_object * sequence;
	command_pipeline_object * pipeline;
}

%token<str> TOKEN
%token SEMICOLON EOL PIPE REDIRECTION_LEFT REDIRECTION_RIGHT APPEND

%type<command> literal command
%type<sequence> seq
%type<pipeline> pipeline

%precedence REDIRECTION_LEFT REDIRECTION_RIGHT APPEND

%right TOKEN

%error-verbose

%%
        cmdline
        :
        | cmdline EOL
        | cmdline seq EOL { run_pipeline_sequence($2); free_pipeline_sequence($2); }
        ;
	
	pipeline
	: command { $$ = command_pipeline_constructor(); 
		command_pipeline_push_front($$, $1); }

	| command PIPE pipeline { $$ = $3;
		command_pipeline_push_front($$, $1); }
	;


        seq
        : pipeline { $$ = pipeline_sequence_constructor();
                pipeline_sequence_push_front($$, $1); }

        | pipeline SEMICOLON { $$ = pipeline_sequence_constructor();
                pipeline_sequence_push_front($$, $1); }

        | pipeline SEMICOLON seq  { $$ = $3; pipeline_sequence_push_front($$, $1);}
        ;

        command
	: literal
	| REDIRECTION_LEFT TOKEN command { $$ = $3; strtok($2, delimiters);
		set_cmd_source($$, $2); }

	| REDIRECTION_RIGHT TOKEN command { $$ = $3; strtok($2, delimiters);
		set_cmd_target($$, $2, 0); }

	| APPEND TOKEN command { $$ = $3; strtok($2, delimiters);
		set_cmd_target($$, $2, 1); }
	;

        literal
        : TOKEN { $$ = command_constructor(); add_cmd_arg($$, $1);}
        | literal TOKEN { add_cmd_arg($$, $2); }
	| literal REDIRECTION_LEFT TOKEN { strtok($3, delimiters);
		set_cmd_source($$, $3); }

        | literal REDIRECTION_RIGHT TOKEN { strtok($3, delimiters);
		set_cmd_target($$, $3, 0); }

        | literal APPEND TOKEN { strtok($3, delimiters);
		set_cmd_target($$, $3, 1); }

	| literal REDIRECTION_LEFT TOKEN TOKEN { strtok($3, delimiters);
                set_cmd_source($$, $3); add_cmd_arg($$, $4); }

        | literal REDIRECTION_RIGHT TOKEN TOKEN { strtok($3, delimiters);
                set_cmd_target($$, $3, 0); add_cmd_arg($$, $4); }

        | literal APPEND TOKEN TOKEN { strtok($3, delimiters);
                set_cmd_target($$, $3, 1); add_cmd_arg($$, $4); }	
	;

%%

int
main (int argc, char * argv[])
{
        launcher(argc, argv);

        // unreachable

        return (0);
}

int
yyerror(const char * s)
{
    if (env_.file_run)
    {
        fprintf(stderr, "error: %d: %s\n", yylineno, s);
    }
    else
    {
        fprintf(stderr, "error: %s\n", s);
    }

    env_.return_val = 2;
    return (0);
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <memory.h>
#include <sysexits.h>
#include <err.h>

/**
 * Allocates a new command object.
 */
command_object*
command_constructor()
{
#ifdef DEBUG
        printf("Creating a new command\n");
#endif
        command_object* cmd = malloc(sizeof(command_object));
	if (cmd == nullptr)
	{
		err(EX_OSERR, nullptr);
	}

        STAILQ_INIT(&cmd->args);

        cmd->arg_count = 0;
	cmd->append_flag = 0;
	cmd->source = nullptr;
	cmd->target = nullptr;

        return (cmd);
}
/**
 * Adds an argument to the commands list of arguments.
 *
 * @param cmd Command, to which an argument will be added.
 *
 * @param arg Name of the argument to be added.
 */
void
add_cmd_arg(command_object* cmd, char* arg)
{
#ifdef DEBUG
        printf("Adding an argument, arg: %s \n", arg);
#endif
        literal_entry* entry = malloc(sizeof(literal_entry));
	if (entry == nullptr)
	{
		err(EX_OSERR, nullptr);
	}

        entry->item = strdup(arg);

        if (STAILQ_EMPTY(&cmd->args))
        {
                STAILQ_INSERT_HEAD(&cmd->args, entry, entries);
        }
        else
        {
                STAILQ_INSERT_TAIL(&cmd->args, entry, entries);
        }

        cmd->arg_count++;
}

/**
 * Deallocates a command object.
 *
 * @param command Command object to be deallocated.
 */
void
free_command(command_object* command)
{
#ifdef DEBUG
        printf("Freeing a command\n");
#endif

        literal_entry* entry = STAILQ_FIRST(&command->args);
        literal_entry* tmp;

        while (entry != nullptr)
        {
                tmp = STAILQ_NEXT(entry, entries);
                
#ifdef DEBUG
		printf("Freeing an argument\n");
#endif
		free(entry->item);
                free(entry);
                
		entry = tmp;
        }

#ifdef DEBUG
	printf("Freeing source\n");
#endif
	free(command->source);

#ifdef DEBUG
	printf("Freeing target\n");
#endif
	free(command->target);

        free(command);
}

/**
 * Stores the path to the command's input file.
 *
 * @param cmd The command whose input file should be set.
 *
 * @param source The path to the input file.
 */
void
set_cmd_source(command_object* cmd, const char* source)
{
#ifdef DEBUG
        printf("Setting source: %s\n", source);
#endif

	cmd->source = strdup(source);

	if (cmd->source == nullptr)
	{
		err(EX_OSERR, nullptr);
	}
}

/**
 * Stores the path to the command's output file.
 *
 * @param cmd The command whose output file should be set.
 *
 * @param source The path to the output file.
 *
 * @param append_flag An integer value saying whether the file should be \n
 * opened in the append mode or rewritten. Value of 0 equals 'Rewrite', \n
 * any other value equals 'Append'.
 */
void
set_cmd_target(command_object* cmd, const char* target, int append_flag)
{
#ifdef DEBUG
        printf("Setting target: %s\n", target);
#endif

	cmd->target = strdup(target);

	cmd->append_flag = append_flag;

	if (cmd->target == nullptr)
	{
		err(EX_OSERR, nullptr);
	}

#ifdef DEBUG
        printf("Target set: %s\n", cmd->target);
#endif

}

/**
 * Allocates a new pipeline of commands.
 */
command_pipeline_object*
command_pipeline_constructor()
{
#ifdef DEBUG
	printf("Creating a new pipeline\n");
#endif
	command_pipeline_object* pipe = malloc(sizeof(command_pipeline_object));
	if (pipe == nullptr)
	{
		err(EX_OSERR, nullptr);
	}

	STAILQ_INIT(&pipe->commands);
	
	return (pipe);
}

/**
 * Adds a command to a command pipeline.
 *
 * @param pipeline Command pipeline to be modified.
 *
 * @command Command object to be inserted into the pipeline.
 */
void
command_pipeline_push_front(command_pipeline_object* pipeline,
		command_object* cmd)
{
#ifdef DEBUG
        printf("Inserting a command\n");
#endif
        command_entry* entry = malloc(sizeof(command_entry));
	if (entry == nullptr)
	{
		err(EX_OSERR, nullptr);
	}

        entry->command = cmd;

        STAILQ_INSERT_HEAD(&pipeline->commands, entry, entries);
}

/**
 * Allocates a new sequence of pipelines.
 */
pipeline_sequence_object*
pipeline_sequence_constructor()
{
#ifdef DEBUG
        printf("Creating a new sequence\n");
#endif
        pipeline_sequence_object* seq = malloc(sizeof(pipeline_sequence_object));
	if (seq == nullptr)
	{
		err(EX_OSERR, nullptr);
	}

        STAILQ_INIT(&seq->pipelines);

        return (seq);
}

/**
 * Adds a pipeline to a pipeline sequence.
 *
 * @param sequence Pipeline sequence to be modified.
 *
 * @param pipe Command pipeline to be inserted into the sequence.
 */
void
pipeline_sequence_push_front(pipeline_sequence_object* sequence,
		command_pipeline_object* pipe)
{
#ifdef DEBUG
        printf("Inserting a pipeline\n");
#endif
        pipeline_entry* entry = malloc(sizeof(pipeline_entry));
	if (entry == nullptr)
	{
		err(EX_OSERR, nullptr);
	}

        entry->pipeline = pipe;

        STAILQ_INSERT_HEAD(&sequence->pipelines, entry, entries);
}

/**
 * Deallocates a pipeline sequence.
 *
 * @param sequence Pipeline sequence to be deallocated.
 */
void
free_pipeline_sequence(pipeline_sequence_object* sequence)
{
#ifdef DEBUG
        printf("Freeing sequence\n");
#endif
        assert(sequence != nullptr);

        pipeline_entry* pipe_entry = STAILQ_FIRST(&sequence->pipelines);
        pipeline_entry* pipe_temp;
	
	// Deallocate individual pipeline entries.
        while (pipe_entry != nullptr)
        {
                pipe_temp = STAILQ_NEXT(pipe_entry, entries);
                
		command_entry* cmd_entry = STAILQ_FIRST(&pipe_entry->
				pipeline->commands);

		command_entry* cmd_temp;

		// Deallocate individual command entries.
		while (cmd_entry != nullptr)
		{
			cmd_temp = STAILQ_NEXT(cmd_entry, entries);
			
			free_command(cmd_entry->command);
			free(cmd_entry);
			
			cmd_entry = cmd_temp;
		}

		free(pipe_entry->pipeline);

                free(pipe_entry);
                pipe_entry = pipe_temp;
        }

        free(sequence);
}
