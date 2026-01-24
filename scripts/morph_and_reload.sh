#!/bin/bash
# Cerberus Morph & Reload Script
# This script runs the morphing engine and restarts Cowrie to apply changes.

# Exit on error
set -e

# Project root directory
if [ -d "/app" ]; then
    PROJECT_ROOT="/app"
else
    PROJECT_ROOT="/home/ph/cerberus-honeypot"
fi
cd "$PROJECT_ROOT"

# Check if we are in a container (no make needed if bins already built)
if [ ! -f "/.dockerenv" ]; then
    echo "[*] Rebuilding morph engine..."
    make build/morph
    MORPH_BIN="./build/morph"
else
    # In container, use baked-in binaries if available
    if [ -f "/usr/local/bin/morph" ]; then
        MORPH_BIN="/usr/local/bin/morph"
    else
        MORPH_BIN="./build/morph"
    fi
fi

echo "[*] Running morphing engine (selecting a random profile)..."
$MORPH_BIN profiles.conf

echo "[*] Ensuring permissions are correct..."
chmod 644 services/cowrie/etc/cowrie.cfg
chmod 644 services/cowrie/etc/cowrie.env
chmod 644 services/cowrie/etc/userdb.txt
chmod -R 644 build/cowrie-dynamic/bin/* 2>/dev/null || true

echo "[*] Restarting Cowrie container..."
# Try docker-compose first, then fallback to direct docker restart
if command -v docker-compose &> /dev/null; then
    docker-compose -f docker/docker-compose.yml restart cowrie
elif docker compose version &> /dev/null; then
    docker compose -f docker/docker-compose.yml restart cowrie
else
    docker restart cerberus-cowrie
fi

echo "[+] Morph complete! Cowrie is now running with the new profile."
