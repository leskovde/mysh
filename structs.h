#include <stdbool.h>
#include <sys/queue.h>

struct env
{
        char* actual_path;
        char* previous_path;
        char* prompt_text;
        int return_val;
        bool file_run;
};
struct env env_;

typedef struct command_wrapper name_command_tuple;
struct command_wrapper
{
        char* name;
        int (*command)(int argc, char* argv[]);
        SLIST_ENTRY(command_wrapper) entries;
};
