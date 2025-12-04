CC=gcc
# Production flags: optimized with security hardening
CFLAGS=-Iinclude -Wall -Wextra -O2 -march=native -fstack-protector-strong -D_FORTIFY_SOURCE=2
# Development flags: with sanitizers for debugging
CFLAGS_DEBUG=-Iinclude -Wall -Wextra -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
BUILD=build

# Core modules
SRC_MORPH=src/morph/morph.c
SRC_QUORUM=src/quorum/quorum.c
SRC_UTILS=src/utils/utils.c

# Phase modules (1-6)
SRC_NETWORK=src/network/network.c
SRC_MORPH_NETWORK=src/morph/morph_network.c
SRC_FILESYSTEM=src/filesystem/filesystem.c
SRC_PROCESSES=src/processes/processes.c
SRC_BEHAVIOR=src/behavior/behavior.c
SRC_TEMPORAL=src/temporal/temporal.c
SRC_QUORUM_ADAPT=src/quorum/quorum_adapt.c

# State engine
SRC_STATE=src/state/state_engine.c

# All includes
INCLUDES=include/morph.h include/quorum.h include/utils.h \
         include/network.h include/filesystem.h include/processes.h \
         include/behavior.h include/temporal.h include/quorum_adapt.h \
         include/state_engine.h

all: $(BUILD)/morph $(BUILD)/quorum $(BUILD)/state_engine_test

# Morphing engine with all phase modules
$(BUILD)/morph: $(SRC_MORPH) $(SRC_UTILS) $(SRC_NETWORK) $(SRC_MORPH_NETWORK) \
                $(SRC_FILESYSTEM) $(SRC_PROCESSES) $(SRC_BEHAVIOR) $(SRC_TEMPORAL) $(INCLUDES)
	$(CC) $(CFLAGS) -o $(BUILD)/morph \
		$(SRC_MORPH) $(SRC_UTILS) $(SRC_NETWORK) $(SRC_MORPH_NETWORK) \
		$(SRC_FILESYSTEM) $(SRC_PROCESSES) $(SRC_BEHAVIOR) $(SRC_TEMPORAL)
	@test -x ./scripts/add_dynamic_commands.sh && ./scripts/add_dynamic_commands.sh || true

# Quorum engine with adaptation module
$(BUILD)/quorum: $(SRC_QUORUM) $(SRC_UTILS) $(SRC_QUORUM_ADAPT) $(INCLUDES)
	$(CC) $(CFLAGS) -o $(BUILD)/quorum $(SRC_QUORUM) $(SRC_UTILS) $(SRC_QUORUM_ADAPT)

# State engine test binary
$(BUILD)/state_engine_test: $(SRC_STATE) $(SRC_UTILS) tests/test_state_engine.c $(INCLUDES)
	$(CC) $(CFLAGS) -o $(BUILD)/state_engine_test tests/test_state_engine.c $(SRC_STATE) $(SRC_UTILS)

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

test: test-morph test-quorum test-state

test-morph: $(BUILD)/morph
	@echo "=== Testing Morphing Engine ==="
	@./tests/test_morph.sh

test-quorum: $(BUILD)/quorum
	@echo "=== Testing Quorum Engine ==="
	@./tests/test_quorum.sh

test-state: $(BUILD)/state_engine_test
	@echo "=== Testing State Engine ==="
	@./$(BUILD)/state_engine_test

test-all: all test
	@echo "=== All tests completed ==="

.PHONY: all debug analyze memcheck clean test test-morph test-quorum test-state test-all
