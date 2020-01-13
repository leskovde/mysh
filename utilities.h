#include "parser_helpfile.h"
#include "structs.h"

int
get_cmdlen();

void
initialize();

int
quit();

void
free_resources();

char**
get_arguments(command_object*);

int
find_in_custom_list(char*);

int
find_and_execute(int argc, char** argv);

char*
get_env_path(void);

int
custom_cd(int, char**);

int
custom_exit();
