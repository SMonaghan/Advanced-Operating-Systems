OBJS=slab.o
CFLAGS=-g -I. -w
#DEFINES=-DTHINK_TIME
BIN=allocator_test
CC=gcc
%.o:%.c
	$(CC) $(CFLAGS) $(DEFINES) -o $@ -c $<
$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(DEFINES) -o $(BIN) $^
	rm $(OBJS)
clean:
	rm $(BIN)
