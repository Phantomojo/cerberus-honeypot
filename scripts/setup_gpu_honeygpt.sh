#!/bin/bash
# Complete GPU Setup Script for HoneyGPT
# Automates: nvidia-container-toolkit install, model pull, service restart

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo "=== Cerberus HoneyGPT GPU Setup ==="
echo ""

# Step 1: Check GPU
echo "1. Checking GPU availability..."
./scripts/check_gpu.sh || {
    echo -e "${RED}GPU check failed. Please install NVIDIA drivers and nvidia-container-toolkit.${NC}"
    exit 1
}

echo ""
echo "2. Stopping containers..."
docker-compose -f docker/docker-compose.yml down

echo ""
echo "3. Starting Ollama with GPU..."
docker-compose -f docker/docker-compose.yml up -d ollama

# Wait for Ollama to initialize
echo "Waiting for Ollama to start..."
sleep 5

echo ""
echo "4. Pulling Qwen2.5 3B Q4_K_M model (~2.1GB)..."
echo "   This fits entirely in 4GB VRAM for <1s response times."
docker exec cerberus-ollama ollama pull qwen2.5:3b-instruct-q4_K_M || {
    echo -e "${YELLOW}Model download failed or interrupted. You can retry later with:${NC}"
    echo "  docker exec cerberus-ollama ollama pull qwen2.5:3b-instruct-q4_K_M"
}

echo ""
echo "5. Verifying model..."
if docker exec cerberus-ollama ollama list | grep -q "qwen2.5:3b"; then
    echo -e "${GREEN}✓ Model installed successfully${NC}"
else
    echo -e "${RED}Model verification failed${NC}"
    exit 1
fi

echo ""
echo "6. Starting HoneyGPT with GPU acceleration..."
docker-compose -f docker/docker-compose.yml up -d honeygpt cowrie

echo ""
echo "7. Waiting for HoneyGPT to initialize..."
sleep 10

# Check logs
echo ""
echo "8. Checking HoneyGPT initialization..."
docker logs cerberus-honeygpt --tail 20

echo ""
echo -e "${GREEN}=== GPU Setup Complete! ===${NC}"
echo ""
echo "Next steps:"
echo "  1. Test honeypot: ssh -p 2222 root@localhost"
echo "  2. Run benchmark: ./scripts/benchmark_response_time.sh"
echo " 3. Monitor GPU: watch -n 1 nvidia-smi"
echo ""
echo "Expected performance: <5s response time (vs 30s+ on CPU)"
