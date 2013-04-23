CC=clang
CFLAGS=-lev -O4
ANALYZEFLAGS=--analyze -Wall -lev
DEBUGFLAGS=-O0 -g -lev
SOURCE=$(wildcard src/*.c) $(RAGELTARGET)
RAGELTARGET=src/http_parser.c src/worker.c
RAGELSOURCE=$(RAGELTARGET:.c=.rl)
APPNAME=midnight

all: debug

$(APPNAME): $(RAGELTARGET)
	$(CC) $(CFLAGS) $(SOURCE) -o $(APPNAME)

analyze: $(RAGELTARGET)
	$(CC) $(ANALYZEFLAGS) $(SOURCE)

debug: $(RAGELTARGET)
	$(CC) $(DEBUGFLAGS) $(SOURCE) -o $(APPNAME)

$(RAGELTARGET): $(RAGELSOURCE)
	ragel -G2 src/http_parser.rl
	ragel -G2 src/worker.rl

clean:
	rm -rf $(APPNAME) $(APPNAME).dSYM a.out a.out.dSYM $(patsubst src%, .%, $(SOURCE:.c=.plist)) $(RAGELTARGET:.c=.plist) $(RAGELTARGET) *.o