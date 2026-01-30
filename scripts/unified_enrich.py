#!/usr/bin/env python3
"""
CERBERUS Unified Intelligence Enricher
Aggregates data from AbuseIPDB, GreyNoise, Shodan, and ZoomEye.
"""

import os
import sys
import json
import argparse
import urllib.request
import urllib.error
from datetime import datetime
from dotenv import load_dotenv

load_dotenv()

def query_abuseipdb(api_key, ip):
    """Check IP reputation on AbuseIPDB (1,000 free requests/day)."""
    if not api_key: return None
    url = f"https://api.abuseipdb.com/api/v2/check?ipAddress={ip}&maxAgeInDays=90"
    headers = {"Accept": "application/json", "Key": api_key}
    try:
        req = urllib.request.Request(url, headers=headers)
        with urllib.request.urlopen(req) as response:
            data = json.loads(response.read().decode())['data']
            return {
                "score": data.get('abuseConfidenceScore'),
                "total_reports": data.get('totalReports'),
                "last_report": data.get('lastReportedAt'),
                "country": data.get('countryCode'),
                "usage_type": data.get('usageType')
            }
    except: return None

def query_greynoise(api_key, ip):
    """Check if IP is 'Noise' on GreyNoise Community API."""
    # GreyNoise Community doesn't always require a key for basic checks, but it helps.
    url = f"https://api.greynoise.io/v3/community/{ip}"
    headers = {"Accept": "application/json"}
    if api_key: headers["key"] = api_key

    try:
        req = urllib.request.Request(url, headers=headers)
        with urllib.request.urlopen(req) as response:
            data = json.loads(response.read().decode())
            return {
                "noise": data.get('noise'),
                "riot": data.get('riot'),
                "classification": data.get('classification'),
                "name": data.get('name'),
                "last_seen": data.get('last_seen')
            }
    except: return None

def query_shodan_basic(api_key, ip):
    """Basic Shodan host lookup."""
    if not api_key: return None
    url = f"https://api.shodan.io/shodan/host/{ip}?key={api_key}"
    try:
        with urllib.request.urlopen(url) as response:
            data = json.loads(response.read().decode())
            return {
                "ports": data.get('ports'),
                "isp": data.get('isp'),
                "os": data.get('os'),
                "hostnames": data.get('hostnames')
            }
    except: return None

def query_zoomeye_basic(api_key, ip):
    """Basic ZoomEye host lookup (ai domain)."""
    if not api_key: return None
    url = f"https://api.zoomeye.ai/host/search?query=ip:{ip}"
    headers = {"API-KEY": api_key}
    try:
        req = urllib.request.Request(url, headers=headers)
        with urllib.request.urlopen(req) as response:
            data = json.loads(response.read().decode())
            if data.get('matches'):
                match = data['matches'][0]
                return {
                    "tags": match.get('tags'),
                    "app": match.get('portinfo', {}).get('app')
                }
    except: return None

def enrich_all(ip):
    # Load keys from environment
    ABUSE_KEY = os.environ.get("ABUSEIPDB_API_KEY")
    GREY_KEY = os.environ.get("GREYNOISE_API_KEY")
    SHODAN_KEY = os.environ.get("SHODAN_API_KEY")
    ZOOM_KEY = os.environ.get("ZOOMEYE_API_KEY")

    results = {
        "ip": ip,
        "timestamp": datetime.now().isoformat(),
        "summary": "Intelligence gathering complete.",
        "tags": []
    }

    # 1. AbuseIPDB (High Value for Honeypots)
    abuse = query_abuseipdb(ABUSE_KEY, ip)
    if abuse:
        results["abuse"] = abuse
        if abuse['score'] > 50:
            results["tags"].append(f"AbuseScore: {abuse['score']}%")

    # 2. GreyNoise (Know if it's just a common scanner)
    grey = query_greynoise(GREY_KEY, ip)
    if grey:
        results["greynoise"] = grey
        if grey['noise']:
            results["tags"].append(f"Noise: {grey['classification']}")

    # 3. Shodan
    shodan = query_shodan_basic(SHODAN_KEY, ip)
    if shodan:
        results["shodan"] = shodan
        if shodan['isp']:
            results["isp"] = shodan['isp']

    # 4. ZoomEye
    zoom = query_zoomeye_basic(ZOOM_KEY, ip)
    if zoom:
        results["zoomeye"] = zoom
        if zoom['tags']:
            results["tags"].extend(zoom['tags'])

    # Build Final Summary
    tag_str = ", ".join(results["tags"])
    if tag_str:
        results["summary"] = f"IP {ip} flagged as: {tag_str}"
    else:
        results["summary"] = f"IP {ip} has no significant threat history."

    return results

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip", required=True)
    args = parser.parse_args()

    intel = enrich_all(args.ip)
    print(json.dumps(intel, indent=2))
