# 🔄 CERBERUS Auto-Update System

Automatically pulls updates from GitHub and restarts services every 10 minutes.

---

## Setup (Run on GCP Instance)

### 1. Install the Auto-Update System
```bash
cd ~/cerberus-honeypot

# Make script executable
chmod +x scripts/auto_update.sh

# Install systemd service and timer
sudo cp systemd/cerberus-autoupdate.service /etc/systemd/system/
sudo cp systemd/cerberus-autoupdate.timer /etc/systemd/system/

# Enable and start the timer
sudo systemctl daemon-reload
sudo systemctl enable cerberus-autoupdate.timer
sudo systemctl start cerberus-autoupdate.timer
```

### 2. Verify It's Running
```bash
# Check timer status
sudo systemctl status cerberus-autoupdate.timer

# View update log
sudo tail -f /var/log/cerberus-autoupdate.log
```

---

## How It Works

1. **Every 10 minutes**, the timer triggers the auto-update service
2. The script checks GitHub for new commits
3. If updates are found:
   - Pulls latest code
   - Installs new Python dependencies
   - Rebuilds C code (if changed)
   - Restarts dashboard service
   - Restarts Docker containers (if config changed)

---

## Deployment Workflow

### From Your Local Machine:
```bash
# 1. Make your changes locally
vim scripts/web_dashboard.py

# 2. Commit and push
git add .
git commit -m "Added new feature"
git push origin fix/codeql-hardening

# 3. Wait ~10 minutes - GCP instance will auto-update!
```

### Manual Update (Immediate)
If you can't wait 10 minutes:
```bash
# SSH into the instance
gcloud compute ssh cerberus-command --zone=us-central1-a --project=lofty-feat-469115-r8

# Run update manually
sudo /home/ph/cerberus-honeypot/scripts/auto_update.sh
```

---

## Monitoring

### View Update History
```bash
sudo journalctl -u cerberus-autoupdate.service
```

### Check Last Update Time
```bash
sudo systemctl status cerberus-autoupdate.timer
```

### Stop Auto-Updates
```bash
sudo systemctl stop cerberus-autoupdate.timer
sudo systemctl disable cerberus-autoupdate.timer
```

---

## Security Notes

✅ **Safe**: The script only pulls from your GitHub repo (read-only)  
✅ **Validated**: Only updates if new commits exist  
✅ **Logged**: All updates logged to `/var/log/cerberus-autoupdate.log`  
✅ **Rollback**: Use `git reset --hard <commit>` to rollback if needed

---

## Troubleshooting

### Updates Not Pulling
```bash
# Check timer is active
sudo systemctl is-active cerberus-autoupdate.timer

# Check for errors
sudo journalctl -u cerberus-autoupdate.service -n 50
```

### Service Failed to Restart
```bash
# View dashboard logs
sudo journalctl -u cerberus-web-dashboard -n 50

# Manually restart
sudo systemctl restart cerberus-web-dashboard
```
