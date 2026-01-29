#!/bin/bash
# ==============================================================================
# CERBERUS GCP DEPLOYMENT ENGINE (FREE TIER OPTIMIZED)
# ==============================================================================

PROJECT_ID="lofty-feat-469115-r8"
ZONE="us-central1-a"
INSTANCE_NAME="cerberus-command"

echo "[*] Preparing CERBERUS GCP Cloud Deployment (Free Tier)..."

# 1. Enable Required APIs
echo "[*] Enabling Compute Engine API..."
gcloud services enable compute.googleapis.com --project=$PROJECT_ID

# 2. Create Firewall Rules (Crucial for Honeypots)
echo "[*] Configuring Tactical Firewall Rules..."
gcloud compute firewall-rules create cerberus-ingress \
    --project=$PROJECT_ID \
    --description="Allow honeypot traffic" \
    --direction=INGRESS \
    --priority=1000 \
    --network=default \
    --action=ALLOW \
    --rules=tcp:2222,tcp:5000 \
    --source-ranges=0.0.0.0/0 \
    --target-tags=cerberus-node 2>/dev/null || echo "[!] Firewall rule already exists"

# 3. Spin up GCP Free Tier Instance (e2-micro)
echo "[*] Booting CERBERUS Node on us-central1 (GCP Free Tier)..."
gcloud compute instances create $INSTANCE_NAME \
    --project=$PROJECT_ID \
    --zone=$ZONE \
    --machine-type=e2-micro \
    --network-interface=network-tier=STANDARD,subnet=default \
    --maintenance-policy=MIGRATE \
    --provisioning-model=STANDARD \
    --service-account=default \
    --scopes=https://www.googleapis.com/auth/cloud-platform \
    --tags=cerberus-node \
    --create-disk=auto-delete=yes,boot=yes,device-name=$INSTANCE_NAME,image-family=ubuntu-2204-lts,image-project=ubuntu-os-cloud,mode=rw,size=20,type=projects/$PROJECT_ID/zones/$ZONE/diskTypes/pd-standard \
    --no-shielded-secure-boot \
    --shielded-vtpm \
    --shielded-integrity-monitoring \
    --labels=env=honeypot,suite=cerberus,tier=free

echo "[+] Instance Created. Waiting for boot..."
sleep 15

# 4. Remote Execution: Install Dependencies and Startup CERBERUS
echo "[*] Deploying CERBERUS software suite..."
gcloud compute ssh $INSTANCE_NAME --zone=$ZONE --project=$PROJECT_ID --command="
    sudo apt-get update && sudo apt-get install -y docker.io docker-compose make build-essential python3-pip git
    git clone https://github.com/Phantomojo/cerberus-honeypot.git
    cd cerberus-honeypot
    sudo make build
    sudo bash scripts/morph_and_reload.sh
    nohup sudo python3 scripts/web_dashboard.py > build/cloud_dashboard.log 2>&1 &
"

EXTERNAL_IP=$(gcloud compute instances describe $INSTANCE_NAME --zone=$ZONE --project=$PROJECT_ID --format='get(networkInterfaces[0].accessConfigs[0].natIP)')

echo ""
echo "[SUCCESS] CERBERUS is now hunting in the cloud."
echo "[INFO] Access Dashboard: http://$EXTERNAL_IP:5000"
echo "[INFO] Attacker Entry Port: 2222"
echo ""
echo "[FREE TIER NOTES]"
echo "- Instance: e2-micro (Always Free: 1 instance/month in us-central1)"
echo "- Disk: 20GB Standard PD (Within 30GB free limit)"
echo "- Network: STANDARD tier (1GB egress/month free to Americas)"
echo "- Monthly Cost: \$0.00 (if within limits)"
