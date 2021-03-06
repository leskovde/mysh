OBJECTS = src/signals.o src/entry.o src/utilities.o src/execution.o
CFLAGS = -I. -Wall -ggdb3 -pedantic

mysh: parser.tab.h lex.yy.c src/parser.l src/parser.y $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) parser.tab.c lex.yy.c -lreadline

parser.tab.h: src/parser.y
	bison -d src/parser.y

lex.yy.c: src/parser.l
	flex src/parser.l

clean:
	rm -f lex.yy.c parser.tab.c parser.tab.h mysh $(OBJECTS)

test:
	cd tests && ./run-tests.sh
