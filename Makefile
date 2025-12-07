CC = gcc
CFLAGS = -Wall -O2 -pthread
all: ecosystem


ecosystem: main.c
	$(CC) $(CFLAGS) -o ecosystem main.c

run: ecosystem
	./ecosystem <ecosystem_examples/input10x10

clean:
	rm -f ecosystem