CC = gcc
CFLAGS = -g -Wall -I./
LIBS = -lpthread

TESTS = tests/test_one_to_many \
		tests/test_many_to_one \
		tests/test_many_to_many \
		tests/test_consume_non_threaded \
		tests/test_no_connections

TESTS_C = ${TESTS:=.c}

.PHONY: gui

all: main

main: main.o streams.o
	$(CC) -lpthread main.o streams.o -o main

gui: gui/gui.c streams.c
	$(CC) $(CFLAGS) $(LIBS) gui/gui.c streams.c `pkg-config --libs --cflags gtk+-2.0` -o gui/gui

tests: ${TESTS}

$(TESTS): ${TESTS_C} streams.c
	@echo
	@echo "========== "$@" =========="
	@echo
	$(CC) $(CFLAGS) $(LIBS) $@.c streams.c -o $@

clean:
	rm ${TESTS} main
