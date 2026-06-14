CC = gcc
CFLAGS = -Wall -O3 -march=native
LDFLAGS = -lm

all: fft_test

fft_test: fft.c fft.h
	$(CC) $(CFLAGS) -o fft_test fft.c $(LDFLAGS)

clean:
	rm -f fft_test *.o
