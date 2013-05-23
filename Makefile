CC=clang
CFLAGS=-lev
ANALYZEFLAGS=--analyze -Wall
DEBUGFLAGS=-O0 -g -DDEBUG
PRODFLAGS=-O4
SOURCE=src/midnight.c $(RAGELTARGET)
RAGELSOURCE=src/http11_parser.rl src/worker.rl
RAGELTARGET=$(RAGELSOURCE:.rl=.c)
APPNAME=midnight

all: debug

$(APPNAME): $(RAGELTARGET) $(SOURCE)
	$(CC) $(CFLAGS) $(PRODFLAGS) $(SOURCE) -o $(APPNAME)

analyze: $(RAGELTARGET) $(SOURCE)
	$(CC) $(CFLAGS) $(ANALYZEFLAGS)  $(SOURCE)

debug: $(RAGELTARGET) $(SOURCE)
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(SOURCE) -o $(APPNAME)

$(RAGELTARGET): $(RAGELSOURCE)
	ragel -G2 src/http11_parser.rl
	ragel -G2 src/worker.rl

clean:
	rm -rf $(APPNAME) $(APPNAME).dSYM a.out a.out.dSYM $(patsubst src%, .%, $(SOURCE:.c=.plist)) $(RAGELTARGET:.c=.plist) $(RAGELTARGET) *.o