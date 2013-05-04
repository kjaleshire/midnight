CC=clang
CFLAGS=-lev -O4
ANALYZEFLAGS=--analyze -Wall -lev
DEBUGFLAGS=-O0 -g -lev
SOURCE=src/midnight.c $(RAGELTARGET)
RAGELTARGET=src/http11_parser.c src/worker.c
RAGELSOURCE=$(RAGELTARGET:.c=.rl)
APPNAME=midnight

all: debug

$(APPNAME): $(RAGELTARGET) $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) $(RAGELTARGET) -o $(APPNAME)

analyze: $(RAGELTARGET) $(SOURCE)
	$(CC) $(ANALYZEFLAGS)  $(SOURCE)

debug: $(RAGELTARGET) $(SOURCE)
	$(CC) $(DEBUGFLAGS) $(SOURCE) -o $(APPNAME)

$(RAGELTARGET): $(RAGELSOURCE)
	ragel -G2 src/http11_parser.rl
	ragel -G2 src/worker.rl

clean:
	rm -rf $(APPNAME) $(APPNAME).dSYM a.out a.out.dSYM $(patsubst src%, .%, $(SOURCE:.c=.plist)) $(RAGELTARGET:.c=.plist) $(RAGELTARGET) *.o