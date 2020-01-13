SOURCES = parser_helpfile.c entry.c
OBJECTS = parser_helpfile.o entry.o
CFLAGS = -I. -Wall -ggdb3 -pedantic

mysh: parser.tab.h lex.yy.c parser.l parser.y $(OBJECTS)
        $(CC) $(CFLAGS) -o $@ $(OBJECTS) parser.tab.c lex.yy.c -lreadline

parser.tab.h: parser.y
        bison -d parser.y

lex.yy.c: parser.l
        flex parser.l

clean:
        rm lex.yy.c parser.tab.c parser.tab.h mysh $(OBJECTS)