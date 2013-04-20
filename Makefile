CC=clang
CFLAGS=-lev -O4
ANALYZEFLAGS=--analyze -Wall -lev
DEBUGFLAGS=-O0 -g -lev
SOURCE=$(wildcard src/*.c)
RAGELSOURCE=src/http_parser.c src/conn_state.c
APPNAME=midnight

all: debug

$(APPNAME): $(RAGELSOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(APPNAME)

analyze: $(RAGELSOURCE)
	$(CC) $(ANALYZEFLAGS) $(SOURCE)

debug: $(RAGELSOURCE)
	$(CC) $(DEBUGFLAGS) $(SOURCE) -o $(APPNAME)

$(RAGELSOURCE):
	ragel -G2 src/http_parser.rl
	ragel -G2 src/conn_state.rl

clean:
	rm -rf $(APPNAME) $(APPNAME).dSYM a.out a.out.dSYM $(SOURCEFILES:.c=.plist) $(RAGELSOURCE:.c=.plist) $(RAGELSOURCE) *.o