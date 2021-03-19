%option nodefault noinput nounput noyywrap yylineno

%{
        #include "lib/parser_helpfile.h"
        #include "parser.tab.h"
%}

tokens          [-$)(/\_.a-zA-Z0-9]+

%%
\<  		{ return (REDIRECTION_LEFT); }
>		{ return (REDIRECTION_RIGHT); }
>>		{ return (APPEND); }
\|		{ return (PIPE); }
{tokens}        { yylval.str = yytext; return (TOKEN); }
;               { return (SEMICOLON); }
\n              { return (EOL); }
#[^\n]*         { }
.               { }
%%

