%{
        #include <stdio.h>
        #include "entry.h"
        #include "parser_helpfile.h"

        extern int yylex();
        extern int yylineno;
        extern int yyerror(const char * format);
%}

%union{
        char * str;
        command_object * command;
        command_sequence_object * sequence;
}

%token<str> TOKEN
%token SEMICOLON EOL REDIRECTION

%type<command> literal cmd
%type<sequence> seq

%error-verbose

%%
        cmdline
        :
        | cmdline EOL
        | cmdline seq EOL { run_command_sequence($2); free_command_sequence($2); }
        ;

        seq
        : cmd { $$ = command_sequence_constructor();
                command_sequence_push_front($$, $1); }
        | cmd SEMICOLON { $$ = command_sequence_constructor();
                command_sequence_push_front($$, $1); }
        | cmd SEMICOLON seq  { $$ = $3; command_sequence_push_front($3, $1);}
        ;

        cmd
        : literal
        ;

        literal
        : TOKEN { $$ = command_constructor(); add_cmd_arg($$, $1);}
        | literal TOKEN { add_cmd_arg($$, $2); }
        ;
%%

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
