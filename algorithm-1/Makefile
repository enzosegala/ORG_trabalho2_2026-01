# Makefile para compilação otimizada do módulo A*
CC = gcc
CFLAGS = -O3 -Wall -Wextra -std=c99
LIBS = -lm
TARGET = astar

all: $(TARGET)

$(TARGET): astar.c astar.h
	$(CC) $(CFLAGS) astar.c -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)
