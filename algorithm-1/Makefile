CC=gcc
CFLAGS=-O3 -Wall
TARGET=astar

all: $(TARGET)

$(TARGET): astar.c
	$(CC) $(CFLAGS) astar.c -lm -o $(TARGET)

clean:
	rm -f $(TARGET)