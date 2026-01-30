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

# --- DECEPTION ENHANCEMENT: Web UI Morphing ---
echo "[*] Synchronizing Web Dashboard with active profile..."
# 1. Identify valid profile from last morph (grep from C output or check cowrie.cfg)
# Since morph.c updates cowrie.cfg, we can't easily know WHICH profile was picked without parsing.
# Simpler approach: Use a python one-liner to read the active profile from the just-generated cowrie.cfg is hard (it only has the banner).
# BETTER: Let's trust that morph.c writes a sidecar file or we parse profiles.conf based on the banner in cowrie.cfg.

# FAST HACK: We will use a python script to match the banner in cowrie.cfg back to profiles.conf and get the HTML path.
python3 -c "
import configparser
import os

# Load Cowrie Config (Active)
cowrie_cfg = configparser.ConfigParser()
cowrie_cfg.read('services/cowrie/etc/cowrie.cfg')
active_banner = cowrie_cfg.get('honeypot', 'ssh_version', fallback='').strip()

# Load Profiles Config (Source)
profiles = configparser.ConfigParser()
profiles.read('profiles.conf')

target_html = 'services/fake-router-web/html/themes/default.html'

for section in profiles.sections():
    if profiles.has_option(section, 'ssh_banner'):
        p_banner = profiles.get(section, 'ssh_banner').strip()
        if p_banner == active_banner:
            target_html = profiles.get(section, 'router_html', fallback=target_html)
            print(f'[MORPH] Matched profile: {section}')
            break

print(f'[MORPH] updating webroot with: {target_html}')
os.system(f'cp {target_html} services/fake-router-web/html/index.html')
"

echo "[*] Ensuring permissions are correct..."
chmod 644 services/cowrie/etc/cowrie.cfg
chmod 644 services/cowrie/etc/cowrie.env
chmod 644 services/cowrie/etc/userdb.txt
chmod -R 644 build/cowrie-dynamic/bin/* 2>/dev/null || true

echo "[*] Restarting Cowrie container..."
# Try docker-compose first, then fallback to direct docker restart
if command -v docker-compose &> /dev/null; then
    docker-compose -f docker/docker-compose.yml restart cowrie
    docker restart cerberus-router-web
elif docker compose version &> /dev/null; then
    docker compose -f docker/docker-compose.yml restart cowrie
    docker restart cerberus-router-web
else
    docker restart cerberus-cowrie
    docker restart cerberus-router-web
fi

echo "[+] Morph complete! Cowrie is now running with the new profile."
