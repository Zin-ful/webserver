CC = gcc
CFLAGS = -Wall -pthread

OUT = server

all: $(OUT)

server: handle_http.o
	   $(CC) $(CFLAGS) handle_http.o -o server

handle_http.o: handle_http.c handle_http.h handle_files.c handle_files.h strops.c strops.h
	   $(CC) $(CFLAGS) -c handle_http.c

clean:
	   rm -f *.o winmngr
