

CFLAGS := -std=c99

all: http-server



http-server: main.c httpserver.h
	#$(CC) $(CFLAGS) -Wall -Wextra -Werror test/main.c -o http-server
	$(CC) $(CFLAGS) -g -Wall  main.c click.c gsnap.c command.c -L/usr/lib  -ljpeg -lz -lm -static -o http-server

clean:
	@rm http-server  *.o
