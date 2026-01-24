#!/usr/bin/env python3
import os
import sys
import requests
import json
from datetime import datetime

# ==============================================================================
# DISCORD WEBHOOK INTEGRATION
# ==============================================================================
# To get a URL: Discord Server Settings -> Integrations -> Webhooks -> New Webhook
# ==============================================================================
DEFAULT_WEBHOOK_URL = os.environ.get('DISCORD_WEBHOOK_URL', 'YOUR_WEBHOOK_URL_HERE')

def send_to_discord(message, title="CERBERUS ALERT", color=0x7289da, status="INFO"):
    """
    Sends a formatted embed message to Discord.
    """
    if not DEFAULT_WEBHOOK_URL or 'YOUR_WEBHOOK_URL_HERE' in DEFAULT_WEBHOOK_URL:
        # Silently fail or log if URL is not set (so it doesn't break the demo)
        print(f"[Discord] (Skipped - URL not set) {status}: {message}")
        return False

    now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    # Mapping icons to status
    icons = {
        "INFO": "ℹ️",
        "WARN": "⚠️",
        "ALERT": "🚨",
        "SUCCESS": "✅",
        "ATTACK": "⚔️"
    }
    icon = icons.get(status, "🔹")

    payload = {
        "username": "Cerberus Honeypot",
        "avatar_url": "https://i.imgur.com/8n8o888.png", # Placeholder logo
        "embeds": [
            {
                "title": f"{icon} {title}",
                "description": message,
                "color": color,
                "footer": {
                    "text": f"Cerberus Bio-Adaptive IoT System | {now}"
                }
            }
        ]
    }

    try:
        response = requests.post(
            DEFAULT_WEBHOOK_URL,
            data=json.dumps(payload),
            headers={'Content-Type': 'application/json'},
            timeout=5
        )
        if response.status_code == 204:
            print(f"[Discord] Sent successfully: {message}")
            return True
        else:
            print(f"[Discord] Error {response.status_code}: {response.text}")
            return False
    except Exception as e:
        print(f"[Discord] Exception: {e}")
        return False

if __name__ == "__main__":
    if len(sys.argv) > 1:
        msg = " ".join(sys.argv[1:])
        status = "INFO"
        title = "CERBERUS NOTIFICATION"

        # Simple keywords for command line use
        if "login" in msg.lower() or "connected" in msg.lower():
            status = "SUCCESS"
            title = "NEW SSH CONNECTION"
        elif "attack" in msg.lower() or "detection" in msg.lower():
            status = "ALERT"
            title = "QUORUM DETECTION ALERT"

        send_to_discord(msg, title=title, status=status)
    else:
        # Default test
        send_to_discord("Testing Cerberus Discord Notification System...", title="Integration Test", status="SUCCESS")
