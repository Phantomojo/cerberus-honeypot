CC=gcc
CFLAGS=-Iinclude -Wall -Wextra -g
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

.PHONY: all clean test test-morph test-quorum test-all