#!/bin/bash
# ==============================================================================
# CERBERUS AUTO-UPDATE SCRIPT
# ==============================================================================
# Pulls latest code from GitHub and restarts services automatically

set -e

LOG_FILE="/var/log/cerberus-autoupdate.log"
REPO_DIR="/home/ph/cerberus-honeypot"
BRANCH="fix/codeql-hardening"

log() {
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] $1" | tee -a "$LOG_FILE"
}

log "=== CERBERUS Auto-Update Started ==="

# Navigate to repo
cd "$REPO_DIR" || exit 1

# Fetch latest changes
log "Fetching latest changes from GitHub..."
git fetch origin "$BRANCH"

# Check if updates are available
LOCAL=$(git rev-parse HEAD)
REMOTE=$(git rev-parse "origin/$BRANCH")

if [ "$LOCAL" = "$REMOTE" ]; then
    log "Already up to date. No changes needed."
    exit 0
fi

log "New updates found! Pulling changes..."
git pull origin "$BRANCH"

# Install any new Python dependencies
log "Updating Python dependencies..."
sudo pip3 install -r requirements.txt --quiet

# Rebuild native components if changed
if git diff --name-only "$LOCAL" "$REMOTE" | grep -q -E '\.(c|h)$'; then
    log "C source files changed. Rebuilding..."
    sudo make build
fi

# Restart services
log "Restarting CERBERUS dashboard..."
sudo systemctl restart cerberus-web-dashboard

# Restart Docker containers if docker-compose changed
if git diff --name-only "$LOCAL" "$REMOTE" | grep -q 'docker-compose'; then
    log "Docker config changed. Restarting containers..."
    cd docker
    sudo docker-compose down
    sudo docker-compose up -d
    cd ..
fi

# Verify services are running
sleep 3
if sudo systemctl is-active --quiet cerberus-web-dashboard; then
    log "✓ Dashboard service is running"
else
    log "✗ WARNING: Dashboard service failed to start!"
fi

log "=== Auto-Update Complete ==="
log "Updated from $LOCAL to $REMOTE"
