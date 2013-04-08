CC=clang
CFLAGS=-lev

all: midnight

midnight: midnight.c midnight_logging.c
	$(CC) $(CFLAGS) midnight.c midnight_logging.c -o midnight

clean:
	rm midnight