#!/bin/bash
# Install development tools for CERBERUS honeypot
# This script installs optional tools for enhanced testing and code quality

set -e

echo "╔══════════════════════════════════════════════════════════════════════╗"
echo "║  CERBERUS Honeypot - Development Tools Installer                     ║"
echo "╚══════════════════════════════════════════════════════════════════════╝"
echo ""

# Detect OS
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
else
    echo "❌ Cannot detect OS. /etc/os-release not found."
    exit 1
fi

echo "Detected OS: $OS"
echo ""

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to install packages based on OS
install_package() {
    local package=$1
    echo "Installing $package..."

    case $OS in
        ubuntu|debian)
            sudo apt-get update -qq
            sudo apt-get install -y "$package"
            ;;
        fedora|centos|rhel)
            sudo dnf install -y "$package" || sudo yum install -y "$package"
            ;;
        arch|manjaro)
            sudo pacman -S --noconfirm "$package"
            ;;
        *)
            echo "⚠️  Unsupported OS: $OS"
            echo "   Please install $package manually"
            return 1
            ;;
    esac

    return 0
}

# Check and install cppcheck
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Checking for cppcheck (static analysis tool)..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if command_exists cppcheck; then
    VERSION=$(cppcheck --version 2>&1 | head -n1)
    echo "✅ cppcheck is already installed: $VERSION"
else
    echo "⊘ cppcheck not found. Installing..."
    if install_package cppcheck; then
        VERSION=$(cppcheck --version 2>&1 | head -n1)
        echo "✅ cppcheck installed successfully: $VERSION"
    else
        echo "❌ Failed to install cppcheck"
    fi
fi

echo ""

# Check and install valgrind
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Checking for valgrind (memory analysis tool)..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if command_exists valgrind; then
    VERSION=$(valgrind --version 2>&1)
    echo "✅ valgrind is already installed: $VERSION"
else
    echo "⊘ valgrind not found. Installing..."
    if install_package valgrind; then
        VERSION=$(valgrind --version 2>&1)
        echo "✅ valgrind installed successfully: $VERSION"
    else
        echo "❌ Failed to install valgrind"
    fi
fi

echo ""

# Check and install clang-tidy
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Checking for clang-tidy (linting tool)..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if command_exists clang-tidy; then
    VERSION=$(clang-tidy --version 2>&1 | head -n1)
    echo "✅ clang-tidy is already installed: $VERSION"
else
    echo "⊘ clang-tidy not found. Installing..."
    PACKAGE="clang-tidy"
    case $OS in
        ubuntu|debian)
            PACKAGE="clang-tidy"
            ;;
        fedora|centos|rhel)
            PACKAGE="clang-tools-extra"
            ;;
    esac

    if install_package "$PACKAGE"; then
        VERSION=$(clang-tidy --version 2>&1 | head -n1)
        echo "✅ clang-tidy installed successfully: $VERSION"
    else
        echo "⚠️  Could not install clang-tidy (optional)"
    fi
fi

echo ""

# Check OpenSSL development libraries
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Checking for OpenSSL development libraries..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

if pkg-config --exists libssl; then
    VERSION=$(pkg-config --modversion libssl)
    echo "✅ OpenSSL development libraries are installed: $VERSION"
else
    echo "⊘ OpenSSL development libraries not found. Installing..."
    PACKAGE="libssl-dev"
    case $OS in
        ubuntu|debian)
            PACKAGE="libssl-dev"
            ;;
        fedora|centos|rhel)
            PACKAGE="openssl-devel"
            ;;
        arch|manjaro)
            PACKAGE="openssl"
            ;;
    esac

    if install_package "$PACKAGE"; then
        VERSION=$(pkg-config --modversion libssl 2>/dev/null || echo "installed")
        echo "✅ OpenSSL development libraries installed: $VERSION"
    else
        echo "❌ Failed to install OpenSSL development libraries"
    fi
fi

echo ""

# Summary
echo "╔══════════════════════════════════════════════════════════════════════╗"
echo "║                      INSTALLATION SUMMARY                            ║"
echo "╚══════════════════════════════════════════════════════════════════════╝"
echo ""

TOOLS=("gcc" "make" "docker" "valgrind" "cppcheck" "clang-tidy")
ALL_OK=true

for tool in "${TOOLS[@]}"; do
    if command_exists "$tool"; then
        echo "✅ $tool"
    else
        echo "❌ $tool (not installed)"
        if [ "$tool" != "clang-tidy" ]; then
            ALL_OK=false
        fi
    fi
done

# Check OpenSSL
if pkg-config --exists libssl; then
    echo "✅ OpenSSL dev libraries"
else
    echo "❌ OpenSSL dev libraries (not installed)"
    ALL_OK=false
fi

echo ""

if [ "$ALL_OK" = true ]; then
    echo "╔══════════════════════════════════════════════════════════════════════╗"
    echo "║                                                                      ║"
    echo "║           ✅ ALL REQUIRED TOOLS INSTALLED SUCCESSFULLY ✅            ║"
    echo "║                                                                      ║"
    echo "╚══════════════════════════════════════════════════════════════════════╝"
    echo ""
    echo "You can now run:"
    echo "  make clean && make           # Rebuild with updated libraries"
    echo "  bash tests/run_all_tests.sh  # Run full test suite"
    echo ""
    exit 0
else
    echo "╔══════════════════════════════════════════════════════════════════════╗"
    echo "║                                                                      ║"
    echo "║              ⚠️  SOME TOOLS ARE STILL MISSING ⚠️                     ║"
    echo "║                                                                      ║"
    echo "╚══════════════════════════════════════════════════════════════════════╝"
    echo ""
    echo "Some tools could not be installed automatically."
    echo "Please install them manually for full functionality."
    echo ""
    exit 1
fi
