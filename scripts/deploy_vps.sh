#!/bin/bash
# ==============================================================================
# CERBERUS VPS DEPLOYMENT AUTOMATION
# ==============================================================================
# This script prepares a clean Ubuntu/Debian VPS for Cerberus.
# RUN AS ROOT: sudo ./deploy_vps.sh
# ==============================================================================

set -e

echo "🛡️ Starting Cerberus Phase II: VPS Provisioning..."

# 1. Host Hardening (SSH & Firewall)
# ------------------------------------------------------------------------------
HOST_SSH_PORT=22222
HONEYPOT_SSH_PORT=2222

echo "[*] Move Host SSH to port ${HOST_SSH_PORT} for protection..."
sed -i "s/#Port 22/Port ${HOST_SSH_PORT}/g" /etc/ssh/sshd_config
sed -i "s/Port 22/Port ${HOST_SSH_PORT}/g" /etc/ssh/sshd_config || true

echo "[*] Configuring UFW Firewall..."
ufw default deny incoming
ufw default allow outgoing
ufw allow ${HOST_SSH_PORT}/tcp comment "Host SSH"
ufw allow ${HONEYPOT_SSH_PORT}/tcp comment "Cerberus SSH"
ufw allow 5000/tcp comment "Web Command Center"
echo "y" | ufw enable

# 2. Dependencies
# ------------------------------------------------------------------------------
echo "[*] Installing dependencies (Docker, GCC, Python)..."
apt-get update
apt-get install -y git make gcc python3-venv python3-pip apt-transport-https ca-certificates curl software-properties-common

# Install Docker
if ! command -v docker &> /dev/null; then
    curl -fsSL https://get.docker.com -o get-docker.sh
    sh get-docker.sh
fi

# 3. Cerberus Stack Setup
# ------------------------------------------------------------------------------
echo "[*] Preparing Cerberus Directories..."
mkdir -p /opt/cerberus
cd /opt/cerberus

# (Assuming user has cloned or will clone here. This script handles the stack start)
echo "[!] IMPORTANT: Copy the project files to /opt/cerberus before continuing."
echo "[!] Then run: make clean build"
echo "[!] And: docker compose -f docker/docker-compose.yml up -d"

echo "✅ VPS HARDENING COMPLETE."
echo "----------------------------------------------------------------------"
echo "NEW STEPS:"
echo "1. Restart SSH: systemctl restart ssh"
echo "2. Reconnect on Port ${HOST_SSH_PORT}: ssh -p ${HOST_SSH_PORT} root@<IP>"
echo "3. Run the Cerberus Stack."
echo "----------------------------------------------------------------------"
