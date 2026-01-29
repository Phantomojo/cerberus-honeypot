#!/usr/bin/env python3
"""
CERBERUS ZoomEye Attacker Enrichment
Queries ZoomEye for an attacker's IP to determine if they are a known scanner or compromised device.
"""

import os
import sys
import json
import argparse
import urllib.request
import urllib.error

def enrich_ip(api_key, ip):
    """Fetch intelligence on a specific IP address."""
    # ZoomEye Host Search API for a specific IP: https://api.zoomeye.ai/host/search?query=ip:1.1.1.1
    url = f"https://api.zoomeye.ai/host/search?query=ip:{ip}"

    headers = {
        "API-KEY": api_key
    }

    try:
        req = urllib.request.Request(url, headers=headers)
        with urllib.request.urlopen(req) as response:
            results = json.loads(response.read().decode())

        if not results.get('matches'):
            return {"status": "unknown", "msg": "No data found for this IP on ZoomEye."}

        # Aggregate data from matches
        tags = set()
        ports = []
        services = []

        for match in results['matches']:
            portinfo = match.get('portinfo', {})
            port = portinfo.get('port')
            service = portinfo.get('service')
            if port: ports.append(port)
            if service: services.append(service)

            # ZoomEye often tags known scanners or botnets
            for tag in match.get('tags', []):
                tags.add(tag)

        return {
            "status": "known",
            "ip": ip,
            "tags": list(tags),
            "open_ports": list(set(ports)),
            "services": list(set(services)),
            "geoinfo": results['matches'][0].get('geoinfo', {}),
            "summary": f"IP {ip} is known to host {', '.join(list(set(services))[:3])}."
        }
    except Exception as e:
        return {"status": "error", "msg": str(e)}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="CERBERUS ZoomEye Enrichment")
    parser.add_argument("--ip", required=True, help="IP to enrich")
    parser.add_argument("--key", help="ZoomEye API Key")

    args = parser.parse_args()

    api_key = args.key or "43F0F3D0-F277-3015f-6420-53093640cb5"

    intel = enrich_ip(api_key, args.ip)
    print(json.dumps(intel, indent=2))
