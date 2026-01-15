#!/usr/bin/env python3
import os
import time
import requests
import json
from datetime import datetime

# ==============================================================================
# DISCORD DASHBOARD BRIDGE
# This script "mirrors" the honeypot state to a Discord channel.
# ==============================================================================

# Project Paths
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOG_FILE = os.path.join(BASE_DIR, "services/cowrie/logs/cowrie.log")
STATE_FILE = os.path.join(BASE_DIR, "build/morph-state.txt")

# Configuration (Use environment variable or edit here)
WEBHOOK_URL = os.environ.get('DISCORD_WEBHOOK_URL', 'YOUR_WEBHOOK_URL_HERE')
# If using a Forum Channel, specify a thread name to create a new post, or thread_id to post to existing
THREAD_NAME = "🛡️ Cerberus Live Dashboard - " + datetime.now().strftime("%Y-%m-%d %H:%M")
THREAD_ID = os.environ.get('DISCORD_THREAD_ID', None) 

def post_to_discord(content, title="Cerberus Status", color=0x3498db):
    global THREAD_ID
    if not WEBHOOK_URL or "YOUR" in WEBHOOK_URL:
        print(f"[Offline Dashboard] {title}: {content}")
        return

    payload = {
        "embeds": [{
            "title": title,
            "description": content,
            "color": color,
            "timestamp": datetime.utcnow().isoformat()
        }]
    }
    
    # Handle Forum Channels or Threads
    params = {}
    if THREAD_ID:
        params['thread_id'] = THREAD_ID
    elif THREAD_NAME:
        # thread_name is only used if the webhook is for a forum channel
        payload['thread_name'] = THREAD_NAME

    try:
        response = requests.post(WEBHOOK_URL, json=payload, params=params, timeout=5)
        # If we successfully created a forum thread, capture the ID so we keep posting to it
        if not THREAD_ID and response.status_code == 201:
            try:
                THREAD_ID = response.json().get('id')
                print(f"✅ Created Forum Thread: {THREAD_NAME} (ID: {THREAD_ID})")
            except: pass
    except Exception as e:
        print(f"Failed to post to Discord: {e}")

def get_current_profile():
    if os.path.exists(STATE_FILE):
        try:
            with open(STATE_FILE, 'r') as f:
                c = f.read().strip()
                return c.split('=')[1] if '=' in c else c
        except: return "N/A"
    return "N/A"

def monitor():
    print("🚀 Cerberus-to-Discord Bridge Started")
    print("Monitoring for morphs and SSH activity...")

    last_profile = get_current_profile()
    post_to_discord(f"🛡️ **Cerberus System Online**\nInitial Profile: `{last_profile}`", "System Start", 0x2ecc71)

    # Simple tail-like monitoring
    if not os.path.exists(LOG_FILE):
        os.makedirs(os.path.dirname(LOG_FILE), exist_ok=True)
        open(LOG_FILE, 'a').close()

    log_size = os.path.getsize(LOG_FILE)

    try:
        while True:
            # 1. Check for Profile Morph
            current_profile = get_current_profile()
            if current_profile != last_profile:
                msg = f"🔄 **BIO-ADAPTIVE MORPH COMPLETE**\nNew Device Identity: `{current_profile}`\nAll banners and command outputs rotated."
                post_to_discord(msg, "Identity Rotation", 0x9b59b6)
                last_profile = current_profile

            # 2. Check logs for new activity
            current_size = os.path.getsize(LOG_FILE)
            if current_size > log_size:
                with open(LOG_FILE, 'r') as f:
                    f.seek(log_size)
                    new_lines = f.readlines()
                    for line in new_lines:
                        line = line.strip()
                        if "New connection" in line:
                            ip = line.split('connection: ')[1].split(' ')[0] if 'connection: ' in line else "Unknown"
                            post_to_discord(f"🌐 **New Connection Detected**\nSource: `{ip}`", "Intrusion Alert", 0x34495e)
                        elif "login attempt" in line:
                            status = "SUCCESS" if "succeeded" in line else "FAILED"
                            user_pass = line.split('attempt [')[1].split(']')[0] if '[' in line else "Unknown"
                            color = 0xe74c3c if "failed" in line.lower() else 0xf1c40f
                            post_to_discord(f"🔑 **Login Attempt: {status}**\nCredentials: `{user_pass}`", "Authentication Event", color)
                        elif "CMD:" in line:
                            cmd = line.split('CMD: ')[1]
                            post_to_discord(f"⌨️ **Command Executed**\n`$ {cmd}`", "Shell Activity", 0xe67e22)
                log_size = current_size

            # 3. Periodic Quorum Check (every 10 seconds or so)
            if int(time.time()) % 10 == 0:
                try:
                    # Run quorum engine
                    res = subprocess.run([os.path.join(BASE_DIR, "build/quorum")], capture_output=True, text=True, timeout=5)
                    if res.returncode == 1: # Alert detected
                        # Extract the alert details (IP and services)
                        alert_text = res.stdout.split("ALERT:")[1].split("---")[0] if "ALERT:" in res.stdout else "Coordinated attack detected!"
                        post_to_discord(f"🚨 **QUORUM ALERT: COORDINATED ATTACK**\n{alert_text.strip()}", "Security Alert", 0xff0000)
                except: pass

            time.sleep(2)
    except KeyboardInterrupt:
        print("\nStopping bridge...")

if __name__ == "__main__":
    monitor()
