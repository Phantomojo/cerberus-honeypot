#!/bin/bash
# =============================================================================
# cloud_self_heal.sh - CERBERUS GUARDIAN
# =============================================================================
# Purpose: Prevents "too much isolation" by automatically detecting and fixing
#          service crashes, firewall lockouts, and network issues on the Cloud Node.
# Run via cron: * * * * * /root/cerberus-honeypot/scripts/cloud_self_heal.sh

LOG_FILE="/var/log/cerberus_guardian.log"
COWRIE_CONTAINER="cerberus-cowrie"
MGMT_PORT=22222
HONEYPOT_PORT=2222

log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') [GUARDIAN] $1" >> "$LOG_FILE"
}

# 1. CHECK FIREWALL (Prevent Lockout)
# Ensure management port is ALWAYS allowed
if ! sudo ufw status | grep -q "$MGMT_PORT.*ALLOW"; then
    log "CRITICAL: Management port $MGMT_PORT is NOT allowed! Fixing..."
    sudo ufw allow $MGMT_PORT/tcp
    sudo ufw reload
fi

# 2. CHECK DOCKER NETWORK
# Ensure the bridge network exists
if ! sudo docker network ls | grep -q "cerberus-cloud-net"; then
    log "ERROR: Cerberus network missing. Recreating..."
    cd /home/ph/cerberus-honeypot && sudo docker-compose -f docker/docker-compose.cloud.yml up -d
    exit 0
fi

# 3. CHECK COWRIE CONTAINER
# Is it running?
if ! sudo docker ps | grep -q "$COWRIE_CONTAINER"; then
    log "ERROR: Cowrie container is DEAD. Restarting..."
    sudo docker restart "$COWRIE_CONTAINER"
    sleep 10
fi

# 4. CHECK HONEYPOT LISTENER
# Is port 2222 actually listening?
if ! sudo netstat -tulpn | grep -q ":$HONEYPOT_PORT "; then
    log "ERROR: Port $HONEYPOT_PORT is closed despite container running. Hard restarting stack..."
    cd /home/ph/cerberus-honeypot && sudo docker-compose -f docker/docker-compose.cloud.yml restart cowrie
fi

# 5. AUTO-PRUNE (Prevent Disk Fill-up)
# Remove unused images every 24h (simple check based on hour)
if [ "$(date +%H)" == "04" ] && [ "$(date +%M)" == "00" ]; then
    log "MAINTENANCE: Pruning Docker system..."
    sudo docker system prune -af --volumes
fi
