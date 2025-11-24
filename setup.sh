#!/bin/bash
# CERBERUS Honeynet Setup Script
# This script sets up the environment and installs dependencies

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Detect OS
detect_os() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        OS=$ID
        OS_VERSION=$VERSION_ID
    else
        echo -e "${RED}Could not detect OS. Exiting.${NC}"
        exit 1
    fi
}

# Check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Print status
print_status() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

# Check dependencies
check_dependencies() {
    echo "Checking dependencies..."
    
    local missing=0
    
    if ! command_exists gcc; then
        print_error "GCC not found"
        missing=1
    else
        print_status "GCC found: $(gcc --version | head -n1)"
    fi
    
    if ! command_exists make; then
        print_error "Make not found"
        missing=1
    else
        print_status "Make found"
    fi
    
    if ! command_exists docker; then
        print_error "Docker not found"
        missing=1
    else
        print_status "Docker found: $(docker --version)"
    fi
    
    if command_exists docker; then
        if docker compose version >/dev/null 2>&1; then
            print_status "Docker Compose found: $(docker compose version)"
        else
            print_error "Docker Compose not found"
            missing=1
        fi
    fi
    
    if ! command_exists python3; then
        print_error "Python 3 not found"
        missing=1
    else
        print_status "Python 3 found: $(python3 --version)"
    fi
    
    if ! command_exists nginx; then
        print_warning "nginx not found (optional, can use Docker)"
    else
        print_status "nginx found: $(nginx -v 2>&1 | head -n1)"
    fi
    
    if [ $missing -eq 1 ]; then
        echo ""
        print_warning "Some dependencies are missing. Install them and run this script again."
        echo "See DEPENDENCIES.md for installation instructions."
        return 1
    fi
    
    return 0
}

# Install dependencies based on OS
install_dependencies() {
    detect_os
    
    echo "Detected OS: $OS"
    echo ""
    echo "This script will install system dependencies."
    read -p "Continue? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "Skipping dependency installation."
        return
    fi
    
    case $OS in
        ubuntu|debian)
            echo "Installing dependencies for Debian/Ubuntu..."
            sudo apt update
            sudo apt install -y build-essential python3 python3-pip python3-venv nginx git make
            ;;
        arch|manjaro)
            echo "Installing dependencies for Arch Linux..."
            sudo pacman -Syu --noconfirm
            sudo pacman -S --noconfirm base-devel python python-pip nginx git make
            ;;
        *)
            print_error "Unsupported OS: $OS"
            echo "Please install dependencies manually. See DEPENDENCIES.md"
            return 1
            ;;
    esac
}

# Setup Python virtual environment
setup_python_env() {
    if [ ! -d "venv" ]; then
        echo "Creating Python virtual environment..."
        python3 -m venv venv
        print_status "Virtual environment created"
    else
        print_status "Virtual environment already exists"
    fi
    
    echo "Installing Python dependencies..."
    source venv/bin/activate
    pip install --upgrade pip
    pip install -r requirements.txt
    print_status "Python dependencies installed"
    deactivate
}

# Build C modules
build_c_modules() {
    echo "Building C modules..."
    make clean
    make
    if [ $? -eq 0 ]; then
        print_status "C modules built successfully"
    else
        print_error "Failed to build C modules"
        return 1
    fi
}

# Setup services directories
setup_services() {
    echo "Setting up service directories..."
    
    # Create necessary directories
    mkdir -p services/cowrie/{logs,data,etc}
    mkdir -p services/fake-router-web/{html,conf}
    mkdir -p services/fake-camera-web/{html,conf}
    mkdir -p services/rtsp/{conf,media}
    mkdir -p logs
    mkdir -p build
    
    print_status "Service directories created"
}

# Main setup function
main() {
    echo "=========================================="
    echo "  CERBERUS Honeynet Setup Script"
    echo "=========================================="
    echo ""
    
    # Check if we should install dependencies
    if [ "$1" == "--install-deps" ]; then
        install_dependencies
        echo ""
    fi
    
    # Check dependencies
    if ! check_dependencies; then
        echo ""
        echo "Run with --install-deps to install missing dependencies:"
        echo "  ./setup.sh --install-deps"
        exit 1
    fi
    
    echo ""
    
    # Setup Python environment
    setup_python_env
    echo ""
    
    # Setup services
    setup_services
    echo ""
    
    # Build C modules
    build_c_modules
    echo ""
    
    echo "=========================================="
    print_status "Setup complete!"
    echo "=========================================="
    echo ""
    echo "Next steps:"
    echo "  1. Review and configure services in docker/docker-compose.yml"
    echo "  2. Start services: cd docker && docker compose up -d"
    echo "  3. Run morphing engine: ./build/morph"
    echo "  4. Run quorum engine: ./build/quorum"
    echo ""
}

# Run main function
main "$@"

