#!/usr/bin/env python3
"""
CERBERUS ZoomEye Profile Harvester (Zero-Dependency)
Fetches real-world device fingerprints from ZoomEye to populate CERBERUS profiles.
"""

import os
import sys
import json
import argparse
import random
import urllib.request
import urllib.error

def get_ghost_profile():
    """GHOST FALLBACK: Generate realistic simulation data if API fails."""
    print("[!] GHOST FALLBACK: Generating realistic identity simulation")
    vendors = ["Hikvision", "Dahua", "TP-Link", "Cisco", "Netgear"]
    vendor = random.choice(vendors)
    fake_ip = f"{random.randint(1,254)}.{random.randint(1,254)}.{random.randint(1,254)}.{random.randint(1,254)}"
    return [{
        "name": f"Ghost_{vendor}_{fake_ip.replace('.', '_')}",
        "ssh_banner": f"SSH-2.0-OpenSSH_7.4-Ghost-{vendor}",
        "kernel": "Linux 3.10.49",
        "arch": "armv7l",
        "isp": f"Simulated {vendor} Node",
        "vulns": []
    }]

def harvest_search(api_key, query, limit=5):
    """Search for devices on ZoomEye."""
    url = f"https://api.zoomeye.ai/host/search?query={urllib.parse.quote(query)}&page=1"

    headers = {
        "API-KEY": api_key
    }

    try:
        print(f"[*] Searching ZoomEye for: {query}...")
        req = urllib.request.Request(url, headers=headers)
        with urllib.request.urlopen(req) as response:
            results = json.loads(response.read().decode())

        profiles = []
        matches = results.get('matches', [])
        if not matches:
            print("[!] No matches found for the query.")
            return get_ghost_profile()

        for result in matches[:limit]:
            ip = result.get('ip', '0.0.0.0')
            portinfo = result.get('portinfo', {})
            service = portinfo.get('service', 'Generic')
            app = portinfo.get('app', 'Embedded')
            banner = portinfo.get('banner', '')

            ssh_banner = "SSH-2.0-OpenSSH_7.4"
            if 'ssh' in service.lower() or '22' == str(portinfo.get('port')):
                ssh_banner = banner.split('\n')[0] if banner else ssh_banner

            os_info = result.get('geoinfo', {}).get('os', 'Linux 3.10.49')

            profile = {
                "name": f"ZoomEye_{app.replace(' ', '_')}_{ip.replace('.', '_')}",
                "ssh_banner": ssh_banner,
                "kernel": os_info if os_info else "Linux 3.10.49",
                "arch": "armv7l",
                "isp": result.get('geoinfo', {}).get('isp', 'Unknown ISP'),
                "vulns": []
            }
            profiles.append(profile)

        return profiles
    except Exception as e:
        print(f"Error: {e}")
        return get_ghost_profile()

def format_as_config(profiles):
    output = ""
    for p in profiles:
        output += f"[{p['name']}]\n"
        output += f"# Profile derived from real node on {p['isp']}\n"
        output += f"ssh_banner={p['ssh_banner']}\n"
        output += f"telnet_banner=Welcome to {p['name']} console\n"
        output += f"kernel_version={p['kernel'].split(' ')[-1] if ' ' in p['kernel'] else '3.10.49'}\n"
        output += f"arch={p['arch']}\n"
        output += f"shell_prompt=router# \n"
        output += f"is_vulnerable={1 if random.random() < 0.4 else 0}\n"
        output += f"memory_mb=128\n"
        output += f"cpu_mhz=600\n\n"
    return output

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="CERBERUS ZoomEye Harvester")
    parser.add_argument("--key", help="ZoomEye API Key")
    parser.add_argument("--query", help="Search query")
    parser.add_argument("--limit", type=int, default=3, help="Number of results to fetch")
    parser.add_argument("--append", action="store_true", help="Append to profiles.conf")

    args = parser.parse_args()

    api_key = args.key or "43F0F3D0-F277-3015f-6420-53093640cb5"
    if not args.query:
        print("Error: Provide a search query via --query")
        sys.exit(1)

    harvested = harvest_search(api_key, args.query, args.limit)

    if harvested:
        config_text = format_as_config(harvested)
        if args.append:
            with open("profiles.conf", "a") as f:
                f.write("\n# Harvested from ZoomEye\n")
                f.write(config_text)
            print(f"[*] Appended {len(harvested)} profiles to profiles.conf")
        else:
            print(config_text)
    else:
        print("[!] No profiles harvested.")
