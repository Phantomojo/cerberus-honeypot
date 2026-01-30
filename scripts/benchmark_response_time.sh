#!/bin/bash
# HoneyGPT Performance Benchmark Script
# Tests response times for GPU-accelerated vs CPU-only inference

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "=== HoneyGPT GPU Acceleration Benchmark ==="
echo ""

# Test commands (router-specific + common Linux)
declare -a COMMANDS=(
    "nvram get wl_ssid"
    "show running-config"
    "cat /proc/cpuinfo"
    "uname -a"
    "ls -la /etc"
    "ps aux"
    "ifconfig"
)

# Wait for HoneyGPT to be ready
echo "Waiting for HoneyGPT service to initialize..."
sleep 5

# Check if model is loaded
echo "Checking if Qwen2.5 model is available..."
docker exec cerberus-ollama ollama list | grep qwen2.5 || {
    echo -e "${YELLOW}Warning: Qwen2.5 model not found. Please run:${NC}"
    echo "  docker exec cerberus-ollama ollama pull qwen2.5:7b-instruct-q4_K_M"
    exit 1
}

echo -e "${GREEN}✓ Model ready${NC}"
echo ""

# Function to test command response time
test_command() {
    local cmd="$1"
    local start=$(date +%s.%N)

    # Send command to HoneyGPT via socket (port 50051)
    # capture output to check if we got a response or connection failed
    output=$(echo "$cmd" | nc -w 30 localhost 50051 2>&1)
    exit_code=$?

    if [ $exit_code -ne 0 ] || [ -z "$output" ]; then
        echo "FAIL (Connection Error)"
        return
    fi

    local end=$(date +%s.%N)
    local duration=$(echo "$end - $start" | bc)

    echo "$duration"
}

echo "Running benchmark tests..."
echo "Target: <5 seconds average response time"
echo ""

total_time=0
count=0

for cmd in "${COMMANDS[@]}"; do
    echo -n "Testing: $cmd ... "

    response_time=$(test_command "$cmd")
    total_time=$(echo "$total_time + $response_time" | bc)
    count=$((count + 1))

    printf "%6.2fs" "$response_time"

    if (( $(echo "$response_time < 5" | bc -l) )); then
        echo -e " ${GREEN}✓ PASS${NC}"
    else
        echo -e " ${YELLOW}⚠ SLOW${NC}"
    fi
done

echo ""
echo "=== Results ==="

avg_time=$(echo "scale=2; $total_time / $count" | bc)
echo "Average response time: ${avg_time}s"
echo "Total commands tested: $count"

if (( $(echo "$avg_time < 5" | bc -l) )); then
    echo -e "${GREEN}✓ Performance target MET (<5s average)${NC}"
else
    echo -e "${YELLOW}⚠ Performance target MISSED (${avg_time}s > 5s)${NC}"
    echo "Consider:"
    echo "  - Verify GPU is being used: docker exec cerberus-ollama nvidia-smi"
    echo "  - Check VRAM usage during inference"
    echo "  - Reduce num_ctx or num_predict in llm_client.py"
fi

echo ""
echo "Detailed logs: build/honeygpt.log"
echo "GPU utilization: nvidia-smi -l 1"
