OBJS=../lwt_thread/lwt.o server.o simple_http.o content.o main.o util.o
CFLAGS=-O3 -I. -w -m32 -fprofile-arcs -ftest-coverage
LIBS=-pthread
DEPS=ll.h ../lwt_thread/lwt.h
CC=gcc
BIN=server
	
all:
	make ../lwt_thread/lwt.o
	make util.o
	make content.o
	make server.o
	make simple_http.o
	make ll.h
	make main.o
	$(CC) $(CFLAGS) $(OBJS) -o $(BIN) $(LIBS)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< $(LIBS)

clean:
	rm $(OBJS)
	rm $(BIN)
	rm *gcno *gcov	
