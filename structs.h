#include <stdbool.h>
#include <sys/queue.h>

struct env
{
        char * actual_path;
        char * previous_path;
        char * prompt_text;
        int return_val;
        bool file_run;
};
struct env env_;

struct name_command_tuple
{
        char* name;
        int (*command)(int argc, char* argv[]);
        SLIST_ENTRY(name_command_tuple) entries;
};
