OBJS=test.o
CFLAGS=-g -w -m32
#DEFINES=-DTHINK_TIME
BIN=thread
CC=gcc
%.o:%.c
	$(CC) $(CFLAGS) $(DEFINES) -o $@ -c $<
$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(DEFINES) -o $(BIN) $^
	rm $(OBJS)
clean:
	rm $(BIN)
