#!/usr/bin/env python3
"""
CERBERUS Precision Cloning Engine
Batch-processes harvested IP addresses into high-fidelity device profiles.
"""

import subprocess
import sys
import os

def batch_clone(ips, api_key):
    print(f"[*] Starting Precision Cloning for {len(ips)} targets...")

    for ip in ips:
        ip = ip.strip()
        if not ip: continue

        print(f"[*] Cloning {ip}...")
        try:
            # Call the existing harvest script with the --ip flag
            cmd = [
                sys.executable,
                "scripts/harvest_shodan.py",
                "--key", api_key,
                "--ip", ip,
                "--append"
            ]
            result = subprocess.run(cmd, capture_output=True, text=True)

            if result.returncode == 0:
                print(f"[+] Successfully cloned {ip}")
            else:
                print(f"[-] Failed to clone {ip}: {result.stderr.strip()}")

        except Exception as e:
            print(f"[-] Error processing {ip}: {e}")

if __name__ == "__main__":
    api_key = os.environ.get("SHODAN_API_KEY", "")
    if not api_key:
        print("Error: SHODAN_API_KEY environment variable not set.")
        sys.exit(1)

    # Target IPs harvested from Web UI
    hikvision_ips = [
        "89.151.2.172",
        "188.21.229.241",
        "5.185.158.55",
        "185.66.131.169",
        "5.249.47.44",
        "74.94.160.190",
        "115.42.71.97",
        "14.190.180.231",
        "80.188.185.210",
        "109.104.220.73"
    ]

    batch_clone(hikvision_ips, api_key)
    print("\n[*] Precision Cloning complete. Profiles updated in profiles.conf")
