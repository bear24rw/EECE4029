all: main.c streams.c queue_a.c queue_a.h
	gcc -g main.c streams.c queue_a.c -lpthread -o main

test: test.c streams.c queue_a.c
	gcc -g test.c streams.c -lpthread -o test

clean:
	rm main
