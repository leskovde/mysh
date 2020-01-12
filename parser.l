%option nodefault noinput nounput noyywrap yylineno

%{
        #include "parser_helpfile.h"
        #include "parser.tab.h"
        #include "entry.h"
%}

tokens          [-$)(/\_.a-zA-Z0-9]+
redirections    [<>|]
%%
{redirections}  { return (REDIRECTION); }
{tokens}        { yylval.str = yytext; return (TOKEN); }
;               { return (SEMICOLON); }
\n              { return (EOL); }
#[^\n]*         { }
.               { }
%%

int
main (int argc, char * argv[])
{
        launcher(argc, argv);

        // unreachable

        return (0);
}
