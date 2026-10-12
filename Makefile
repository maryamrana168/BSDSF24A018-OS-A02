CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = bin/ls
SOURCE = src/ls-v1.0.0.c
OBJECT = obj/ls-v1.0.0.o

all: $(TARGET)

$(TARGET): $(OBJECT)
	$(CC) $(CFLAGS) $(OBJECT) -o $(TARGET)

$(OBJECT): $(SOURCE)
	$(CC) $(CFLAGS) -c $(SOURCE) -o $(OBJECT)

bin obj:
	mkdir -p $@

clean:
	rm -f $(OBJECT) $(TARGET)

.PHONY: all clean
