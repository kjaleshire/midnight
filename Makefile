CC=clang
CFLAGS=-lev
ANALYZEFLAGS=--analyze
DEBUGFLAGS=-O0 -g -Wall
PRODFLAGS=-O4
SOURCEFILES=midnight.c midnight_logging.c midnight_worker.c
APPNAME=midnight

all: debug

$(APPNAME): $(SOURCEFILES)
	$(CC) $(CFLAGS) $(SOURCEFILES) -o $(APPNAME)

analyze: $(SOURCEFILES)
	$(CC) $(CFLAGS) $(ANALYZEFLAGS) $(SOURCEFILES)

debug: $(SOURCEFILES)
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(SOURCEFILES) -o $(APPNAME)

product: $(SOURCEFILES)
	$(CC) $(CFLAGS) $(PRODFLAGS) $(SOURCEFILES) -o $(APPNAME)

clean:
	rm -rf $(APPNAME) $(APPNAME).dSYM a.out a.out.dSYM $(SOURCEFILES:.c=.plist) *.o