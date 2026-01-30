import socket
import json
import os
import argparse
import sys
try:
    from .cerberus_loader import load_cerberus_output, cerberus_available
except ImportError:
    try:
        from cerberus_loader import load_cerberus_output, cerberus_available
    except ImportError:
        def load_cerberus_output(*args, **kwargs): return None
        def cerberus_available(): return False

import time

# Configuration
BRAIN_HOST = os.getenv("CERBERUS_BRAIN_HOST", "localhost")
BRAIN_PORT = int(os.getenv("CERBERUS_BRAIN_PORT", 50051))
CACHE_PATH = "/cowrie/cowrie-git/var/lib/cowrie/ai_cache.json"
RATE_LIMIT_COOLDOWN = 2 # Seconds between requests per IP
ip_cooldowns = {}

def check_rate_limit(ip):
    now = time.time()
    if ip in ip_cooldowns and now - ip_cooldowns[ip] < RATE_LIMIT_COOLDOWN:
        return False
    ip_cooldowns[ip] = now
    return True

def get_cached_response(command):
    if not os.path.exists(CACHE_PATH):
        return None
    try:
        with open(CACHE_PATH, 'r') as f:
            cache = json.load(f)
            return cache.get(command)
    except:
        return None

def save_to_cache(command, response):
    try:
        cache = {}
        if os.path.exists(CACHE_PATH):
            with open(CACHE_PATH, 'r') as f:
                cache = json.load(f)
        cache[command] = response
        with open(CACHE_PATH, 'w') as f:
            json.dump(cache, f)
    except:
        pass

def query_brain(command, attacker_ip="unknown"):
    """Tiered Defense: 1. C-Engine -> 2. Local Cache -> 3. Remote PC Brain."""

    # --- TIER 0: RATE LIMITING ---
    if not check_rate_limit(attacker_ip):
         return {"response": "System busy. Try again in a few seconds.", "status": "error"}

    # --- TIER 1: C-ENGINE (Ultra Fast) ---
    if cerberus_available():
        # Map command to specific C-engine files if available
        # This uses the high-performance local morphing engine output
        c_output = load_cerberus_output(command.split()[0], args=command.split()[1:], ip=attacker_ip)
        if c_output:
            return {"response": c_output, "status": "success"}

    # --- TIER 2: LOCAL CACHE (Fast) ---
    cached = get_cached_response(command)
    if cached:
        return {"response": cached, "status": "success"}

    # --- TIER 3: REMOTE BRAIN (Local PC) ---
    try:
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.settimeout(5)
        client.connect((BRAIN_HOST, BRAIN_PORT))
        client.sendall(command.encode('utf-8'))

        response_data = client.recv(8192).decode('utf-8')
        client.close()

        result = json.loads(response_data)
        if result.get("status") == "success":
            save_to_cache(command, result.get("response"))
        return result
    except:
        return {"response": f"bash: {command}: command not found", "status": "error"}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Cerberus AI Bridge")
    parser.add_argument("command", help="The command to process")
    parser.add_argument("--ip", default="unknown", help="Attacker IP")
    args = parser.parse_args()

    result = query_brain(args.command, args.ip)

    # Cowrie custom commands expect the raw response string
    if result.get("status") == "success":
        print(result.get("response", ""))
    else:
        # Fallback to standard error message if brain is down
        print(f"bash: {args.command}: command not found")
        sys.exit(1)
