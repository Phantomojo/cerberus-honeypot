# 🦅 GCP Setup Guide for CERBERUS

If you haven't used Google Cloud before, follow these steps to get your CLI (Command Line Interface) ready for the deployment script.

---

### 1. Install the Google Cloud CLI
You need the `gcloud` command on your local machine to control the cloud.

*   **Linux**:
    ```bash
    curl https://sdk.cloud.google.com | bash
    exec -l $SHELL
    ```

### 2. Physical Initialization
Once installed, run this to link your Google Account:
```bash
gcloud init
```
*   It will open your browser—log in with your Google account.
*   Select `[1] Create a new project`.
*   Give it a name (e.g., `cerberus-mission-01`).

### 3. Enable Billing (Crucial for Free Tier)
Google requires a linked credit card to prevent bot abuse, even for the **Free Tier**.
1.  Go to the [GCP Console Billing Page](https://console.cloud.google.com/billing).
2.  Enable billing for your new `cerberus-mission-01` project.
> [!NOTE]
> The `e2-micro` instance is FREE and falls under the "Always Free" usage limits. You won't be charged as long as you stay within the limits.

### 4. Run the Deployment
Once you've done the above, run our script from your terminal:
```bash
bash scripts/gcp_deploy.sh
```

---

### 🛡️ Admin Security Note
The deployment script configures a "Tactical Firewall."
- **Internal Dashboard**: Protected by SSH tunneling or Basic Auth. Your password is now **`CERBERUS_THREAT_OMEGA_99X`**.
- **Attacker Entry**: Only specific ports (like 2222) are open to the world.
- **Your IP**: Only your IP will have access to the management console.
