OBJS=main1.o lwt.o
CFLAGS=-O3 -I. -w -m32
#DEFINES=-DTHINK_TIME
DEPS = lwt.h
CC=gcc
BIN = lwt_test
	
all:
	make lwt.o
	make main1.o
	$(CC) $(CFLAGS) $(OBJS) -o $(BIN)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm $(OBJS)
	rm $(BIN)
	
