# 🛡️ CERBERUS Production Setup & Security Guide

## Quick Setup (Run on GCP Instance)

### 1. Install Python Dependencies
```bash
cd ~/cerberus-honeypot
sudo pip3 install -r requirements.txt
```

### 2. Build Native Components
```bash
sudo make build
```

### 3. Start the Dashboard (Manual Test)
```bash
sudo python3 scripts/web_dashboard.py &
```

Test it: http://35.209.99.106:5000 (Password: `CERBERUS_THREAT_OMEGA_99X`)

---

## 🔄 Auto-Restart Setup (Systemd)

### Install Systemd Service
This keeps the dashboard running automatically and restarts it if it crashes:

```bash
cd ~/cerberus-honeypot
sudo cp systemd/cerberus-web-dashboard.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable cerberus-web-dashboard
sudo systemctl start cerberus-web-dashboard
```

### Check Status
```bash
sudo systemctl status cerberus-web-dashboard
```

### View Logs
```bash
sudo journalctl -u cerberus-web-dashboard -f
```

---

## 🔐 Security Architecture

### Three-Layer Security Model

#### Layer 1: GCP Firewall (Network Perimeter)
**Current Rules:**
- Port 2222 (SSH Honeypot) → **Open to World** ✅
- Port 5000 (Admin Dashboard) → **Open to World** ⚠️

**Recommended: Lock Down Dashboard**
```bash
# Get your current IP
MY_IP=$(curl -s ifconfig.me)

# Update firewall to only allow YOUR IP to access dashboard
gcloud compute firewall-rules update cerberus-ingress \
  --project=lofty-feat-469115-r8 \
  --source-ranges=0.0.0.0/0 \
  --allow=tcp:2222

# Create separate rule for admin access
gcloud compute firewall-rules create cerberus-admin \
  --project=lofty-feat-469115-r8 \
  --source-ranges=$MY_IP/32 \
  --allow=tcp:5000 \
  --target-tags=cerberus-node
```

#### Layer 2: Application Authentication
The dashboard has **Admin Shield** built-in:
- Password: `CERBERUS_THREAT_OMEGA_99X`
- SHA-256 hashed
- All API endpoints protected

**To Change Password:**
Edit `scripts/web_dashboard.py` line 21:
```python
ADMIN_PASSWORD = "YOUR_NEW_ULTRA_SECRET_PASSWORD"
```

#### Layer 3: Honeypot Isolation
The honeypot runs in **Docker containers**, isolated from the host:
- Attackers interact with Cowrie (fake SSH)
- No access to real system
- All data logged and analyzed

---

## 🚀 Start All Services (Production Mode)

```bash
cd ~/cerberus-honeypot
sudo docker-compose -f docker/docker-compose.yml up -d
sudo systemctl start cerberus-web-dashboard
```

### Check Everything is Running
```bash
# Docker containers
sudo docker ps

# Dashboard service
sudo systemctl status cerberus-web-dashboard

# Firewall rules
gcloud compute firewall-rules list --project=lofty-feat-469115-r8
```

---

## 📊 Monitoring & Maintenance

### Daily Checks
```bash
# Dashboard logs
sudo journalctl -u cerberus-web-dashboard --since today

# Honeypot logs
sudo docker logs cerberus-cowrie --tail 50

# System resources
htop
```

### Weekly Tasks
- Review captured credentials at http://35.209.99.106:5000
- Update CERBERUS: `git pull origin fix/codeql-hardening`
- Apply security updates: `sudo apt update && sudo apt upgrade`

---

## 🛑 Emergency Commands

### Stop Everything
```bash
sudo systemctl stop cerberus-web-dashboard
sudo docker-compose -f docker/docker-compose.yml down
```

### Restart Dashboard
```bash
sudo systemctl restart cerberus-web-dashboard
```

### Delete the Instance (Clean Slate)
```bash
gcloud compute instances delete cerberus-command \
  --zone=us-central1-a \
  --project=lofty-feat-469115-r8
```
