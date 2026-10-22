# Usage:
#   make test                 build and run every unit test
#   make test SAN=1           same, with AddressSanitizer + UndefinedBehaviorSanitizer
#   make test CC=clang        pick the compiler
#   make bench                build and run the benchmarks (prints a Markdown table)
#   make coverage             gcov/lcov line coverage of src/ (needs gcc, lcov)
#   make valgrind             run the tests under valgrind (needs valgrind)
CC       ?= cc
SAN      ?= 0
BUILD    ?= build

WARN     := -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wstrict-prototypes -Wmissing-prototypes
CFLAGS   += -std=c17 $(WARN) -Iinclude -g
ifeq ($(SAN),1)
CFLAGS   += -O1 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=undefined
else
CFLAGS   += -O2
endif
ifeq ($(COV),1)
CFLAGS   += -O0 --coverage
endif

SRC      := $(wildcard src/*.c)
OBJ      := $(SRC:src/%.c=$(BUILD)/%.o)
LIB      := $(BUILD)/libdcds.a
TESTS    := $(patsubst tests/%.c,$(BUILD)/%,$(wildcard tests/test_*.c))

.PHONY: all test bench coverage valgrind clean
all: $(LIB)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB): $(OBJ)
	$(AR) rcs $@ $^

$(BUILD)/test_%: tests/test_%.c tests/minunit.h $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

test: $(TESTS)
	@set -e; for t in $(TESTS); do echo "== $$t"; $$t; done

$(BUILD)/bench: bench/bench.c $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) $< $(LIB) -o $@

bench: $(BUILD)/bench
	@$(BUILD)/bench

coverage:
	$(MAKE) clean
	$(MAKE) test COV=1 CC=gcc BUILD=build-cov
	lcov --capture --directory build-cov --output-file build-cov/all.info --quiet --rc branch_coverage=1 --ignore-errors mismatch
	lcov --extract build-cov/all.info "$(CURDIR)/src/*" --output-file build-cov/src.info --quiet --rc branch_coverage=1
	lcov --summary build-cov/src.info --rc branch_coverage=1

valgrind: $(TESTS)
	@set -e; for t in $(TESTS); do echo "== valgrind $$t"; valgrind --leak-check=full --errors-for-leak-kinds=all --error-exitcode=1 -q $$t; done

clean:
	rm -rf build build-cov
