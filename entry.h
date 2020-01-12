#include <stdio.h>
#include <stdbool.h>

#define nullptr NULL

struct env
{
        char * actual_path;
        char * previous_path;
        char * prompt_text;
        int return_val;
        bool file_run;
};

struct env env_;

int
quit();

void
free_resources();

int
launcher(int, char **);
