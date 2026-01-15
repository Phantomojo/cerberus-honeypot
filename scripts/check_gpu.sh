#!/bin/bash
# GPU Detection and Validation Script for Cerberus HoneyGPT
# Checks NVIDIA GPU availability, drivers, and CUDA compatibility

set -e

echo "=== Cerberus HoneyGPT GPU Detection ==="
echo ""

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Exit codes
EXIT_GPU_READY=0
EXIT_SETUP_NEEDED=1
EXIT_NO_GPU=2

# Check for NVIDIA GPU
echo "1. Checking for NVIDIA GPU..."
if lspci | grep -i nvidia > /dev/null 2>&1; then
    GPU_NAME=$(lspci | grep -i nvidia | grep -i vga | head -1 | cut -d: -f3 | xargs)
    echo -e "${GREEN}✓${NC} NVIDIA GPU detected: ${GPU_NAME}"
else
    echo -e "${RED}✗${NC} No NVIDIA GPU found"
    echo "   This system does not have an NVIDIA GPU or it's not recognized."
    exit $EXIT_NO_GPU
fi

# Check for nvidia-smi (driver installed)
echo ""
echo "2. Checking NVIDIA drivers..."
if command -v nvidia-smi &> /dev/null; then
    DRIVER_VERSION=$(nvidia-smi --query-gpu=driver_version --format=csv,noheader | head -1)
    CUDA_VERSION=$(nvidia-smi | grep "CUDA Version" | awk '{print $9}')
    echo -e "${GREEN}✓${NC} NVIDIA drivers installed"
    echo "   Driver Version: ${DRIVER_VERSION}"
    echo "   CUDA Version: ${CUDA_VERSION}"

    # Show GPU memory
    GPU_MEM=$(nvidia-smi --query-gpu=memory.total --format=csv,noheader | head -1)
    GPU_MEM_USED=$(nvidia-smi --query-gpu=memory.used --format=csv,noheader | head -1)
    echo "   GPU Memory: ${GPU_MEM_USED} / ${GPU_MEM}"
else
    echo -e "${RED}✗${NC} nvidia-smi not found"
    echo "   NVIDIA drivers may not be installed correctly."
    echo "   Install with: sudo apt install nvidia-driver-XXX (or equivalent for your distro)"
    exit $EXIT_SETUP_NEEDED
fi

# Check for Docker
echo ""
echo "3. Checking Docker installation..."
if command -v docker &> /dev/null; then
    DOCKER_VERSION=$(docker --version | awk '{print $3}' | tr -d ',')
    echo -e "${GREEN}✓${NC} Docker installed: ${DOCKER_VERSION}"
else
    echo -e "${RED}✗${NC} Docker not found"
    echo "   Install Docker: https://docs.docker.com/engine/install/"
    exit $EXIT_SETUP_NEEDED
fi

# Check for nvidia-container-toolkit
echo ""
echo "4. Checking NVIDIA Container Toolkit..."
if docker run --rm --gpus all nvidia/cuda:12.0.0-base-ubuntu22.04 nvidia-smi &> /dev/null; then
    echo -e "${GREEN}✓${NC} NVIDIA Container Toolkit working"
else
    echo -e "${YELLOW}!${NC} NVIDIA Container Toolkit not configured or not installed"
    echo "   Installing nvidia-container-toolkit..."

    # Detect package manager
    if command -v apt &> /dev/null; then
        echo "   Using apt (Debian/Ubuntu)..."
        echo "   Run the following commands:"
        echo ""
        echo "   curl -fsSL https://nvidia.github.io/libnvidia-container/gpgkey | sudo gpg --dearmor -o /usr/share/keyrings/nvidia-container-toolkit-keyring.gpg"
        echo "   curl -s -L https://nvidia.github.io/libnvidia-container/stable/deb/nvidia-container-toolkit.list | sed 's#deb https://#deb [signed-by=/usr/share/keyrings/nvidia-container-toolkit-keyring.gpg] https://#g' | sudo tee /etc/apt/sources.list.d/nvidia-container-toolkit.list"
        echo "   sudo apt update"
        echo "   sudo apt install -y nvidia-container-toolkit"
        echo "   sudo systemctl restart docker"
    elif command -v dnf &> /dev/null; then
        echo "   Using dnf (Fedora/RHEL)..."
        echo "   Run: sudo dnf install -y nvidia-container-toolkit"
        echo "   sudo systemctl restart docker"
    elif command -v pacman &> /dev/null; then
        echo "   Using pacman (Arch)..."
        echo "   Run: sudo pacman -S nvidia-container-toolkit"
        echo "   sudo systemctl restart docker"
    fi

    exit $EXIT_SETUP_NEEDED
fi

# Check Docker daemon config for nvidia runtime
echo ""
echo "5. Checking Docker daemon configuration..."
if [ -f /etc/docker/daemon.json ]; then
    if grep -q "nvidia" /etc/docker/daemon.json 2>/dev/null; then
        echo -e "${GREEN}✓${NC} Docker configured for NVIDIA runtime"
    else
        echo -e "${YELLOW}!${NC} Docker daemon.json exists but NVIDIA runtime may not be default"
        echo "   Current config:"
        cat /etc/docker/daemon.json | head -5
    fi
else
    echo -e "${YELLOW}!${NC} No /etc/docker/daemon.json found (using defaults)"
fi

# Final status
echo ""
echo "=== Summary ==="
echo -e "${GREEN}✓ GPU Setup Complete!${NC}"
echo ""
echo "Hardware Details:"
nvidia-smi --query-gpu=index,name,memory.total,memory.free,memory.used --format=table
echo ""
echo "Ready to run GPU-accelerated HoneyGPT!"
echo "Next steps:"
echo "  1. Run ./scripts/setup_gpu_honeygpt.sh to pull Qwen2.5 model"
echo "  2. Update docker-compose.yml for GPU passthrough"
echo "  3. Test with: docker-compose up -d"

exit $EXIT_GPU_READY
