CC=gcc
# Production flags: optimized with security hardening
CFLAGS=-Iinclude -Wall -Wextra -O2 -march=native -fstack-protector-strong -D_FORTIFY_SOURCE=2
# Development flags: with sanitizers for debugging
CFLAGS_DEBUG=-Iinclude -Wall -Wextra -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
BUILD=build

SRC_MORPH=src/morph/morph.c
SRC_QUORUM=src/quorum/quorum.c
SRC_UTILS=src/utils/utils.c
INCLUDES=include/morph.h include/quorum.h include/utils.h

all: $(BUILD)/morph $(BUILD)/quorum

$(BUILD)/morph: $(SRC_MORPH) $(SRC_UTILS) $(INCLUDES)
	$(CC) $(CFLAGS) -o $(BUILD)/morph $(SRC_MORPH) $(SRC_UTILS)

$(BUILD)/quorum: $(SRC_QUORUM) $(SRC_UTILS) $(INCLUDES)
	$(CC) $(CFLAGS) -o $(BUILD)/quorum $(SRC_QUORUM) $(SRC_UTILS)

# Debug builds with sanitizers
debug: CFLAGS=$(CFLAGS_DEBUG)
debug: clean all
	@echo "Debug build complete with sanitizers enabled"

# Static analysis
analyze:
	@echo "Running static analysis..."
	@command -v cppcheck >/dev/null 2>&1 && cppcheck --enable=all --suppress=missingIncludeSystem src/ || echo "cppcheck not installed"
	@command -v clang-tidy >/dev/null 2>&1 && clang-tidy src/*/*.c -- -Iinclude || echo "clang-tidy not installed"

# Memory leak check (requires valgrind)
memcheck: $(BUILD)/morph $(BUILD)/quorum
	@echo "Running memory leak detection..."
	@command -v valgrind >/dev/null 2>&1 && valgrind --leak-check=full --show-leak-kinds=all ./build/morph profiles.conf || echo "valgrind not installed"

clean:
	rm -rf $(BUILD)/*

test: test-morph test-quorum

test-morph: $(BUILD)/morph
	@echo "=== Testing Morphing Engine ==="
	@./tests/test_morph.sh

test-quorum: $(BUILD)/quorum
	@echo "=== Testing Quorum Engine ==="
	@./tests/test_quorum.sh

test-all: all test
	@echo "=== All tests completed ==="

.PHONY: all debug analyze memcheck clean test test-morph test-quorum test-all