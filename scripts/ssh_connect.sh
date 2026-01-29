#!/bin/bash
# CERBERUS Dev SSH Connect
# Connects to the honeypot while ignoring host key changes (dynamic identities).

PORT=${1:-2222}
USER=${2:-root}
HOST=${3:-localhost}

echo "[*] Connecting to CERBERUS Honeypot on port $PORT..."
echo "[*] Ignoring host key verification (safe for local dev testing)"

ssh -o StrictHostKeyChecking=no \
    -o UserKnownHostsFile=/dev/null \
    -p "$PORT" \
    "$USER@$HOST"
