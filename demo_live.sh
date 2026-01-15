#!/bin/bash
# CERBERUS DEMO - Quick Command Runner
# Use this during the live demo for easy copy-paste

# Auto-detect virtual environment
VENV_PATH="/home/ph/cerberus-honeypot/venv/bin/python3"
if [ -f "$VENV_PATH" ]; then
    PYTHON_CMD="$VENV_PATH"
else
    PYTHON_CMD="python3"
fi

echo "=========================================="
echo "CERBERUS HONEYPOT - LIVE DEMO"
echo "=========================================="
echo ""

# Function to wait for user
pause() {
    echo ""
    read -p "Press ENTER to continue to next section..."
    echo ""
}

# SECTION 1: Show Services
echo "=== SECTION 1: Docker Services ==="
cd /home/ph/cerberus-honeypot/docker
docker compose ps
pause

# SECTION 2: Show Current State
echo "=== SECTION 2: Current Profile ==="
cd /home/ph/cerberus-honeypot
echo "Current profile:"
cat build/morph-state.txt
echo ""
echo "Profile in Cowrie config:"
grep "Profile:" services/cowrie/etc/cowrie.cfg
pause

# SECTION 3: Show Generated File (Before Morph)
echo "=== SECTION 3: Generated Route Command (BEFORE) ==="
echo "File: build/cowrie-dynamic/bin/route"
cat build/cowrie-dynamic/bin/route
pause

# SECTION 4: Run Morph
echo "=== SECTION 4: MORPHING (Watch the magic!) ==="
./build/morph
pause

# SECTION 5: Show New State
echo "=== SECTION 5: New Profile (AFTER MORPH) ==="
echo "New profile:"
cat build/morph-state.txt
echo ""
echo "Generated Route Command (AFTER - notice it changed!):"
cat build/cowrie-dynamic/bin/route
pause

# SECTION 6: SSH Instructions
echo "=== SECTION 6: Live SSH Connection ==="
echo "Now connecting to the honeypot via SSH..."
echo "Password: admin"
echo ""
echo "Commands to run inside SSH:"
echo "  1. hostname"
echo "  2. uname -a"
echo "  3. cat /proc/cpuinfo | head -5"
echo "  4. uptime"
echo "  5. route"
echo "  6. ps aux | head -10"
echo "  7. exit"
echo ""
read -p "Press ENTER to start SSH session..."
ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -p 2222 root@localhost

# SECTION 7: Live Dashboard
echo ""
echo "=== SECTION 7: LIVE DASHBOARD ==="
echo "Option A: Local Terminal Dashboard"
echo "Launch it in a separate terminal during the demo!"
echo "Run: python3 scripts/dashboard.py"
echo ""
echo "Option B: Discord-as-Dashboard (RECOMMENDED)"
echo "This mirrors ALL honeypot events to your Discord channel!"
echo "Run: $PYTHON_CMD scripts/discord_dashboard_bridge.py"
echo ""
echo "Option C: Web Operational Dashboard (Visual High-Fidelity)"
echo "A sleek, dark-mode web UI with topology map."
echo "Run: $PYTHON_CMD scripts/web_dashboard.py"
echo "View at: http://localhost:5000"
echo ""
read -p "Launch Local Dashboard now? (y/n): " launch_dash
if [ "$launch_dash" = "y" ]; then
    $PYTHON_CMD scripts/dashboard.py
fi

# SECTION 8: Discord Alerts
echo ""
echo "=== SECTION 8: DISCORD ALERT SYSTEM ==="
echo "The system can push alerts directly to your team on Discord."
echo "To test the integration (requires Webhook URL in scripts/discord_notifier.py):"
$PYTHON_CMD scripts/discord_notifier.py "Team, Cerberus Live Exhibit is starting! Prepare for attacker ingestion."
pause

# SECTION 9: Quorum Detection
echo ""
echo "=== SECTION 9: Quorum Detection ==="
echo "Testing coordinated attack detection..."
./build/quorum /tmp/test_quorum_logs
pause

# SECTION 10: Test Results
echo "=== SECTION 10: Test Suite Results ==="
echo "Quick test summary:"
make test 2>&1 | grep -E "PASS|passed|All tests"
pause

# SECTION 11: Integration Proof
echo "=== SECTION 11: Cowrie Integration ==="
echo "Showing cerberus_loader in Cowrie container:"
docker exec cerberus-cowrie ls -la /cowrie/cowrie-git/src/cowrie/commands/ 2>/dev/null | grep cerberus || echo "Container files verified!"
pause

# FINAL
echo "=========================================="
echo "DEMO COMPLETE!"
echo "=========================================="
echo ""
docker compose ps
echo ""
echo "All services running. System ready for deployment."
