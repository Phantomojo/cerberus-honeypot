#!/bin/bash
# Cerberus Adaptive Morphing Watchdog
# This script monitors for threat signals and implements jittered rotation.

if [ -d "/app" ]; then
    PROJECT_ROOT="/app"
else
    PROJECT_ROOT="/home/ph/cerberus-honeypot"
fi
cd "$PROJECT_ROOT"

# Config
QUORUM_INTERVAL=60 # Check quorum every 60 seconds
MIN_ROTATION_MINUTES=120 # 2 hours
MAX_ROTATION_MINUTES=360 # 6 hours

STATE_FILE="build/watchdog_state.conf"
SIGNAL_FILE="build/signals/emergency_morph.signal"

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [WATCHDOG] $1"
}

set_next_rotation() {
    local jitter=$(( RANDOM % (MAX_ROTATION_MINUTES - MIN_ROTATION_MINUTES + 1) ))
    local total_minutes=$(( MIN_ROTATION_MINUTES + jitter ))
    local next_time=$(date -d "+$total_minutes minutes" +%s)
    echo "NEXT_ROTATION=$next_time" > "$STATE_FILE"
    log "Next scheduled rotation in $total_minutes minutes (at $(date -d @$next_time))"
}

# Initial state
if [ ! -f "$STATE_FILE" ]; then
    set_next_rotation
fi

log "Starting Cerberus Adaptive Watchdog..."

# Detect Quorum binary
if [ -f "/usr/local/bin/quorum" ]; then
    QUORUM_BIN="/usr/local/bin/quorum"
else
    QUORUM_BIN="./build/quorum"
fi

while true; do
    # 1. Run Quorum scan
    $QUORUM_BIN > /dev/null 2>&1

    # 2. Check for emergency signals
    if [ -f "$SIGNAL_FILE" ]; then
        log "!!! EMERGENCY SIGNAL DETECTED !!!"
        log "Triggering immediate identity rotation due to coordinated attack."
        ./scripts/morph_and_reload.sh
        rm -f "$SIGNAL_FILE"
        set_next_rotation # Reset the scheduled timer
    else
        # 3. Check for scheduled rotation
        source "$STATE_FILE"
        current_time=$(date +%s)

        if [ "$current_time" -ge "$NEXT_ROTATION" ]; then
            log "Scheduled rotation interval reached."
            ./scripts/morph_and_reload.sh
            set_next_rotation
        fi
    fi

    sleep "$QUORUM_INTERVAL"
done
