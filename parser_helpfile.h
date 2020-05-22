#include <sys/queue.h>

#define nullptr NULL

STAILQ_HEAD(string_head, list_node);

typedef struct list_node literal_entry;
struct list_node
{
        char * item;
        STAILQ_ENTRY(list_node) entries;
};

/* Command section */

typedef struct command command_object;
struct command
{
        struct string_head args;
        int arg_count;
	int append_flag;
	char * source;
	char * target;
};

command_object *
command_constructor();

STAILQ_HEAD(command_head, command_list_entry);

typedef struct command_list_entry command_entry;
struct command_list_entry
{
        command_object * command;
        STAILQ_ENTRY(command_list_entry) entries;
};

void
add_cmd_arg(command_object*, char*);

void
set_cmd_source(command_object*, const char*);

void
set_cmd_target(command_object*, const char*, int);

/* Pipeline section */

typedef struct pipeline command_pipeline_object;
struct pipeline
{
	struct command_head commands;
};

STAILQ_HEAD(pipeline_head, pipeline_list_entry);

typedef struct pipeline_list_entry pipeline_entry;
struct pipeline_list_entry
{
	command_pipeline_object* pipeline;
	STAILQ_ENTRY(pipeline_list_entry) entries;
};

void
command_pipeline_push_front(command_pipeline_object*, command_object*);

command_pipeline_object*
command_pipeline_constructor();

/* Pipeline sequence section */

typedef struct sequence pipeline_sequence_object;
struct sequence
{
        struct pipeline_head pipelines;
};

void
run_pipeline_sequence(pipeline_sequence_object* sequence);

pipeline_sequence_object*
pipeline_sequence_constructor();

void
pipeline_sequence_push_front(pipeline_sequence_object*, command_pipeline_object*);

void
free_pipeline_sequence(pipeline_sequence_object*);
