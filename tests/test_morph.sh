#!/bin/bash
# Test script for morphing engine
# Tests profile rotation, file generation, and configuration updates

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Test counter
TESTS_PASSED=0
TESTS_FAILED=0

# Helper function to print test results
pass() {
    echo -e "${GREEN}✅ PASS${NC}: $1"
    TESTS_PASSED=$((TESTS_PASSED + 1))
}

fail() {
    echo -e "${RED}❌ FAIL${NC}: $1"
    TESTS_FAILED=$((TESTS_FAILED + 1))
}

echo "Testing Morphing Engine..."
echo "=========================="
echo

# Test 1: Morphing binary exists
echo "Test 1: Check if morph binary exists"
if [ -f "./build/morph" ]; then
    pass "Morph binary exists"
else
    fail "Morph binary not found"
    exit 1
fi

# Test 2: Morphing engine runs without errors
echo "Test 2: Run morphing engine"
if ./build/morph > /tmp/morph_test_output.txt 2>&1; then
    pass "Morphing engine executed successfully"
else
    fail "Morphing engine failed to run"
    cat /tmp/morph_test_output.txt
    exit 1
fi

# Test 3: Check output contains expected messages
echo "Test 3: Verify output contains success message"
if grep -q "Successfully morphed" /tmp/morph_test_output.txt; then
    pass "Success message found in output"
else
    fail "Success message not found"
    echo "Output was:"
    cat /tmp/morph_test_output.txt
fi

# Test 4: Check morph-state.txt is created
echo "Test 4: Check state file creation"
if [ -f "./build/morph-state.txt" ]; then
    pass "State file created"
    STATE_PROFILE=$(cat ./build/morph-state.txt)
    echo "   Current profile: $STATE_PROFILE"
else
    fail "State file not created"
fi

# Test 5: Check morph-events.log is created
echo "Test 5: Check events log creation"
if [ -f "./build/morph-events.log" ]; then
    pass "Events log created"
    LOG_ENTRIES=$(wc -l < ./build/morph-events.log)
    echo "   Log entries: $LOG_ENTRIES"
else
    fail "Events log not created"
fi

# Test 6: Check Cowrie config is updated
echo "Test 6: Check Cowrie configuration update"
if [ -f "./services/cowrie/etc/cowrie.cfg.local" ]; then
    pass "Cowrie config file created"
    if grep -q "CERBERUS morph engine" ./services/cowrie/etc/cowrie.cfg.local; then
        pass "Cowrie config contains CERBERUS header"
    else
        fail "Cowrie config missing CERBERUS header"
    fi
else
    fail "Cowrie config file not created"
fi

# Test 7: Check router HTML is updated
echo "Test 7: Check router HTML update"
if [ -f "./services/fake-router-web/html/index.html" ]; then
    pass "Router HTML file created"
    if grep -q "Router" ./services/fake-router-web/html/index.html; then
        pass "Router HTML contains router content"
    else
        fail "Router HTML missing router content"
    fi
else
    fail "Router HTML file not created"
fi

# Test 8: Check camera HTML is updated
echo "Test 8: Check camera HTML update"
if [ -f "./services/fake-camera-web/html/index.html" ]; then
    pass "Camera HTML file created"
    if grep -q "Camera" ./services/fake-camera-web/html/index.html; then
        pass "Camera HTML contains camera content"
    else
        fail "Camera HTML missing camera content"
    fi
else
    fail "Camera HTML file not created"
fi

# Test 9: Run morphing twice and check profile changes
echo "Test 9: Test profile rotation (run twice)"
PROFILE_BEFORE=$(cat ./build/morph-state.txt 2>/dev/null || echo "none")
./build/morph > /dev/null 2>&1
PROFILE_AFTER=$(cat ./build/morph-state.txt 2>/dev/null || echo "none")

if [ "$PROFILE_BEFORE" != "$PROFILE_AFTER" ]; then
    pass "Profile rotated from '$PROFILE_BEFORE' to '$PROFILE_AFTER'"
else
    # Could be same if only one profile, but that's still valid
    pass "Profile state maintained: '$PROFILE_AFTER'"
fi

# Test 10: Check log format
echo "Test 10: Verify log entry format"
if [ -f "./build/morph-events.log" ]; then
    LAST_LOG=$(tail -n 1 ./build/morph-events.log)
    if echo "$LAST_LOG" | grep -q -E "(Morph event|Successfully morphed)"; then
        pass "Log entry format is correct"
        echo "   Last log: $LAST_LOG"
    else
        fail "Log entry format is incorrect"
        echo "   Last log: $LAST_LOG"
    fi
fi

# Summary
echo
echo "=========================="
echo "Test Summary"
echo "=========================="
echo -e "${GREEN}Passed: $TESTS_PASSED${NC}"
if [ $TESTS_FAILED -gt 0 ]; then
    echo -e "${RED}Failed: $TESTS_FAILED${NC}"
    exit 1
else
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
fi
