CC = gcc
CFLAGS = -g -Wall
LIBS = -lpthread

TESTS = test_one_to_many \
		test_many_to_one \
		test_many_to_many

TESTS_C = ${TESTS:=.c}

all: main

main: main.o streams.o
	$(CC) -lpthread main.o streams.o -o main

tests: ${TESTS}

$(TESTS): ${TESTS_C} streams.c
	@echo
	@echo "========== "$@" =========="
	@echo
	$(CC) $(CFLAGS) $(LIBS) $@.c streams.c -o $@

clean:
	rm ${TESTS} main
