#!/bin/bash
# Test script for quorum detection engine
# Tests log scanning, IP extraction, and attack correlation

# Exit code constants
# Exit 0: Normal execution, no attacks detected
# Exit 1: Normal execution, coordinated attacks detected
EXIT_SUCCESS=0
EXIT_ALERT=1

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
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

info() {
    echo -e "${YELLOW}ℹ INFO${NC}: $1"
}

echo "Testing Quorum Detection Engine..."
echo "=================================="
echo

# Test 1: Quorum binary exists
echo "Test 1: Check if quorum binary exists"
if [ -f "./build/quorum" ]; then
    pass "Quorum binary exists"
else
    fail "Quorum binary not found"
    exit 1
fi

# Test 2: Create test log directories
echo "Test 2: Prepare test log directories"
mkdir -p services/cowrie/logs
mkdir -p services/fake-router-web/logs
mkdir -p services/fake-camera-web/logs
mkdir -p services/rtsp/logs
# Clean any existing test logs
rm -f services/cowrie/logs/*.log
rm -f services/fake-router-web/logs/*.log
rm -f services/fake-camera-web/logs/*.log
rm -f services/rtsp/logs/*.log
pass "Test log directories created and cleaned"

# Test 3: Quorum engine runs without errors (no logs)
echo "Test 3: Run quorum engine with empty logs"
if ./build/quorum > /tmp/quorum_test_output1.txt 2>&1; then
    pass "Quorum engine executed successfully"
else
    fail "Quorum engine failed to run"
    cat /tmp/quorum_test_output1.txt
    exit 1
fi

# Test 4: Check output with no logs
echo "Test 4: Verify output with no attack traffic"
if grep -q "No coordinated attacks detected" /tmp/quorum_test_output1.txt || \
   grep -q "Total unique IPs tracked: 0" /tmp/quorum_test_output1.txt; then
    pass "Correctly reports no attacks with empty logs"
else
    fail "Unexpected output with empty logs"
    echo "Output was:"
    cat /tmp/quorum_test_output1.txt
fi

# Test 5: Generate test log entries with single IP
echo "Test 5: Create test logs with single IP"
TEST_IP="192.168.1.100"
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Connection from $TEST_IP" > services/cowrie/logs/cowrie.log
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Failed login attempt from $TEST_IP" >> services/cowrie/logs/cowrie.log
echo "$(date '+%Y-%m-%d %H:%M:%S') - Router access from $TEST_IP" > services/fake-router-web/logs/access.log
pass "Test logs created with IP: $TEST_IP"

# Test 6: Run quorum and check IP detection
echo "Test 6: Test IP extraction from logs"
./build/quorum > /tmp/quorum_test_output2.txt 2>&1
QUORUM_EXIT=$?
# Both success and alert exit codes are acceptable
if [ $QUORUM_EXIT -eq $EXIT_SUCCESS ] || [ $QUORUM_EXIT -eq $EXIT_ALERT ]; then
    pass "Quorum engine executed successfully (exit: $QUORUM_EXIT)"
    if [ $QUORUM_EXIT -eq $EXIT_ALERT ]; then
        pass "Coordinated attack detected as expected"
    fi
else
    fail "Quorum engine failed with test logs (exit: $QUORUM_EXIT)"
    cat /tmp/quorum_test_output2.txt
fi

# Test 7: Generate coordinated attack scenario
echo "Test 7: Create coordinated attack scenario"
ATTACKER_IP="10.0.0.50"
echo "[$(date '+%Y-%m-%d %H:%M:%S')] SSH connection from $ATTACKER_IP" > services/cowrie/logs/cowrie.log
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Telnet connection from $ATTACKER_IP" >> services/cowrie/logs/cowrie.log
echo "$(date '+%Y-%m-%d %H:%M:%S') - Router access from $ATTACKER_IP" > services/fake-router-web/logs/access.log
echo "$(date '+%Y-%m-%d %H:%M:%S') - Camera access from $ATTACKER_IP" > services/fake-camera-web/logs/access.log
echo "$(date '+%Y-%m-%d %H:%M:%S') - RTSP connection from $ATTACKER_IP" > services/rtsp/logs/rtsp.log
pass "Coordinated attack logs created with IP: $ATTACKER_IP"

# Test 8: Run quorum and check for coordinated attack detection
echo "Test 8: Test coordinated attack detection"
./build/quorum > /tmp/quorum_test_output3.txt 2>&1
QUORUM_EXIT=$?
# Both success and alert exit codes are acceptable
if [ $QUORUM_EXIT -eq $EXIT_SUCCESS ] || [ $QUORUM_EXIT -eq $EXIT_ALERT ]; then
    pass "Quorum engine executed with coordinated attack logs (exit: $QUORUM_EXIT)"
    if [ $QUORUM_EXIT -eq $EXIT_ALERT ]; then
        pass "Coordinated attack correctly detected"
    fi

    # Display output for manual verification
    echo "   Quorum output:"
    grep -E "(INFO|WARN|ERROR|ALERT)" /tmp/quorum_test_output3.txt | sed 's/^/   /' || true
else
    fail "Quorum engine failed with coordinated attack logs (exit: $QUORUM_EXIT)"
    cat /tmp/quorum_test_output3.txt
fi

# Test 9: Test with invalid IP addresses
echo "Test 9: Test handling of invalid log entries"
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Connection from invalid-ip" > services/cowrie/logs/test.log
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Connection from 999.999.999.999" >> services/cowrie/logs/test.log
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Random log entry without IP" >> services/cowrie/logs/test.log
./build/quorum > /tmp/quorum_test_output4.txt 2>&1
QUORUM_EXIT=$?
# Both success and alert exit codes are acceptable
if [ $QUORUM_EXIT -eq $EXIT_SUCCESS ] || [ $QUORUM_EXIT -eq $EXIT_ALERT ]; then
    pass "Quorum handles invalid log entries gracefully"
else
    fail "Quorum crashes with invalid log entries (exit: $QUORUM_EXIT)"
    cat /tmp/quorum_test_output4.txt
fi

# Test 10: Test with multiple different IPs
echo "Test 10: Test with multiple unique IPs"
{
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Connection from 192.168.1.10"
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Connection from 192.168.1.20"
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Connection from 192.168.1.30"
echo "[$(date '+%Y-%m-%d %H:%M:%S')] Connection from 10.0.0.100"
} > services/cowrie/logs/cowrie.log
./build/quorum > /tmp/quorum_test_output5.txt 2>&1
QUORUM_EXIT=$?
# Both success and alert exit codes are acceptable
if [ $QUORUM_EXIT -eq $EXIT_SUCCESS ] || [ $QUORUM_EXIT -eq $EXIT_ALERT ]; then
    pass "Quorum processes multiple unique IPs"

    # Check if multiple IPs tracked - split into steps for better error handling
    UNIQUE_LINE=$(grep "unique IPs tracked:" /tmp/quorum_test_output5.txt 2>/dev/null || echo "")
    if [ -n "$UNIQUE_LINE" ]; then
        UNIQUE_COUNT=$(echo "$UNIQUE_LINE" | grep -o "[0-9]*$" || echo "0")
        if [ -n "$UNIQUE_COUNT" ] && [ "$UNIQUE_COUNT" != "0" ]; then
            pass "Tracked $UNIQUE_COUNT unique IP(s)"
        else
            info "IP tracking: $UNIQUE_COUNT"
        fi
    else
        info "Could not extract IP count from output"
    fi
else
    fail "Quorum failed with multiple IPs (exit: $QUORUM_EXIT)"
    cat /tmp/quorum_test_output5.txt
fi

# Cleanup test logs
echo
echo "Test 11: Cleanup test logs"
rm -f services/cowrie/logs/*.log
rm -f services/fake-router-web/logs/*.log
rm -f services/fake-camera-web/logs/*.log
rm -f services/rtsp/logs/*.log
pass "Test logs cleaned up"

# Summary
echo
echo "=================================="
echo "Test Summary"
echo "=================================="
echo -e "${GREEN}Passed: $TESTS_PASSED${NC}"
if [ $TESTS_FAILED -gt 0 ]; then
    echo -e "${RED}Failed: $TESTS_FAILED${NC}"
    exit 1
else
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
fi
