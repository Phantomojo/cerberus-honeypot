# CERBERUS Sprint Tracker - Live Status

**Current Sprint:** Sprint 1 - Quick Wins
**Sprint Duration:** Nov 27 - Dec 11, 2025 (2 weeks)
**Last Updated:** Nov 27, 2025

---

## Sprint 1 Checklist (Quick Wins)

### Task 1: Change Cowrie Port 2222 → 22 ⏱️ 15 minutes

**Description:** Move Cowrie from non-standard port 2222 to standard SSH port 22 to evade detection.

**Files to Modify:**
- [ ] `docker/docker-compose.yml` - Update port mapping
- [ ] `services/cowrie/etc/cowrie.cfg` - Update [ssh] port setting

**Steps:**

```bash
# 1. Edit docker-compose.yml
nano docker/docker-compose.yml
# Change:
#   - "2222:2222" to "22:2222"
# Also change environment:
#   - COWRIE_SSH_PORT=2222 to COWRIE_SSH_PORT=22

# 2. Edit cowrie.cfg
nano services/cowrie/etc/cowrie.cfg
# Under [ssh]:
#   port = 22
#   listen_port = 22
#   address = 0.0.0.0

# 3. Ensure no real SSH running on port 22
sudo systemctl stop ssh
# Or move real SSH to different port:
sudo sed -i 's/#Port 22/Port 2222/' /etc/ssh/sshd_config
sudo systemctl restart ssh

# 4. Restart Cowrie
cd docker
docker compose restart cowrie

# 5. Verify
netstat -tulpn | grep :22
# Should show Cowrie listening on 22
```

**Expected Output:**
```
tcp    0    0 0.0.0.0:22    0.0.0.0:*    LISTEN
```

**Testing:**
```bash
ssh -v localhost
# Should connect to Cowrie on port 22
# Try any username/password, should succeed
```

**Status:** 🟡 **Queued** | **Effort:** 15 min | **Blocker:** None

---

### Task 2: Randomize SSH Version Strings ⏱️ 30 minutes

**Description:** Change default OpenSSH banner from old version to random current versions.

**Current State:**
```
OpenSSH_6.0p1 Debian 4+deb7u2  ❌ From 2014, obvious
```

**Target State:**
```
OpenSSH_7.6p1 Ubuntu-4ubuntu0.3  ✅ Realistic
OpenSSH_8.2p1 Ubuntu-4ubuntu0.5  ✅ Changes per restart
OpenSSH_8.3p1 Debian-1           ✅ Varies per morph cycle
```

**Implementation:**

```bash
# 1. Create version list file
cat > services/cowrie/etc/ssh_versions.txt << 'EOF'
OpenSSH_7.4p1 Debian-10+deb9u7
OpenSSH_7.6p1 Ubuntu-4ubuntu0.3
OpenSSH_8.2p1 Ubuntu-4ubuntu0.5
OpenSSH_8.0p1 Debian-6+deb10u3
OpenSSH_7.3p1 Debian-1+deb9u4
OpenSSH_8.1p1 Debian-1
OpenSSH_7.9p1 Ubuntu-10+deb9u5
OpenSSH_8.3p1 Debian-1
EOF

# 2. Create randomization script in src/morph/
cat > src/morph/randomize_ssh_version.sh << 'EOF'
#!/bin/bash
# Randomly select SSH version and update cowrie config

VERSIONS_FILE="services/cowrie/etc/ssh_versions.txt"
COWRIE_CFG="services/cowrie/etc/cowrie.cfg"

# Pick random version
RANDOM_VERSION=$(shuf -n 1 "$VERSIONS_FILE")

# Update cowrie.cfg
sed -i "s/^banner = SSH-2.0-.*/banner = SSH-2.0-$RANDOM_VERSION/" "$COWRIE_CFG"

echo "SSH version randomized to: $RANDOM_VERSION"
EOF

chmod +x src/morph/randomize_ssh_version.sh

# 3. Add to morph engine call
# In src/morph/morph.c, call this script during morphing

# 4. Manual update for now (quick test)
RANDOM_VERSION=$(shuf -n 1 services/cowrie/etc/ssh_versions.txt)
sed -i "s/^banner = SSH-2.0-.*/banner = SSH-2.0-$RANDOM_VERSION/" services/cowrie/etc/cowrie.cfg

# 5. Restart Cowrie
docker compose restart cowrie

# 6. Verify
ssh -v localhost 2>&1 | head -5
# Should show new SSH version in banner
```

**Testing:**
```bash
# Test multiple times
for i in {1..3}; do
  echo "Test $i:"
  ssh -v localhost 2>&1 | grep OpenSSH
  docker compose restart cowrie
  sleep 2
done
# Each should show different version
```

**Status:** 🟡 **Queued** | **Effort:** 30 min | **Blocker:** Task 1

---

### Task 3: Extend Session Timeout ⏱️ 10 minutes

**Description:** Increase session timeout from 180s to 600s for more engagement.

```bash
# 1. Edit cowrie.cfg
nano services/cowrie/etc/cowrie.cfg

# Find [honeypot] section and change:
# OLD: timeout = 180
# NEW: timeout = 600

# 2. Restart
docker compose restart cowrie

# 3. Test
ssh root@localhost
# Wait 10 minutes, type a command
# Should still work (not timed out)
```

**Benefit:** 3.3x longer average session = more attacker engagement

**Status:** 🟡 **Queued** | **Effort:** 10 min | **Blocker:** Task 1

---

### Task 4: Add 5 Missing Commands ⏱️ 20 minutes

**Description:** Implement docker, systemctl, python, git, curl commands.

```bash
# 1. Create response files in honeyfs
mkdir -p services/cowrie/honeyfs/bin
mkdir -p services/cowrie/honeyfs/usr/bin

# 2. Docker version
cat > services/cowrie/honeyfs/usr/bin/docker << 'EOF'
#!/bin/bash
if [[ "$1" == "--version" ]] || [[ "$1" == "-v" ]]; then
  echo "Docker version 20.10.12, build e91ed57"
else
  echo "Cannot connect to Docker daemon"
fi
EOF

# 3. Systemctl status
cat > services/cowrie/honeyfs/usr/bin/systemctl << 'EOF'
#!/bin/bash
if [[ "$1" == "status" ]]; then
  echo "● ssh.service - OpenSSH server"
  echo "   Loaded: loaded (/etc/systemd/system/ssh.service; enabled)"
  echo "   Active: active (running) since Sun 2025-11-27 10:00:00 UTC"
fi
EOF

# 4. Python version
cat > services/cowrie/honeyfs/usr/bin/python << 'EOF'
#!/bin/bash
echo "Python 3.8.10"
EOF

# 5. Git version
cat > services/cowrie/honeyfs/usr/bin/git << 'EOF'
#!/bin/bash
echo "git version 2.34.1"
EOF

# 6. Curl version
cat > services/cowrie/honeyfs/usr/bin/curl << 'EOF'
#!/bin/bash
echo "curl 7.68.0 (x86_64-pc-linux-gnu) libcurl/7.68.0"
EOF

chmod +x services/cowrie/honeyfs/usr/bin/*

# 7. Restart
docker compose restart cowrie

# 8. Test
ssh root@localhost
$ docker --version
Docker version 20.10.12, build e91ed57
$ systemctl status ssh
...
```

**Status:** 🟡 **Queued** | **Effort:** 20 min | **Blocker:** Task 1

---

### Task 5: Create Device-Profile Filesystems ⏱️ 30 minutes

**Description:** Create router and camera specific filesystem variants.

```bash
# 1. Create device-specific filesystem directories
mkdir -p services/cowrie/honeyfs-router/etc
mkdir -p services/cowrie/honeyfs-camera/etc

# 2. Router profile (embedded Linux)
cat > services/cowrie/honeyfs-router/etc/hostname << 'EOF'
router
EOF

cat > services/cowrie/honeyfs-router/etc/passwd << 'EOF'
root:x:0:0:root:/root:/bin/sh
nobody:x:65534:65534:Nobody:/var:/false
EOF

# 3. Camera profile (more users)
cat > services/cowrie/honeyfs-camera/etc/hostname << 'EOF'
camera
EOF

cat > services/cowrie/honeyfs-camera/etc/passwd << 'EOF'
root:x:0:0:root:/root:/bin/sh
admin:x:1000:1000:admin:/home/admin:/bin/sh
guest:x:1001:1001:guest:/home/guest:/bin/sh
EOF

# 4. Test
ssh root@localhost
$ cat /etc/hostname
# Should show "router" or "camera" depending on device profile

# 5. Verify filesystem was swapped on morph
./build/morph profiles.conf
ssh root@localhost
$ cat /etc/hostname
# Should show new device profile
```

**Status:** 🟡 **Queued** | **Effort:** 30 min | **Blocker:** Task 1

---

### Task 6: Add Random Response Delays ⏱️ 15 minutes

**Description:** Add 100-500ms latency to command responses for realism.

```bash
# 1. Create delay function in Python/C
# In services/cowrie/src/cowrie/commands/base.py

import time
import random

def execute_with_delay(self):
    """Execute command with random delay for realism"""
    delay_ms = random.randint(100, 500)
    time.sleep(delay_ms / 1000.0)
    return self.get_output()

# 2. Or in C if modifying core:
// In src/behavior/behavior.c
#include <unistd.h>

void add_command_delay() {
    int delay_ms = (rand() % 400) + 100;  // 100-500ms
    usleep(delay_ms * 1000);
}

# 3. Test timing
time ssh root@localhost "uname -a"
# Should take 100-600ms, not instant
```

**Expected Output:**
```
real    0m0.234s
user    0m0.012s
sys     0m0.010s
```

**Status:** 🟡 **Queued** | **Effort:** 15 min | **Blocker:** Task 4

---

### Task 7: Setup Attack Monitoring Script ⏱️ 30 minutes

**Description:** Create real-time monitoring for attack detection.

```bash
# 1. Create monitoring script
cat > scripts/monitor-attacks.sh << 'EOF'
#!/bin/bash

LOG_FILE="services/cowrie/logs/cowrie.log"
ALERT_FILE="logs/alerts.log"

tail -f "$LOG_FILE" | grep -E "connection|brute|attempt" | while read line; do
    # Log with timestamp
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] $line" >> "$ALERT_FILE"

    # Alert on detection attempts
    if echo "$line" | grep -qE "docker|systemctl|uname|passwd"; then
        echo "DETECTION ATTEMPT: $line" >> "$ALERT_FILE"
        echo "⚠️  Detection attempt detected at $(date)"
    fi
done
EOF

chmod +x scripts/monitor-attacks.sh

# 2. Create metrics script
cat > scripts/metrics.sh << 'EOF'
#!/bin/bash

LOG_FILE="services/cowrie/logs/cowrie.log"

echo "=== CERBERUS Honeypot Metrics ==="
echo "Total SSH connections: $(grep 'connection from' "$LOG_FILE" | wc -l)"
echo "Failed logins: $(grep 'Failed' "$LOG_FILE" | wc -l)"
echo "Unique IPs: $(grep 'connection from' "$LOG_FILE" | awk '{print $NF}' | sort -u | wc -l)"
echo "Successful logins: $(grep 'Successful login' "$LOG_FILE" | wc -l)"
echo ""
echo "Top attacking IPs:"
grep 'connection from' "$LOG_FILE" | awk '{print $NF}' | sort | uniq -c | sort -rn | head -5
EOF

chmod +x scripts/metrics.sh

# 3. Run monitoring in background
nohup ./scripts/monitor-attacks.sh &

# 4. View metrics
./scripts/metrics.sh
```

**Expected Output:**
```
=== CERBERUS Honeypot Metrics ===
Total SSH connections: 247
Failed logins: 89
Unique IPs: 34
Successful logins: 158

Top attacking IPs:
     45 192.168.1.100
     32 10.0.0.50
     28 172.16.0.25
     ...
```

**Status:** 🟡 **Queued** | **Effort:** 30 min | **Blocker:** Task 1

---

## Sprint Completion Checklist

### Deliverables
- [ ] docker-compose.yml updated with port 22 mapping
- [ ] cowrie.cfg has correct port and timeout settings
- [ ] SSH version randomization script working
- [ ] 5 new commands (docker, systemctl, python, git, curl) implemented
- [ ] Device-profile specific filesystems created
- [ ] Random response delays integrated
- [ ] monitoring script deployed and running
- [ ] metrics script showing attack data

### Testing
- [ ] `ssh localhost` connects on port 22
- [ ] SSH version differs per restart (verify 3x)
- [ ] Sessions last > 10 minutes
- [ ] New commands return expected output
- [ ] Device hostname differs based on profile
- [ ] Commands take 100-600ms to respond
- [ ] Monitoring alerts appear in logs
- [ ] cowrie_detect.py reports LOW confidence

### Documentation
- [ ] Changes committed to git
- [ ] This tracker updated with completion dates
- [ ] SPRINT_COMPLETION_SUMMARY.md written
- [ ] Issues logged for Sprint 2

---

## Time Tracking

| Task | Planned | Actual | Notes |
|------|---------|--------|-------|
| Port change | 15 min | - | |
| SSH versions | 30 min | - | |
| Timeout | 10 min | - | |
| Commands | 20 min | - | |
| Filesystems | 30 min | - | |
| Delays | 15 min | - | |
| Monitoring | 30 min | - | |
| **TOTAL** | **150 min (2.5h)** | - | On track |

---

## Current Blockers

None - ready to start immediately!

---

## Next Sprint Preview

**Sprint 2: Behavioral Randomization (Dec 12 - Dec 26)**
- Response time variability
- Realistic failure modes (permission denied)
- Time-based behavior (cron jobs)
- Protocol-specific tweaks

---

## Notes

- All scripts are idempotent (safe to run multiple times)
- Docker volumes persist data between restarts
- Morphing engine will be updated to call randomization scripts
- Test against `cowrie_detect.py` after each major change

---

**Questions?** Check QUICK_WINS.md for detailed implementation steps.
