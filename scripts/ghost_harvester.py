#!/usr/bin/env python3
"""
CERBERUS Ghost Harvester
Periodically hunts for real-world IoT identities to feed the Morphing Engine.
"""

import os
import sys
import time
import subprocess
import random

# Config
INTERVAL = 3600 # 1 hour
QUERIES = [
    "product:Hikvision",
    "product:Dahua",
    "product:TP-Link",
    "product:Netgear",
    "port:37777", # Dahua DVR
    "server:GoAhead", # Common embedded web server
    "title:'IP Camera'"
]

SCRIPTS = ["scripts/harvest_shodan.py", "scripts/harvest_zoomeye.py"]

def run_harvest():
    query = random.choice(QUERIES)
    script = random.choice(SCRIPTS)

    print(f"[*] Ghost Harvester starting cycle: {script} with query '{query}'")

    cmd = [sys.executable, script, "--query", query, "--limit", "2", "--append"]
    try:
        # We don't want to fail if one service is down or out of credits
        subprocess.run(cmd, timeout=30)
    except Exception as e:
        print(f"[-] Harvester cycle failed: {e}")

if __name__ == "__main__":
    print("👻 CERBERUS GHOST HARVESTER ACTIVE")
    print("[*] Monitoring the internet for new identities...")

    # Run once at start
    run_harvest()

    while True:
        time.sleep(INTERVAL)
        run_harvest()
