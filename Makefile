CC=clang
CFLAGS=-lev
ANALYZEFLAGS=--analyze -Wall
DEBUGFLAGS=-O0 -g
PRODFLAGS=-O4
SOURCEFILES=midnight.c midnight_worker.c midnight_parser.c
RAGEL=ragel
RAGELFILES=midnight_parser.rl
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

parser: $(RAGELFILES)
	$(RAGEL) $(RAGELFLAGS) $(RAGELFILES)

clean:
	rm -rf $(APPNAME) $(APPNAME).dSYM a.out a.out.dSYM $(SOURCEFILES:.c=.plist) $(RAGELFILES:.rl=.c) *.o