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

analyze:
	cppcheck \
	--enable=all \
	--inconclusive \
	--std=c11 \
	--quiet \
	core tests	

WIN_CC=x86_64-w64-mingw32-gcc

SIM_SRC=simulator/btnsim_win32.c

WIN_OUT=build/btnsim.exe

win-build: build
	$(WIN_CC) \
	-Wall \
	-O2 \
	$(SIM_SRC) \
	$(CORE_SRC) \
	-o $(WIN_OUT) \
	-lcomctl32 \
	-lgdi32 \
	-luser32	