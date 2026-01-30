#!/usr/bin/env python3
"""
CERBERUS Multi-Vendor Cloning Engine
Batch-processes harvested IP addresses across different IoT vendors.
"""

import subprocess
import sys
import os

def batch_clone(vendor_data, api_key):
    print(f"[*] Starting Multi-Vendor Precision Cloning...")

    for vendor, ips in vendor_data.items():
        print(f"\n[!] Vendor: {vendor}")
        for ip in ips:
            ip = ip.strip()
            if not ip: continue

            print(f"[*] Cloning {ip}...")
            try:
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

    # Harvested IPs from Multi-Vendor Scrape
    harvested_data = {
        "D-Link": ["47.252.18.37", "151.56.204.137", "188.83.11.119", "115.69.45.105", "172.234.64.71"],
        "TP-Link": ["178.167.119.255", "73.241.244.87", "176.98.98.99", "80.122.28.26", "59.125.158.54"],
        "Dahua": ["91.150.84.85", "189.157.231.205", "119.241.90.37", "194.114.128.246", "42.112.172.5"],
        "Cisco": ["192.31.29.3", "201.217.100.100", "200.55.200.200"] # Manually added a few high-value Cisco targets observed
    }

    batch_clone(harvested_data, api_key)
    print("\n[*] Multi-Vendor Batch Cloning complete.")
