#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <err.h>
#include <errno.h>
#include <sysexits.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/wait.h>
#include <limits.h>

#include "entry.h"
#include "parser_helpfile.h"
#include "parser.tab.h"
#include "lex.yy.h"

const int prompt_limit = 3 * PATH_MAX;

extern FILE* yyin;
extern int yyparse();

extern int yylex_destroy();
extern void yy_delete_buffer(YY_BUFFER_STATE b);
extern YY_BUFFER_STATE yy_scan_string(const char* f);

struct name_command_tuple
{
        char* name;
        int (*command)(int argc, char* argv[]);
        SLIST_ENTRY(name_command_tuple) entries;
};

SLIST_HEAD(slist_head, name_command_tuple) custom_command_list =
                SLIST_HEAD_INITIALIZER(custom_command_list);

int
custom_exit()
{
#ifdef DEBUG
        printf("Running Exit\n");
#endif
        free_resources();
        return (quit());
}

char*
get_env_path(void)
{
#ifdef DEBUG
        printf("Attempting malloc in get_env_path\n");
#endif
        char* buffer = malloc(PATH_MAX);

        if (getcwd(buffer, PATH_MAX) == NULL)
        {
                err(EX_SOFTWARE, "PATH_MAX limit reached");
        }

        return (buffer);
}

int
custom_cd(int argc, char* argv[])
{
#ifdef DEBUG
        printf("Running cd\n");
#endif
        char* target_path = nullptr;

        if (argc == 1)
        {
                target_path = getenv("HOME");
        }
        else if (argc == 2 && !(strcmp(argv[1], "-")))
        {
                fprintf(stderr, "%s\n", env_.previous_path);
                target_path = env_.previous_path;
        }
        else if (argc == 2)
        {
                target_path = argv[1];
        }
        else
        {
                fprintf(stderr, "cd: too many arguments\n");
                return (1);
        }

        if (chdir(target_path))
        {
                fprintf(stderr, "cd: %s: No such file or directory\n", target_path);
                return (1);
        }

        free(env_.previous_path);
        env_.previous_path = env_.actual_path;
        env_.actual_path = get_env_path();
        return (0);
}

int
find_in_custom_list(const char* name)
{
#ifdef DEBUG
        printf("Calling find on custom commands list\n");
#endif
        struct name_command_tuple* entry;
        SLIST_FOREACH(entry, &custom_command_list, entries)
        {
                if (!strcmp(entry->name, name))
                {
                        return (1);
                }
        }

        return (0);
}

int
find_and_execute(int argc, char* argv[])
{
#ifdef DEBUG
        printf("Calling execute on %s\n", argv[0]);
#endif
        struct name_command_tuple* entry;
        SLIST_FOREACH(entry, &custom_command_list, entries)
        {
                if (!strcmp(entry->name, argv[0]))
                {
                        return ((*entry->command)(argc, argv));
                }
        }

        err(EX_SOFTWARE, "No such command");
}

static char**
get_arguments(const command_object* cmd)
{
#ifdef DEBUG
        printf("Running get_arguments\n");
#endif
        char** argv;
        int i = 0;
        struct list_node* arg;
#ifdef DEBUG
        printf("Attempting malloc in get_arguments\n");
#endif
        argv = malloc(cmd->arg_count + 1);

        if (argv == nullptr)
        {
                errno = ENOMEM;
                return (nullptr);
        }

        STAILQ_FOREACH(arg, &cmd->args, entries)
        {
                argv[i++] = arg->item;
        }
        argv[i] = nullptr;

        return (argv);
}

void
forked_process(const command_object* cmd, char* const* argv)
{
#ifdef DEBUG
        printf("Forked a process\n");
#endif
        execvp(argv[0], argv);
        err(127, "%s", argv[0]);
}

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

void
free_resources()
{
#ifdef DEBUG
        printf("Freeing resources\n");
#endif
        free(env_.actual_path);
        free(env_.previous_path);

        struct name_command_tuple* entry;

        while (!SLIST_EMPTY(&custom_command_list))
        {
                entry = SLIST_FIRST(&custom_command_list);
                SLIST_REMOVE_HEAD(&custom_command_list, entries);
                free(entry);
        }

        (void)yylex_destroy();
}

int
quit()
{
        exit(env_.return_val);
}

void
initialize()
{
        struct name_command_tuple* cd = (struct name_command_tuple*)(malloc(sizeof(struct name_command_tuple)));

        cd->name = "cd";
        cd->command = custom_cd;
        SLIST_INSERT_HEAD(&custom_command_list, cd, entries);

        struct name_command_tuple* exit = (struct name_command_tuple*)(malloc(sizeof(struct name_command_tuple))
        );

        exit->name = "exit";
        exit->command = custom_exit;
        SLIST_INSERT_HEAD(&custom_command_list, exit, entries);

        env_.prompt_text = "mysh";
        env_.return_val = 0;
        env_.file_run = false;
        env_.actual_path = get_env_path();
        env_.previous_path = get_env_path();
}

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

int
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

static void
interrupt_cmd(int signo)
{
        if (signo == SIGINT)
        {
                rl_on_new_line_with_prompt();
                rl_replace_line("", 0);
                rl_redisplay();
        }
}

static void
interrupt_prompt(int signo)
{
        if (signo == SIGINT)
        {
                rl_crlf();
                rl_on_new_line();
                rl_replace_line("", 0);
                rl_redisplay();
        }
}

int
get_cmdlen()
{
        return PATH_MAX + prompt_limit + strlen(env_.prompt_text);
}

void
await_action()
{
        char* command;

        struct sigaction prompt_sig = {{0}};
        struct sigaction cmd_sig = {{0}};

        int buf_size = get_cmdlen();
#ifdef DEBUG
        printf("Attempting malloc in await_action (%d bytes)\n", buf_size);
#endif
        char* in = malloc(buf_size);

        cmd_sig.sa_handler = interrupt_cmd;
        cmd_sig.sa_flags = SA_RESTART;

        prompt_sig.sa_handler = interrupt_prompt;
        prompt_sig.sa_flags = SA_RESTART;

        int t = sigaction(SIGINT, &prompt_sig, NULL);
        if (t == -1)
        {
                free(in);
                err(EX_OSERR, NULL);
        }

        sprintf(in, "%s:%s ", env_.prompt_text, env_.actual_path);

        while ((command = readline(in)) != NULL)
        {
                t = sigaction(SIGINT, &cmd_sig, NULL);
                if (t == -1)
                {
                        free(in);
                        err(EX_OSERR, NULL);
                }


                if (strcmp(command, ""))
                {
                        add_history(command);
                }

                (void)execute(command);
                free(command);

                t = sigaction(SIGINT, &prompt_sig, NULL);
                if (t == -1)
                {
                        free(in);
                        err(EX_OSERR, NULL);
                }
#ifdef DEBUG
                printf("Printing prompt\n");
#endif
                sprintf(in, "%s:%s ", env_.prompt_text, env_.actual_path);
        }

        free(in);
        free_resources();
        quit();
}

int
launcher(int argc, char* argv[])
{
        initialize();
        int opt;

        while ((opt = getopt(argc, argv, "c:")) != -1)
        {
                if (opt == 'c')
                {
                        execute(optarg);
                        free_resources();
                        quit();
                }
                err(1, "Unknown option: %c", opt);
        }

        if (argc == 2)
        {
                execute_file(argv[1]);
        }

        await_action();

        return (0);
}
