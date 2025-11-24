# CERBERUS Systemd Service Files

This directory contains systemd service and timer files for automated operation of the CERBERUS honeypot.

## Services

### cerberus-morph.service
Runs the morphing engine to rotate device profiles.

### cerberus-morph.timer
Schedules morphing every 6 hours with random delay (unpredictable timing).

### cerberus-quorum.service
Runs the quorum detection engine to identify coordinated attacks.

### cerberus-quorum.timer
Schedules quorum detection every 15 minutes.

## Installation

1. **Copy files to systemd directory:**
```bash
sudo cp systemd/*.service /etc/systemd/system/
sudo cp systemd/*.timer /etc/systemd/system/
```

2. **Create cerberus user (recommended for security):**
```bash
sudo useradd -r -s /bin/false cerberus
sudo chown -R cerberus:cerberus /opt/cerberus-honeypot
```

3. **Reload systemd:**
```bash
sudo systemctl daemon-reload
```

4. **Enable and start timers:**
```bash
# Enable morphing (runs every 6 hours)
sudo systemctl enable cerberus-morph.timer
sudo systemctl start cerberus-morph.timer

# Enable quorum detection (runs every 15 minutes)
sudo systemctl enable cerberus-quorum.timer
sudo systemctl start cerberus-quorum.timer
```

5. **Check status:**
```bash
# View timer status
sudo systemctl list-timers cerberus-*

# View service logs
sudo journalctl -u cerberus-morph.service -f
sudo journalctl -u cerberus-quorum.service -f
```

## Manual Execution

To run services manually:
```bash
sudo systemctl start cerberus-morph.service
sudo systemctl start cerberus-quorum.service
```

## Security Features

- Runs as unprivileged `cerberus` user
- Restricted filesystem access (ReadWritePaths)
- CPU and memory limits
- Private /tmp directory
- No new privileges allowed

## Customization

Edit the service files to adjust:
- **Timing**: Modify `OnCalendar` in .timer files
- **Resource limits**: Adjust CPUQuota and MemoryLimit
- **Paths**: Change WorkingDirectory if installed elsewhere
- **Randomization**: Modify RandomizedDelaySec for morphing unpredictability

## Notes

- The morphing timer includes RandomizedDelaySec=30min to avoid predictable patterns
- Services are marked as `oneshot` and run to completion
- Logs are sent to systemd journal for centralized logging
- Failed executions will be logged for debugging
