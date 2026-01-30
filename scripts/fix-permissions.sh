#!/bin/bash
# Fix file permissions for CERBERUS honeypot test execution
# This script ensures all test-related directories are writable by the current user

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "╔══════════════════════════════════════════════════════════════════════╗"
echo "║  CERBERUS Honeypot - Permission Fixer                                ║"
echo "╚══════════════════════════════════════════════════════════════════════╝"
echo ""

# Function to fix directory permissions
fix_dir_permissions() {
    local dir="$1"

    if [ ! -d "$dir" ]; then
        echo "⊘ Directory does not exist: $dir"
        return 0
    fi

    # Check if we need sudo
    if [ ! -w "$dir" ]; then
        if [ "$(stat -c '%U' "$dir" 2>/dev/null || stat -f '%Su' "$dir" 2>/dev/null)" = "root" ]; then
            echo "🔐 Fixing permissions for: $dir (requires sudo)"
            sudo chown -R "$(whoami)":"$(id -gn)" "$dir" 2>/dev/null || {
                echo "⚠️  Failed to change ownership for $dir"
                echo "    Run: sudo chown -R $(whoami):$(id -gn) $dir"
                return 1
            }
            sudo chmod -R u+rwX "$dir" 2>/dev/null || {
                echo "⚠️  Failed to change permissions for $dir"
                return 1
            }
        else
            echo "✓ Fixing permissions for: $dir"
            chmod -R u+rwX "$dir" 2>/dev/null || {
                echo "⚠️  Failed to change permissions for $dir"
                return 1
            }
        fi
    else
        echo "✓ Already writable: $dir"
    fi

    return 0
}

cd "$PROJECT_ROOT"

echo "Fixing log directory permissions..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Fix service log directories
fix_dir_permissions "services/cowrie/logs"
fix_dir_permissions "services/fake-camera-web/logs"
fix_dir_permissions "services/fake-router-web/logs"
fix_dir_permissions "services/rtsp/logs"

# Fix service data directories
fix_dir_permissions "services/cowrie/data"
fix_dir_permissions "services/cowrie/etc"
fix_dir_permissions "services/cowrie/honeyfs"

# Fix build directory
fix_dir_permissions "build"

# Fix root-owned .gitkeep files if any
echo ""
echo "Fixing .gitkeep file permissions..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

find services -name ".gitkeep" -type f | while read -r file; do
    if [ ! -w "$file" ]; then
        if [ "$(stat -c '%U' "$file" 2>/dev/null || stat -f '%Su' "$file" 2>/dev/null)" = "root" ]; then
            echo "🔐 Fixing: $file"
            sudo chown "$(whoami)":"$(id -gn)" "$file" 2>/dev/null || {
                echo "⚠️  Failed to fix $file"
            }
        fi
    fi
done

echo ""
echo "Creating test directories if missing..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Ensure test directories exist
mkdir -p services/cowrie/logs 2>/dev/null || true
mkdir -p services/fake-camera-web/logs 2>/dev/null || true
mkdir -p services/fake-router-web/logs 2>/dev/null || true
mkdir -p services/rtsp/logs 2>/dev/null || true
mkdir -p build 2>/dev/null || true
mkdir -p test-results 2>/dev/null || true

echo "✓ Test directories verified"

echo ""
echo "Checking write permissions..."
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# Test write permissions
DIRS_TO_TEST=(
    "services/cowrie/logs"
    "services/fake-camera-web/logs"
    "services/fake-router-web/logs"
    "services/rtsp/logs"
    "build"
)

ALL_OK=true
for dir in "${DIRS_TO_TEST[@]}"; do
    if [ -d "$dir" ] && [ -w "$dir" ]; then
        echo "✅ $dir - writable"
    else
        echo "❌ $dir - NOT writable"
        ALL_OK=false
    fi
done

echo ""
if [ "$ALL_OK" = true ]; then
    echo "╔══════════════════════════════════════════════════════════════════════╗"
    echo "║                                                                      ║"
    echo "║              ✅ ALL PERMISSIONS FIXED SUCCESSFULLY ✅                ║"
    echo "║                                                                      ║"
    echo "╚══════════════════════════════════════════════════════════════════════╝"
    exit 0
else
    echo "╔══════════════════════════════════════════════════════════════════════╗"
    echo "║                                                                      ║"
    echo "║              ⚠️  SOME PERMISSIONS COULD NOT BE FIXED ⚠️              ║"
    echo "║                                                                      ║"
    echo "╚══════════════════════════════════════════════════════════════════════╝"
    echo ""
    echo "Try running manually:"
    echo "  sudo chown -R \$(whoami):\$(id -gn) services/ build/"
    echo ""
    exit 1
fi
