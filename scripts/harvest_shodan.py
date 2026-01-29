#!/usr/bin/env python3
"""
CERBERUS Shodan Profile Harvester (Zero-Dependency)
Fetches real-world device fingerprints to populate CERBERUS profiles.
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
    vendors = ["Hikvision", "Dahua", "TP-Link", "Cisco", "Netgear", "Ubiquiti"]
    vendor = random.choice(vendors)
    fake_ip = f"{random.randint(1,254)}.{random.randint(1,254)}.{random.randint(1,254)}.{random.randint(1,254)}"
    return [{
        "name": f"ShodanGhost_{vendor}_{fake_ip.replace('.', '_')}",
        "ssh_banner": f"SSH-2.0-OpenSSH_7.4-Ghost-{vendor}",
        "kernel": "Linux 3.10.49",
        "arch": "armv7l",
        "isp": f"Simulated {vendor} Node",
        "vulns": []
    }]

def harvest_ip(api_key, ip):
    """Fetch metadata for a specific IP."""
    if not api_key: return get_ghost_profile()
    url = f"https://api.shodan.io/shodan/host/{ip}?key={api_key}"
    try:
        print(f"[*] Fetching metadata for IP: {ip}...")
        with urllib.request.urlopen(url) as response:
            data = json.loads(response.read().decode())

        product = data.get('product', 'Generic').replace(' ', '_')
        os_info = data.get('os', 'Linux 3.10')
        isp = data.get('isp', 'Unknown ISP')
        vulns = data.get('vulns', [])

        ssh_banner = "SSH-2.0-OpenSSH_7.4"
        for service in data.get('data', []):
            if service.get('transport') == 'tcp' and service.get('port') == 22:
                ssh_banner = service.get('data', '').split('\n')[0]
                break

        return [{
            "name": f"{product}_{ip.replace('.', '_')}",
            "ssh_banner": ssh_banner,
            "kernel": os_info if os_info else "Linux 3.10.49",
            "arch": "mips" if "mips" in str(data).lower() else "armv7l",
            "isp": isp,
            "vulns": vulns
        }]
    except Exception as e:
        print(f"Error fetching IP {ip}: {e}")
        return get_ghost_profile()

def harvest_search(api_key, query, limit=5):
    """Search for devices on Shodan."""
    if not api_key: return get_ghost_profile()
    url = f"https://api.shodan.io/shodan/host/search?key={api_key}&query={urllib.parse.quote(query)}&limit={limit}"

    try:
        print(f"[*] Searching Shodan for: {query}...")
        with urllib.request.urlopen(url) as response:
            results = json.loads(response.read().decode())

        profiles = []
        for result in results.get('matches', []):
            product = result.get('product', 'Generic').replace(' ', '_')
            os_info = result.get('os', 'Linux 3.10')
            isp = result.get('isp', 'Unknown ISP')
            vulns = result.get('vulns', [])

            banner_data = result.get('data', '')
            ssh_banner = banner_data.split('\n')[0] if 'ssh' in banner_data.lower() else "SSH-2.0-OpenSSH_7.4"

            profile = {
                "name": f"{product}_{result['ip_str'].replace('.', '_')}",
                "ssh_banner": ssh_banner,
                "kernel": os_info if os_info else "Linux 3.10.49",
                "arch": "mips" if "mips" in str(result).lower() else "armv7l",
                "isp": isp,
                "vulns": vulns
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
        output += f"shell_prompt={p['name'].split('_')[0].lower()}# \n"
        output += f"is_vulnerable={1 if p.get('vulns') or random.random() < 0.3 else 0}\n"
        if p.get('vulns'):
            output += f"cves={','.join(p['vulns'][:5])}\n"
        output += f"memory_mb=128\n"
        output += f"cpu_mhz=600\n\n"
    return output

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="CERBERUS Shodan Harvester")
    parser.add_argument("--key", help="Shodan API Key")
    parser.add_argument("--query", help="Search query")
    parser.add_argument("--ip", help="Specific IP to harvest")
    parser.add_argument("--limit", type=int, default=3, help="Number of results")
    parser.add_argument("--append", action="store_true", help="Append to profiles.conf")

    args = parser.parse_args()

    api_key = args.key or os.environ.get("SHODAN_API_KEY")
    harvested = []
    if args.ip:
        harvested = harvest_ip(api_key, args.ip)
    elif args.query:
        harvested = harvest_search(api_key, args.query, args.limit)
    else:
        harvested = get_ghost_profile()

    if harvested:
        config_text = format_as_config(harvested)
        if args.append:
            with open("profiles.conf", "a") as f:
                f.write("\n# Harvested from Shodan\n")
                f.write(config_text)
            print(f"[*] Appended {len(harvested)} profiles to profiles.conf")
        else:
            print(config_text)
    else:
        print("[!] No profiles harvested.")
