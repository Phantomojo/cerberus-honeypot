# 🚀 Installation Guide: The Golden Path

Follow these steps to deploy the CERBERUS Bio-Adaptive Honeynet and the Phoenix Intelligence HUD.

## 1. Prerequisites
- **Docker & Docker Compose**
- **Tailscale** (Recommended for secure remote HUD access)
- **GCP Account** (Optimized for Google Cloud VPS)
- **API Keys**: Shodan, AbuseIPDB, IPInfo.io (See `.env.template`)

## 2. Secure Initialization
Before deploying, secure your intelligence tokens:
1.  **Clone the Repository**:
    ```bash
    git clone https://github.com/Phantomojo/cerberus-honeypot.git
    cd cerberus-honeypot
    ```
2.  **Initialize Secrets**:
    ```bash
    cp .env.template .env
    # Add your keys and a strong ADMIN_PASSWORD to .env
    ```

## 3. Deployment Options

### A. The Tactical Cloud Path (Recommended)
Automatically deploys a GCP VPS, installs Docker, configures Tailscale, and launches the **Phoenix HUD**:
```bash
./scripts/gcp_deploy.sh
```

### B. The Local Laboratory Path (Manual)
1.  **Setup Environment**:
    ```bash
    ./setup.sh --install-deps
    ./setup.sh
    ```
2.  **Launch Honeynet**:
    ```bash
    cd docker
    docker compose up -d
    ```
3.  **Launch HUD**:
    ```bash
    sudo python3 scripts/web_dashboard.py
    ```

## 4. Operational HUD Access
Once deployed, access your Command Center at:
`http://<YOUR_IP>:8080` (or via your Tailscale IP)

-   **Authentication**: Use the `ADMIN_PASSWORD` you set in `.env`.
-   **Mission Control**: Use the **Intelligence Terminal** on the left to track attackers and the **Target Deep-Intel** on the right to trigger AI Threat Reports.

---

## 5. Verification & Security
-   **Service Check**: `docker compose ps`
-   **Intelligence Flux**: `sudo journalctl -u cerberus-dashboard -f`
-   **Firewall Audit**: Ensure port **8080** is only open to your management IP or accessible via Tailscale.

🛡️ **Cerberus is now monitoring for threats.**
