CC=clang
CFLAGS=-Isrc/
ANALYZEFLAGS=--analyze -Wall -DDEBUG
DEBUGFLAGS=-O0 -g -DDEBUG
PRODFLAGS=-O4
LIBS=-lev
SOURCE=src/midnight.c $(RAGELTARGET)
RAGELSOURCE=src/http11_parser.rl src/mdt_worker.rl
RAGELTARGET=$(RAGELSOURCE:.rl=.c)
APPNAME=midnight

all: debug

$(APPNAME): $(RAGELTARGET) $(SOURCE)
	$(CC) $(CFLAGS) $(LIBS) $(PRODFLAGS) $(SOURCE) -o $(APPNAME)

analyze: $(RAGELTARGET) $(SOURCE)
	$(CC) $(CFLAGS) $(ANALYZEFLAGS)  $(SOURCE)

debug: $(RAGELTARGET) $(SOURCE)
	$(CC) $(CFLAGS) $(LIBS) $(DEBUGFLAGS) $(SOURCE) -o $(APPNAME)

$(RAGELTARGET): $(RAGELSOURCE)
	ragel -G2 src/http11_parser.rl
	ragel -G2 src/mdt_worker.rl

clean:
	rm -rf $(APPNAME) $(APPNAME).dSYM a.out a.out.dSYM $(patsubst src%, .%, $(SOURCE:.c=.plist)) $(RAGELTARGET:.c=.plist) $(RAGELTARGET) *.o

.PHONY: all clean analyze debug