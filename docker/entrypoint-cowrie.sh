#!/bin/bash
# Cowrie entrypoint - copies dynamic commands before starting

# Copy dynamic commands into container filesystem
if [ -d "/data/cowrie-dynamic" ]; then
    echo "[*] Installing dynamic commands..."
    cp -f /data/cowrie-dynamic/bin/* /cowrie/cowrie-git/share/cowrie/honeyfs/usr/bin/ 2>/dev/null || true
    cp -f /data/cowrie-dynamic/bin/* /cowrie/cowrie-git/share/cowrie/honeyfs/bin/ 2>/dev/null || true
    cp -f /data/cowrie-dynamic/sbin/* /cowrie/cowrie-git/share/cowrie/honeyfs/sbin/ 2>/dev/null || true
    cp -f /data/cowrie-dynamic/usr/bin/* /cowrie/cowrie-git/share/cowrie/honeyfs/usr/bin/ 2>/dev/null || true
fi

# Ensure proper Cowrie config
if [ -f "/etc/cowrie/cowrie.cfg" ]; then
    echo "[*] Cowrie config found"
fi

# Start Cowrie
echo "[*] Starting Cowrie honeypot on port 2222 (SSH) and 2323 (Telnet)..."
exec /cowrie/cowrie-git/bin/cowrie -c /etc/cowrie/cowrie.cfg start
