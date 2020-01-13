#include <sys/queue.h>

#define nullptr NULL

STAILQ_HEAD(string_head, list_node);

struct list_node
{
        char * item;
        STAILQ_ENTRY(list_node) entries;
};

typedef struct command command_object;
struct command
{
        struct string_head args;
        int arg_count;
};

command_object *
command_constructor();

STAILQ_HEAD(command_head, command_entry);

struct command_entry
{
        command_object * command;
        STAILQ_ENTRY(command_entry) entries;
};

void
add_cmd_arg(command_object*, char*);

typedef struct sequence command_sequence_object;
struct sequence
{
        struct command_head commands;
};

void
run_command_sequence(command_sequence_object * sequence);

command_sequence_object *
command_sequence_constructor();

void
command_sequence_push_front(command_sequence_object *, command_object *);

void
free_command_sequence(command_sequence_object *);
