CC=clang
CFLAGS=-lev

all: midnight

midnight: midnight.c midnight_logging.c midnight_worker.c
	$(CC) $(CFLAGS) midnight.c midnight_logging.c midnight_worker.c -o midnight

clean:
	rm midnight