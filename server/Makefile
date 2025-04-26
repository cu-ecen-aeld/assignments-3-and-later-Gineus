CROSS_COMPILE ?= 
CC := $(CROSS_COMPILE)gcc
CFLAGS := -Wall -Werror -g
TARGET := aesdsocket
SRC := aesdsocket.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
