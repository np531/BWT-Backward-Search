CC = gcc
CFLAGS = -g -Wall

TARGET = bwtsearch
SRC = bwtsearch.c bwt.c
INC = bwtsearch.h bwt.h

# Object files
OBJS = $(SRC:.c=.o)

# all: $(target)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) -lm
	rm $(OBJS)

$(OBJS): $(INCS)

clean:
	rm -rf $(OBJS) $(TARGET)
