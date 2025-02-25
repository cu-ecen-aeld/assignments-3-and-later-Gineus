CC = gcc
CFLAGS = -Wall -Wextra -Werror -g
TARGET = writer
SRC = writer.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
	rm -f *.o

.PHONY: all clean

