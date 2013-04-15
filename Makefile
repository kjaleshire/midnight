CC=clang
CFLAGS=-lev
ANALYZEFLAGS=--analyze -Wall
DEBUGFLAGS=-O0 -g -Wall  -Wwrite-strings -Wdeclaration-after-statement -Wcast-qual -Wstrict-prototypes -Wshadow -Wextra -Wno-deprecated-declarations -Waggregate-return -Wchar-subscripts
PRODFLAGS=-O4
RAGEL=ragel
SOURCEFILES=midnight.c midnight_worker.c midnight_parser.c
SMFILES=midnight_parser.rl
APPNAME=midnight

all: debug

$(APPNAME): $(SOURCEFILES) parser
	$(CC) $(CFLAGS) $(SOURCEFILES) -o $(APPNAME)

analyze: $(SOURCEFILES) parser
	$(CC) $(CFLAGS) $(ANALYZEFLAGS) $(SOURCEFILES)

debug: $(SOURCEFILES) parser
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(SOURCEFILES) -o $(APPNAME)

product: $(SOURCEFILES) parser
	$(CC) $(CFLAGS) $(PRODFLAGS) $(SOURCEFILES) -o $(APPNAME)

parser: $(PARSERFILES)
	$(RAGEL) $(RAGELFLAGS) $(SMFILES)

clean:
	rm -rf $(APPNAME) $(APPNAME).dSYM a.out a.out.dSYM $(SOURCEFILES:.c=.plist) *.o