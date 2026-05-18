CC=gcc

CFLAGS=-Wall -Wextra -std=c11 -O2

CORE_SRC=core/btn_fsm.c

TEST_SRC=tests/test_btn.c

TEST_OUT=build/test_btn

all: test

build:
	mkdir -p build

test: build
	$(CC) $(CFLAGS) \
	$(CORE_SRC) \
	$(TEST_SRC) \
	-o $(TEST_OUT)

run-tests: test
	./$(TEST_OUT)

clean:
	rm -rf build