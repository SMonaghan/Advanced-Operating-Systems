OBJS=comm_test.o lwt.o
CFLAGS=-O3 -I. -w -m32 -fprofile-arcs -ftest-coverage
LIBS=-pthread
DEPS=lwt.h
CC=gcc
BIN=lwt_test
	
all:
	make lwt.o
	make comm_test.o
	$(CC) $(CFLAGS) $(OBJS) -o $(BIN) $(LIBS)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< $(LIBS)

clean:
	rm $(OBJS)
	rm $(BIN)
	
