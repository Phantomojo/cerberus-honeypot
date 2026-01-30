# CERBERUS: Bio-Adaptive IoT Honeynet — Tactical Edition (v5.0) 🛸 🛡️

**CERBERUS** is a high-fidelity, bio-adaptive IoT honeynet designed to deceive advanced adversaries by emulating a dynamic fleet of smart-home devices (Routers, CCTV, Medical Nodes).

It features real-time attack correlation via **Quorum Sensing**, autonomous identity acquisition via **Shodan**, and the professional-grade **Phoenix Tactical HUD** for mission control.

---

## 🚀 Key Features

### 1. Bio-Adaptive Deception
- **Identity Morphing**: Rotates device fingerprints, HTML themes, and service banners on-the-fly to disrupt attacker reconnaissance.
- **Deep Hardware Emulation**: Advanced Cowrie personality engine that mirrors real-world IoT kernel signatures.

### 2. Phoenix Tactical HUD (v5.0)
- **NASA Intelligence Maps**: Global visualization of inbound threats using satellite and high-res street-level imagery.
- **Counter-Scan [Shodan]**: Automatically "scans back" attackers to reveal their open ports, detected OS, and hostnames.
- **AI Threat Intel**: On-demand AI-generated reports that analyze attacker behavior and intent.
- **Deep Activity Log**: Persistent SQLite database recording every command typed by every attacker.

### 3. Autonamous Intelligence
- **Quorum Sensing**: Correlates events across the honeynet. If an attacker touches the Router and the Camera, the system flags a "Hostage Command" or coordinated probe.
- **Geo-Precision**: Integrated **IPInfo.io** and **AbuseIPDB** for pinpoint location accuracy and real-time reputation scoring.

---

## 📂 Project Structure
```
honeynet/
├── scripts/           # Core logic: HUD, AI Enrichment, GCP Deploy
├── src/               # Deception Engines (C)
├── services/          # Honeypot Personalities (Cowrie, Router, Camera)
├── docker/            # Containerization (Compose, Dockerfiles)
├── captures/          # Harvested logs and malware artifacts
├── build/             # Persistent DB (cerberus.db) and Binaries
└── .env               # [SECURE] Your API Keys (Shodan, IPInfo, etc.)
```

---

## 🛠️ Quick Start (Cloud Deployment)

### 1. Preparation
Clone the repo and install local dependencies:
```bash
git clone https://github.com/Phantomojo/cerberus-honeypot.git
cd cerberus-honeypot
./setup.sh --install-deps
```

### 2. Configure Secrets
Copy the template and add your API keys:
```bash
cp .env.template .env
# Edit .env with your Shodan, AbuseIPDB, and IPInfo keys
```

### 3. Deploy to GCP
Cerberus is optimized for Google Cloud. Run the automated deployment:
```bash
./scripts/gcp_deploy.sh
```
This will set up the VPS, install Docker, configure Tailscale, and launch the **Phoenix HUD** at `http://YOUR_INSTANCE_IP:8080`.

---

## 📚 Detailed Guides
- **[PRODUCTION_SETUP.md](./PRODUCTION_SETUP.md)**: Hardening and deploying for live internet capturing.
- **[ATTACK_SIMULATION.md](./ATTACK_SIMULATION.md)**: How to test the HUD using the provided stress-attack scripts.
- **[TECHNICAL_SPEC.md](./TECHNICAL_SPEC.md)**: Deep dive into the C-based Morphing and Quorum engines.

---

## ⚖️ License
CERBERUS is licensed for educational and research purposes. Use responsibly. 🛡️
