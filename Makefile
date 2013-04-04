CC=clang
CFLAGS=

all: october

october: october.c october_worker.c october_logging.c october_parser.c
	$(CC) $(CFLAGS) october.c october_worker.c october_logging.c october_parser.c -o october

clean:
	rm october