# 🛡️ CERBERUS Production Setup & Hardening Guide

## 🛸 Quick Setup (GCP Instance)

### 1. Configure Private Secrets (PREVENET LEAKS)
Cerberus is designed to be secure. **NEVER** commit your secrets to git.
1. Create a `.env` file in the root directory:
   ```bash
   cp .env.template .env
   ```
2. Fill in your professional API keys:
   - **IPINFO_TOKEN**: High-precision geolocation and ASN data.
   - **ABUSEIPDB_API_KEY**: Real-time risk scoring for attackers.
   - **SHODAN_API_KEY**: "Counter-Scanning" of the attacker network.
   - **ADMIN_PASSWORD**: Your secret key for the HUD dashboard.

### 2. Deploy Intelligence Core
Deploy the VPS and launch the HUD (handles Tailscale and Docker automatedly):
```bash
./scripts/gcp_deploy.sh
```

---

## 🛸 Phoenix HUD v5.0: Mission Control

### 📊 Tactical Features
- **NASA Satellite Feed**: Integrated global map with deep-recon capabilities.
- **AI Threat Reporting**: Click "Generate AI Report" on any target to see intent analysis.
- **Counter-Scan [Shodan]**: Real-time identification of attacker open ports and OS.
- **Persistent Intelligence**: All attacker commands and sessions are stored in `build/cerberus.db` (SQLite). This data persists across restarts.

### 🔐 Multi-Layer Security
- **Layer 1: Perimeter**: GCP Firewalls allow honeypot ports (2222) while locking admin ports (8080).
- **Layer 2: Zero-Config P2P**: Cerberus uses **Tailscale** for a secure P2P encrypted tunnel. If you are on the Tailscale network, you can access the dashboard via its internal IP without opening port 8080 to the world.
- **Layer 3: Deception Terminal**: The HUD includes a **Secure Proxy Terminal** that only allows interaction with the honeypot container, preventing host VM lateral movement.

---

## 🔄 Management & Services

### Start/Restart HUD
```bash
sudo systemctl restart cerberus-dashboard
```

### Monitor Intelligence Flux
```bash
# Dashboard Logs
sudo journalctl -u cerberus-dashboard -f

# Threat Database Audit
sqlite3 build/cerberus.db "SELECT * FROM activities LIMIT 10;"
```

---

## 🛑 Hardening Recommendations
1. **Rotate Admin Keys**: Periodically change `ADMIN_PASSWORD` in `.env`.
2. **Whitelist Management**: Ensure only your management IPs can access the GCP VM.
3. **Automated Harvest**: Run `./scripts/cloud_sync.sh` weekly to pull cloud malware samples and logs to your local secure workstation.

---

## 📊 Monitoring
Cerberus provides a **Global Threat Flux** dashboard at `http://your-vps-ip:8080`.
Authentication requires the **TACTICAL_KEY** set in your `.env`.

🛡️ **Mission Status: FULLY_OPERATIONAL** 🛸
