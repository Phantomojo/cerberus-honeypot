#!/bin/bash
# ==============================================================================
# CERBERUS CLOUD HARVESTER
# ==============================================================================
# Syncs logs and session data from GCP node to local cerberus-honeypot.
# Usage: ./scripts/cloud_sync.sh
# ==============================================================================

PROJECT_ID="lofty-feat-469115-r8"
ZONE="us-central1-a"
INSTANCE_NAME="cerberus-command"
REMOTE_PATH="cerberus-honeypot/services/cowrie"

TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
SYNC_TARGET="./captures/cloud_$TIMESTAMP"

echo "[*] Initializing CERBERUS Cloud Harvest..."
mkdir -p "$SYNC_TARGET"

# 1. Fetch JSON Logs (Primary analysis data)
echo "[*] Downloading JSON logs from $INSTANCE_NAME..."
gcloud compute scp "$INSTANCE_NAME:$REMOTE_PATH/logs/cowrie.json" "$SYNC_TARGET/cowrie.json" --zone="$ZONE" --project="$PROJECT_ID" || echo "[!] No JSON logs found"

# 2. Fetch Session Data (Recorded attacker sessions)
echo "[*] Syncing session data (this may take a moment)..."
gcloud compute scp --recurse "$INSTANCE_NAME:$REMOTE_PATH/data/tty/" "$SYNC_TARGET/sessions/" --zone="$ZONE" --project="$PROJECT_ID" || echo "[!] No session data found"

# 3. Fetch Downloaded Malware
echo "[*] Harvesting downloaded artifacts..."
gcloud compute scp --recurse "$INSTANCE_NAME:$REMOTE_PATH/data/downloads/" "$SYNC_TARGET/malware/" --zone="$ZONE" --project="$PROJECT_ID" || echo "[!] No downloads found"

echo ""
echo "[SUCCESS] Cloud Harvest Complete."
echo "[INFO] Data saved to: $SYNC_TARGET"
echo "[INFO] Run './scripts/unified_enrich.py' to analyze the new data."
