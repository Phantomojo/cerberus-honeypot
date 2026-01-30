#!/bin/bash
# Comprehensive live SSH command test

HOST="localhost"
PORT="2222"
USER="root"
PASS="admin"

echo "=========================================="
echo "CERBERUS LIVE SSH COMMAND TEST"
echo "=========================================="
echo ""

# Test each command
{
    sleep 1
    echo "$PASS"
    sleep 2

    echo "echo '=== TEST 1: hostname ==='"
    sleep 1
    echo "hostname"
    sleep 2

    echo "echo '=== TEST 2: uname -a ==='"
    sleep 1
    echo "uname -a"
    sleep 2

    echo "echo '=== TEST 3: cat /proc/cpuinfo (first 10 lines) ==='"
    sleep 1
    echo "cat /proc/cpuinfo | head -10"
    sleep 2

    echo "echo '=== TEST 4: cat /proc/meminfo (first 10 lines) ==='"
    sleep 1
    echo "cat /proc/meminfo | head -10"
    sleep 2

    echo "echo '=== TEST 5: route (CUSTOM COMMAND) ==='"
    sleep 1
    echo "route"
    sleep 2

    echo "echo '=== TEST 6: docker ps (CUSTOM COMMAND) ==='"
    sleep 1
    echo "docker ps"
    sleep 2

    echo "echo '=== TEST 7: uptime ==='"
    sleep 1
    echo "uptime"
    sleep 2

    echo "echo '=== TEST 8: whoami ==='"
    sleep 1
    echo "whoami"
    sleep 2

    echo "echo '=== TEST 9: ps aux (first 10 lines) ==='"
    sleep 1
    echo "ps aux | head -10"
    sleep 2

    echo "echo '=== TESTS COMPLETE ==='"
    sleep 1
    echo "exit"

} | ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -p "$PORT" "$USER@$HOST" 2>&1 | \
    grep -v "Warning:\|Permanently added\|algorithm\|password:" | \
    grep -E "^===|^Linux|^root|^D-Link|Kernel|route|ps|uptime|docker|processor|MemTotal|Destination"

echo ""
echo "=========================================="
echo "SSH COMMAND TESTING COMPLETE"
echo "=========================================="
