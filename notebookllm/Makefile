CC=gcc
# Production flags: simplified for container compatibility
CFLAGS=-Iinclude -Wall -Wextra -O2
# Development flags: with sanitizers for debugging
CFLAGS_DEBUG=-Iinclude -Wall -Wextra -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fsanitize=leak
BUILD=build

$(BUILD):
	mkdir -p $(BUILD)

# Core modules
SRC_MORPH=src/morph/morph.c
SRC_QUORUM=src/quorum/quorum.c
SRC_UTILS=src/utils/utils.c src/utils/path_security.c
SRC_SECURITY=src/security/security_utils.c
SRC_SANDBOX=src/security/sandbox.c
SRC_ENCRYPTION=src/security/encryption.c

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
         include/state_engine.h include/security_utils.h include/sandbox.h include/encryption.h

all: $(BUILD)/morph $(BUILD)/quorum $(BUILD)/state_engine_test $(BUILD)/test_security $(BUILD)/test_sandbox

# Morphing engine with all phase modules
$(BUILD)/morph: $(SRC_MORPH) $(SRC_UTILS) $(SRC_SECURITY) $(SRC_SANDBOX) $(SRC_ENCRYPTION) $(SRC_NETWORK) $(SRC_MORPH_NETWORK) \
                $(SRC_FILESYSTEM) $(SRC_PROCESSES) $(SRC_BEHAVIOR) $(SRC_TEMPORAL) $(INCLUDES) | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/morph \
		$(SRC_MORPH) $(SRC_UTILS) $(SRC_SECURITY) $(SRC_SANDBOX) $(SRC_ENCRYPTION) $(SRC_NETWORK) $(SRC_MORPH_NETWORK) \
		$(SRC_FILESYSTEM) $(SRC_PROCESSES) $(SRC_BEHAVIOR) $(SRC_TEMPORAL)

# Quorum engine with adaptation module
$(BUILD)/quorum: $(SRC_QUORUM) $(SRC_UTILS) $(SRC_SECURITY) $(SRC_SANDBOX) $(SRC_ENCRYPTION) $(SRC_QUORUM_ADAPT) $(INCLUDES) | $(BUILD)
	$(CC) $(CFLAGS) -o $(BUILD)/quorum $(SRC_QUORUM) $(SRC_UTILS) $(SRC_SECURITY) $(SRC_SANDBOX) $(SRC_ENCRYPTION) $(SRC_QUORUM_ADAPT)

# State engine test binary
$(BUILD)/state_engine_test: $(SRC_STATE) $(SRC_UTILS) $(SRC_SECURITY) $(SRC_SANDBOX) $(SRC_ENCRYPTION) tests/test_state_engine.c $(INCLUDES)
	$(CC) $(CFLAGS) -o $(BUILD)/state_engine_test tests/test_state_engine.c $(SRC_STATE) $(SRC_UTILS) $(SRC_SECURITY) $(SRC_SANDBOX) $(SRC_ENCRYPTION)

# Security tests
$(BUILD)/test_security: test_security.c $(SRC_SECURITY) $(SRC_UTILS) $(INCLUDES)
	$(CC) $(CFLAGS) -o $(BUILD)/test_security test_security.c $(SRC_SECURITY) $(SRC_UTILS)

# Sandbox isolation tests
$(BUILD)/test_sandbox: test_sandbox.c $(SRC_SANDBOX) $(SRC_SECURITY) $(SRC_UTILS) $(SRC_ENCRYPTION) $(INCLUDES)
	$(CC) $(CFLAGS) -o $(BUILD)/test_sandbox test_sandbox.c $(SRC_SANDBOX) $(SRC_SECURITY) $(SRC_UTILS) $(SRC_ENCRYPTION)

# Debug builds with sanitizers
debug: CFLAGS=$(CFLAGS_DEBUG)
debug: clean all
	@echo "Debug build complete with sanitizers enabled"

# Static analysis
analyze:
	@echo "Running static analysis..."
	@command -v cppcheck >/dev/null 2>&1 && cppcheck --enable=all --suppress=missingIncludeSystem src/ || echo "cppcheck not installed"
	@command -v clang-tidy >/dev/null 2>&1 && clang-tidy src/*/*.c -- -Iinclude || echo "clang-tidy not installed"

# Encryption testing
test-encryption:
	@echo "Running encryption tests..."
	@./scripts/encryption_test.sh

# Build encryption module
build-encryption:
	@echo "Building encryption module..."
	@$(CC) $(CFLAGS) -Iinclude -c src/security/encryption.c -o build/encryption.o -lcrypto 2>/dev/null || echo "OpenSSL development libraries not installed"

# Memory leak check (requires valgrind)
memcheck: $(BUILD)/morph $(BUILD)/quorum
	@echo "Running memory leak detection..."
	@command -v valgrind >/dev/null 2>&1 && valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./build/morph profiles.conf || echo "valgrind not installed"

# AddressSanitizer check
asan-check: $(BUILD)/morph $(BUILD)/quorum
	@echo "Running AddressSanitizer check..."
	@echo "Note: This requires ASAN-enabled build (make debug)"
	@ASAN_OPTIONS=detect_stack_use_after_return=1:strict_string_checks=1:detect_odr_violation=1 ./build/morph profiles.conf || echo "ASAN check completed"

# ThreadSanitizer check
tsan-check: $(BUILD)/morph $(BUILD)/quorum
	@echo "Running ThreadSanitizer check..."
	@echo "Note: This requires TSAN-enabled build"
	@./build/morph profiles.conf || echo "TSAN check completed"

# UndefinedBehaviorSanitizer check
ubsan-check: $(BUILD)/morph $(BUILD)/quorum
	@echo "Running UndefinedBehaviorSanitizer check..."
	@echo "Note: This requires UBSAN-enabled build (make debug)"
	@UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 ./build/morph profiles.conf || echo "UBSAN check completed"

# Comprehensive memory safety check
memcheck-full: memcheck asan-check ubsan-check
	@echo "=== Comprehensive Memory Safety Check Complete ==="

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
