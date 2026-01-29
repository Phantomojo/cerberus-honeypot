#!/bin/bash
# ==============================================================================
# CERBERUS DEMO ATTACK SCRIPT
# ==============================================================================
# Automates a series of common attacker behaviors to demo the Phoenix HUD.
# Run this from YOUR LOCAL MACHINE (not the VM).

HOST="cerberus-hunting.duckdns.org"
PORT="2222"
USER="root"
PASS="root"

echo "🛸 CERBERUS Tactical Simulation Engine"
echo "Target: $HOST:$PORT"
echo "---------------------------------------"

# Check if sshpass is installed
if ! command -v sshpass &> /dev/null; then
    echo "❌ Error: 'sshpass' is required for this demo."
    echo "   Install it with: sudo apt install sshpass (Linux) or brew install httpie (Mac)"
    exit 1
fi

echo "[1/4] Starting Reconnaissance (Port Scan Simulation)..."
nmap -Pn -p $PORT $HOST > /dev/null
sleep 2

echo "[2/4] Simulating Brute Force (Failed Attempts)..."
for i in {1..3}; do
    sshpass -p "admin$RANDOM" ssh -o StrictHostKeyChecking=no -o ConnectTimeout=5 $USER@$HOST -p $PORT "whoami" &> /dev/null
    echo "      Attempt $i failed (Expected)"
done
sleep 2

echo "[3/4] Successful Infiltration..."
sshpass -p "$PASS" ssh -o StrictHostKeyChecking=no -o ConnectTimeout=5 $USER@$HOST -p $PORT "whoami; sleep 1; uname -a; sleep 1; ls -la /etc"
sleep 2

echo "[4/4] Exfiltration Simulated..."
sshpass -p "$PASS" ssh -o StrictHostKeyChecking=no -o ConnectTimeout=5 $USER@$HOST -p $PORT "cat /etc/passwd" > /tmp/exfiltrated.txt
rm /tmp/exfiltrated.txt

echo "---------------------------------------"
echo "✅ Simulation Complete! Check the Phoenix HUD for logs and map updates."
echo "URL: http://$HOST:5000 (or http://$HOST if proxy is active)"
