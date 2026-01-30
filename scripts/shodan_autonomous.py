#!/usr/bin/env python3
"""
CERBERUS Autonomous Identity Acquisition (AIA)
Automates Shodan web scraping to bypass API search limits for "Proper" profile cloning.
"""

import requests
from bs4 import BeautifulSoup
import os
import sys
import time
import argparse
import random
import subprocess

LOGIN_URL = "https://account.shodan.io/login"
SEARCH_URL = "https://www.shodan.io/search"

class ShodanScraper:
    def __init__(self, cookie_value):
        self.session = requests.Session()
        self.session.headers.update({
            "User-Agent": "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36",
        })
        # Set the authentication cookie provided by the user
        self.session.cookies.set("polito", cookie_value, domain=".shodan.io")

    def login(self):
        print("[*] Validating Shodan Session Cookie...")
        try:
            res = self.session.get("https://www.shodan.io/account")
            if "Logout" in res.text:
                print("[+] Session Valid. Logged in as authorized user.")
                return True
            else:
                print("[-] Session Invalid or Expired. Please provide a fresh 'polito' cookie.")
                return False
        except Exception as e:
            print(f"[-] Validation Error: {e}")
            return False

    def find_ips(self, query, pages=1):
        ips = []
        print(f"[*] Hunting for targets with query: {query}...")

        for p in range(1, pages + 1):
            params = {"query": query, "page": p}
            try:
                res = self.session.get(SEARCH_URL, params=params)
                soup = BeautifulSoup(res.text, "html.parser")

                result_divs = soup.find_all("div", class_="search-result")
                for div in result_divs:
                    ip_tag = div.find("a", class_="ip")
                    if ip_tag:
                        ip = ip_tag.get_text().strip()
                        ips.append(ip)

                time.sleep(random.uniform(2, 5))
            except Exception as e:
                print(f"[-] Search Error on page {p}: {e}")

        return list(set(ips))

def run_cloning_engine(ips, api_key):
    print(f"\n[*] Passing {len(ips)} IPs to the Precision Cloning Engine...")
    for ip in ips:
        print(f"[*] Ingesting {ip}...")
        try:
            cmd = [sys.executable, "scripts/harvest_shodan.py", "--key", api_key, "--ip", ip, "--append"]
            subprocess.run(cmd)
        except Exception as e:
            print(f"[-] Ingestion failed for {ip}: {e}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="CERBERUS AIA Engine")
    parser.add_argument("--cookie", help="Shodan 'polito' session cookie")
    parser.add_argument("--key", help="Shodan API Key")
    parser.add_argument("--query", default="product:Hikvision", help="Search query")
    parser.add_argument("--pages", type=int, default=1, help="Number of pages to scrape")

    args = parser.parse_args()

    cookie = args.cookie or os.environ.get("SHODAN_SESSION_COOKIE")
    api_key = args.key or os.environ.get("SHODAN_API_KEY")

    if not cookie:
        print("Error: Provide Shodan 'polito' cookie via --cookie or SHODAN_SESSION_COOKIE env var")
        sys.exit(1)

    if not api_key:
        print("Error: Provide Shodan API Key via --key or SHODAN_API_KEY env var")
        sys.exit(1)

    scraper = ShodanScraper(cookie)
    if scraper.login():
        ips = scraper.find_ips(args.query, args.pages)
        if ips:
            print(f"[+] Found {len(ips)} unrestricted targets.")
            run_cloning_engine(ips, api_key)
        else:
            print("[!] No IPs found. Search listing might be empty or blocked (Cloudflare?).")
    else:
        print("[!] AIA Engine failed to start.")
