#!/bin/bash
# Verify all quick wins have been implemented correctly

echo "================================================"
echo "CERBERUS Quick Wins Verification Script"
echo "================================================"
echo ""

PASS=0
FAIL=0

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

check_file_exists() {
    local file=$1
    local desc=$2
    if [ -f "$file" ]; then
        echo -e "${GREEN}✓${NC} $desc"
        ((PASS++))
    else
        echo -e "${RED}✗${NC} $desc (file not found: $file)"
        ((FAIL++))
    fi
}

check_file_contains() {
    local file=$1
    local pattern=$2
    local desc=$3
    if grep -qF "$pattern" "$file" 2>/dev/null; then
        echo -e "${GREEN}✓${NC} $desc"
        ((PASS++))
    else
        echo -e "${RED}✗${NC} $desc"
        ((FAIL++))
    fi
}

check_dir_exists() {
    local dir=$1
    local desc=$2
    if [ -d "$dir" ]; then
        echo -e "${GREEN}✓${NC} $desc"
        ((PASS++))
    else
        echo -e "${RED}✗${NC} $desc (directory not found: $dir)"
        ((FAIL++))
    fi
}

# QW #1: Port Change
echo "QW #1: Port Change (2222 → 22)"
check_file_contains "docker/docker-compose.yml" "22:2222" "Docker port mapping uses port 22"
echo ""

# QW #2: SSH Version
echo "QW #2: Modern SSH Versions"
check_file_contains "src/morph/morph.c" "SSH-2.0-OpenSSH_7" "SSH version updated in profiles"
if grep -q "OpenSSH_7" services/cowrie/etc/cowrie.cfg 2>/dev/null; then
    echo -e "${GREEN}✓${NC} Cowrie config has modern SSH version"
    ((PASS++))
else
    echo -e "${YELLOW}⚠${NC} Cowrie config may need morph run (expected, run ./build/morph)"
fi
echo ""

# QW #3: Session Timeout
echo "QW #3: Extended Session Timeout"
check_file_contains "src/morph/morph.c" "timeout = 600" "Morph engine configured for 600s timeout"
check_file_contains "src/morph/morph.c" "login_attempt_limit = 10" "Login attempt limit configured"
echo ""

# QW #4: Dynamic Commands
echo "QW #4: Realistic Commands"
check_dir_exists "build/cowrie-dynamic/bin" "Dynamic commands bin directory exists"
check_dir_exists "build/cowrie-dynamic/usr/bin" "Dynamic commands usr/bin directory exists"

if [ -d "build/cowrie-dynamic/bin" ]; then
    count=$(ls build/cowrie-dynamic/bin/ 2>/dev/null | wc -l)
    if [ "$count" -gt 3 ]; then
        echo -e "${GREEN}✓${NC} Commands in build/cowrie-dynamic/bin/ ($count files)"
        ((PASS++))
    else
        echo -e "${YELLOW}⚠${NC} Few commands in build/cowrie-dynamic/bin/ (run: ./scripts/add_dynamic_commands.sh)"
    fi
fi

check_file_exists "scripts/add_dynamic_commands.sh" "Command generation script exists"
echo ""

# QW #5: Device Filesystems
echo "QW #5: Device-Specific Filesystems"
check_dir_exists "services/cowrie/honeyfs-profiles/router/etc" "Router profile filesystem"
check_dir_exists "services/cowrie/honeyfs-profiles/camera/etc" "Camera profile filesystem"
check_file_exists "services/cowrie/honeyfs-profiles/router/etc/hostname" "Router hostname file"
check_file_exists "services/cowrie/honeyfs-profiles/camera/etc/hostname" "Camera hostname file"
check_file_exists "services/cowrie/honeyfs-profiles/router/etc/passwd" "Router passwd file"
check_file_exists "services/cowrie/honeyfs-profiles/camera/etc/passwd" "Camera passwd file"
echo ""

# QW #6: Integration
echo "QW #6: Automated Integration"
check_file_contains "src/morph/morph.c" "setup_device_filesystem" "Morph engine has filesystem setup function"
check_file_contains "docker/docker-compose.yml" "cowrie-dynamic" "Docker compose mounts dynamic commands"
check_file_contains "docker/docker-compose.yml" "cp -f /data/cowrie-dynamic" "Docker entrypoint injects commands"
check_file_contains "Makefile" "add_dynamic_commands.sh" "Makefile calls command generation script"
echo ""

# Build Status
echo "Build & Compilation"
if [ -f "build/morph" ]; then
    echo -e "${GREEN}✓${NC} Morph binary exists ($(du -h build/morph | cut -f1))"
    ((PASS++))
else
    echo -e "${RED}✗${NC} Morph binary not found (run: make)"
    ((FAIL++))
fi

if [ -f "build/quorum" ]; then
    echo -e "${GREEN}✓${NC} Quorum binary exists ($(du -h build/quorum | cut -f1))"
    ((PASS++))
else
    echo -e "${RED}✗${NC} Quorum binary not found (run: make)"
    ((FAIL++))
fi
echo ""

# Documentation
echo "Documentation"
check_file_exists "QUICK_WINS_COMPLETED.md" "Quick wins completion summary"
check_file_exists "DEPLOYMENT_GUIDE.md" "Deployment guide"
check_file_exists "docker/entrypoint-cowrie.sh" "Cowrie entrypoint script (reference)"
echo ""

# Summary
echo "================================================"
echo "Summary: ${GREEN}$PASS Passed${NC}, ${RED}$FAIL Failed${NC}"
echo "================================================"

if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}✓ All Quick Wins Implemented!${NC}"
    echo ""
    echo "Next steps:"
    echo "  1. make clean && make  (builds everything)"
    echo "  2. ./build/morph       (initialize morphing)"
    echo "  3. docker compose build"
    echo "  4. docker compose up -d cowrie"
    echo "  5. ssh localhost       (test connection on port 22)"
    exit 0
else
    echo -e "${RED}✗ Some checks failed. See above for details.${NC}"
    exit 1
fi
