#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <memory.h>
#include <sysexits.h>

#include "parser_helpfile.h"

command_object*
command_constructor()
{
#ifdef DEBUG
        printf("Creating a new command\n");
#endif
        struct command* cmd = (struct command*)(malloc(sizeof(struct command)));

        STAILQ_INIT(&cmd->args);

        cmd->arg_count = 0;

        return (cmd);
}

void
add_cmd_arg(command_object* cmd, char* arg)
{
#ifdef DEBUG
        printf("Adding an argument, arg: %s \n", arg);
#endif
        struct list_node* entry = (struct list_node*)(malloc(sizeof(struct list_node)));

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

static void
free_command(command_object* command)
{
#ifdef DEBUG
        printf("Freeing a command\n");
#endif

        struct list_node* entry = STAILQ_FIRST(&command->args);
        struct list_node* tmp;

        while (entry != nullptr)
        {
                tmp = STAILQ_NEXT(entry, entries);
                free(entry->item);
                free(entry);
                entry = tmp;
        }

        free(command);
}

command_sequence_object*
command_sequence_constructor()
{
#ifdef DEBUG
        printf("Creating a new sequence\n");
#endif
        struct sequence* seq = (struct sequence*)(malloc(sizeof(struct sequence)));

        STAILQ_INIT(&seq->commands);

        return (seq);
}

void
command_sequence_push_front(command_sequence_object* sequence, command_object* command)
{
#ifdef DEBUG
        printf("Inserting a command\n");
#endif
        struct command_entry* entry = (struct command_entry*)(malloc(sizeof(struct command_entry)));

        entry->command = command;

        STAILQ_INSERT_HEAD(&sequence->commands, entry, entries);
}

void
free_command_sequence(command_sequence_object* sequence)
{
#ifdef DEBUG
        printf("Freeing sequence\n");
#endif
        assert(sequence != NULL);

        struct command_entry* entry = STAILQ_FIRST(&sequence->commands);
        struct command_entry* tmp;

        while (entry != nullptr)
        {
                tmp = STAILQ_NEXT(entry, entries);
                free_command(entry->command);
                free(entry);
                entry = tmp;
        }

        free(sequence);
}