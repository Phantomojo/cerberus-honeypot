#!/usr/bin/env python3
import os
import subprocess
import threading
import time
import json
import random
import hashlib
import psutil
import urllib.request
from datetime import datetime
from flask import Flask, render_template_string, jsonify, request
import sys
import sqlite3
from dotenv import load_dotenv

load_dotenv() # Load from .env file

# ==============================================================================
# CERBERUS COMMAND CENTER (V5.0 - PHOENIX TACTICAL HUD)
# ==============================================================================

app = Flask(__name__)

# Tactical Config
ADMIN_PASSWORD = os.environ.get("ADMIN_PASSWORD", "CERBERUS_THREAT_OMEGA_99X")
AUTH_TOKEN = hashlib.sha256(ADMIN_PASSWORD.encode()).hexdigest()

# Project Paths
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOG_FILE = os.path.join(BASE_DIR, "services/cowrie/logs/cowrie.json")
STATE_FILE = os.path.join(BASE_DIR, "build/morph-state.txt")
MORPH_BIN = os.path.join(BASE_DIR, "build/morph")
QUORUM_BIN = os.path.join(BASE_DIR, "build/quorum")

# Enrichment Config
IPINFO_TOKEN = os.environ.get("IPINFO_TOKEN")
ABUSE_KEY = os.environ.get("ABUSEIPDB_API_KEY")
SHODAN_KEY = os.environ.get("SHODAN_API_KEY")

# Global State
HISTORY_FILE = os.path.join(BASE_DIR, "build/web_history.json")
DB_FILE = os.path.join(BASE_DIR, "build/cerberus.db")
EVENT_HISTORY = []
QUORUM_ALERTS = []
SYSTEM_ERRORS = []
DASHBOARD_START_TIME = time.time()
GEO_CACHE = {}
AIRCRAFT_DATA = []
INTELLIGENCE_CACHE = {} # Store deep hits

def log_system_error(msg):
    ts = datetime.now().strftime("%H:%M:%S")
    SYSTEM_ERRORS.append({"time": ts, "msg": msg})
    print(f"ERROR [{ts}]: {msg}")

def check_auth():
    # Accept both hashed token (from localStorage) and plain password
    auth = request.headers.get('Authorization')
    if not auth:
        return False

    # Remove 'Bearer ' prefix
    token = auth.replace('Bearer ', '')

    # Check if it's the pre-hashed token OR hash the incoming password
    if token == AUTH_TOKEN:
        return True

    # Try hashing the incoming value (in case it's the plain password)
    plain_hash = hashlib.sha256(token.encode()).hexdigest()
    return plain_hash == AUTH_TOKEN

def resolve_geoip(ip):
    if ip in ["127.0.0.1", "localhost"] or ip.startswith("192.168."):
        return {"city": "Local Network", "region": "Internal", "lat": 0, "lon": 0, "company": {"name": "Cerberus Node"}}
    if ip in GEO_CACHE: return GEO_CACHE[ip]
    try:
        # Use ipinfo.io for premium carrier/location detail
        url = f"https://ipinfo.io/{ip}?token={IPINFO_TOKEN}"
        with urllib.request.urlopen(url, timeout=5) as response:
            data = json.loads(response.read().decode())
            loc = data.get("loc", "0,0").split(',')
            normalized = {
                "city": data.get("city", "Unknown"),
                "region": data.get("region", "Unknown"),
                "country": data.get("country", "Unknown"),
                "lat": float(loc[0]),
                "lon": float(loc[1]),
                "isp": data.get("org", "Unknown"),
                "org": data.get("org", "Unknown"),
                "postal": data.get("postal", ""),
                "timezone": data.get("timezone", "")
            }
            GEO_CACHE[ip] = normalized
            return normalized
    except Exception as e:
        log_system_error(f"IPInfo failed for {ip}: {str(e)}")
        # Fallback to ipwho.is if token fails or rate limited
        try:
            url = f"https://ipwho.is/{ip}"
            with urllib.request.urlopen(url, timeout=5) as response:
                data = json.loads(response.read().decode())
                if data.get("success"):
                    normalized = {
                        "city": data.get("city", "Unknown"),
                        "region": data.get("region", "Unknown"),
                        "country": data.get("country", "Unknown"),
                        "lat": data.get("latitude", 0),
                        "lon": data.get("longitude", 0),
                        "isp": data.get("connection", {}).get("isp", "Unknown"),
                        "org": data.get("connection", {}).get("org", "Unknown")
                    }
                    GEO_CACHE[ip] = normalized
                    return normalized
        except: pass
    return {"city": "Unknown", "region": "Unknown", "lat": 0, "lon": 0}

def get_reputation(ip):
    if ip in INTELLIGENCE_CACHE: return INTELLIGENCE_CACHE[ip]
    intel = {"abuse_score": 0, "reports": 0, "last_report": "None", "is_threat": False}
    try:
        # Check AbuseIPDB
        url = f"https://api.abuseipdb.com/api/v2/check?ipAddress={ip}&maxAgeInDays=90"
        req = urllib.request.Request(url)
        req.add_header('Key', ABUSE_KEY)
        req.add_header('Accept', 'application/json')
        with urllib.request.urlopen(req, timeout=5) as response:
            data = json.loads(response.read().decode())
            rep = data.get("data", {})
            intel["abuse_score"] = rep.get("abuseConfidenceScore", 0)
            intel["reports"] = rep.get("totalReports", 0)
            intel["last_report"] = rep.get("lastReportedAt", "Unknown")
            intel["is_threat"] = intel["abuse_score"] > 20
    except: pass
    INTELLIGENCE_CACHE[ip] = intel
    return intel

def get_shodan_intel(ip):
    # Check cache
    cache_key = f"shodan_{ip}"
    if cache_key in GEO_CACHE: return GEO_CACHE[cache_key]

    intel = {"ports": [], "os": "Unknown", "hostnames": [], "isp": "Unknown"}
    try:
        url = f"https://api.shodan.io/shodan/host/{ip}?key={SHODAN_KEY}"
        with urllib.request.urlopen(url, timeout=5) as response:
            data = json.loads(response.read().decode())
            intel = {
                "ports": data.get("ports", []),
                "os": data.get("os", "Unknown"),
                "hostnames": data.get("hostnames", []),
                "isp": data.get("isp", "Unknown")
            }
            GEO_CACHE[cache_key] = intel
    except: pass
    return intel

def generate_ai_report(ip, history):
    """
    Simulated AI Threat Intelligence Engine.
    In a full deployment, this calls HoneyGPT/Ollama.
    For the dashboard, it provides a structured Tactical Report.
    """
    if not history:
        return "NO_COMMAND_DATA: Threat intent cannot be determined."

    cmds = [h['cmd'] for h in history]
    # Simple tactical logic
    threat_level = "LOW"
    intent = "RECONNAISSANCE"
    technique = "MANUAL_PROBING"

    if any(c in str(cmds) for c in ['rm -rf', 'wget', 'curl', 'chmod +x']):
        threat_level = "CRITICAL"
        intent = "PAYLOAD_DELIVERY / PERSISTENCE"
        technique = "AUTOMATED_SCANNER_EXPLOIT"
    elif len(cmds) > 10:
        threat_level = "ELEVATED"
        intent = "SYSTEM_ENUMERATION"
        technique = "BRUTE_FORCE_EXPLORATION"

    report = f"""[AI INTEL REPORT // {ip}]
---------------------------------
THREAT_LEVEL: {threat_level}
INTENT: {intent}
TECHNIQUE: {technique}

OBSERVED_BEHAVIOR:
Attacker entered {len(cmds)} commands.
Primary focus: {cmds[0] if cmds else 'N/A'}.

TACTICAL_RECOMMENDATION:
{'ENABLE_QUORUM_HARDENING' if threat_level != 'LOW' else 'MONITOR_ONLY'}
"""
    return report

# Phoenix HUD: Aircraft Tracker (Simulated for high FPS)
def update_flight_hud():
    global AIRCRAFT_DATA
    while True:
        try:
            # Simulated Phoenix Data (MacGyver Style)
            new_aircraft = []
            for i in range(15):
                new_aircraft.append({
                    "id": f"PHX-{random.randint(100,999)}",
                    "lat": (random.random() * 120) - 60,
                    "lon": (random.random() * 360) - 180,
                    "alt": random.randint(20000, 45000),
                    "spd": random.randint(400, 600)
                })
            AIRCRAFT_DATA = new_aircraft
        except: pass
        time.sleep(10)

threading.Thread(target=update_flight_hud, daemon=True).start()

def init_db():
    conn = sqlite3.connect(DB_FILE)
    c = conn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS sessions
                 (ip TEXT, session_id TEXT PRIMARY KEY, start_time TEXT, city TEXT, country TEXT, score INTEGER)''')
    c.execute('''CREATE TABLE IF NOT EXISTS activities
                 (session_id TEXT, timestamp TEXT, input TEXT, output TEXT)''')
    conn.commit()
    conn.close()

init_db()

def log_session_to_db(src, sid, ts, geo, rep):
    try:
        conn = sqlite3.connect(DB_FILE)
        c = conn.cursor()
        c.execute("INSERT OR IGNORE INTO sessions VALUES (?,?,?,?,?,?)",
                  (src, sid, ts, geo['city'], geo['country'], rep['abuse_score']))
        conn.commit()
        conn.close()
    except: pass

def log_activity_to_db(sid, ts, cmd):
    try:
        conn = sqlite3.connect(DB_FILE)
        c = conn.cursor()
        c.execute("INSERT INTO activities VALUES (?,?,?,?)", (sid, ts, cmd, ""))
        conn.commit()
        conn.close()
    except: pass

def load_persistence():
    global EVENT_HISTORY, CREDENTIAL_COUNTS, QUORUM_ALERTS
    try:
        if os.path.exists(HISTORY_FILE):
            with open(HISTORY_FILE, 'r') as f:
                data = json.load(f)
                EVENT_HISTORY = data.get('events', [])
                CREDENTIAL_COUNTS = data.get('credentials', {})
                QUORUM_ALERTS = data.get('quorum', [])
    except: pass

def save_persistence():
    try:
        with open(HISTORY_FILE, 'w') as f:
            json.dump({"events": EVENT_HISTORY[:50], "credentials": CREDENTIAL_COUNTS, "quorum": QUORUM_ALERTS[-10:]}, f)
    except: pass

load_persistence()

def get_current_profile():
    try:
        if os.path.exists(STATE_FILE):
            with open(STATE_FILE, 'r') as f:
                c = f.read().strip()
                return c.split('=')[1] if '=' in c else c
    except: return "Generic Device"
    return "Initializing..."

def get_docker_stats():
    try:
        cmd = ["docker", "ps", "--format", "{{.Names}}|{{.Status}}"]
        output = subprocess.check_output(cmd).decode().strip()
        if not output: return []
        return [{"name": l.split('|')[0], "stat": l.split('|')[1], "active": "Up" in l.split('|')[1]} for l in output.split('\n') if '|' in l]
    except: return []

def get_system_metrics():
    try:
        # Get usage over 1s for accurate readings
        cpu = psutil.cpu_percent(interval=None)
        ram = psutil.virtual_memory().percent
        disk = psutil.disk_usage('/').percent
        return {"cpu": cpu, "ram": ram, "disk": disk}
    except: return {"cpu": 0, "ram": 0, "disk": 0}

def get_deployment_status():
    try:
        # Check for active docker builds
        cmd = ["ps", "aux"]
        output = subprocess.check_output(cmd).decode()
        if "docker-compose up" in output or "docker build" in output:
            # Estimate progress based on common log patterns or time
            # For now, return a tactical 'Deploying' state
            return {"active": True, "target": "CORE_SERVICES", "progress": 45}
        return {"active": False, "target": "STABLE", "progress": 100}
    except: return {"active": False, "target": "NULL", "progress": 0}

# Background Logic: Monitoring and Parsing
def log_monitor():
    global EVENT_HISTORY, CREDENTIAL_COUNTS, QUORUM_ALERTS
    if not os.path.exists(LOG_FILE):
        os.makedirs(os.path.dirname(LOG_FILE), exist_ok=True)
        open(LOG_FILE, 'a').close()
    file_pos = 0 # Start from beginning to catch recent events on start
    while True:
        try:
            if not os.path.exists(LOG_FILE):
                time.sleep(1)
                continue
            current_size = os.path.getsize(LOG_FILE)
            if current_size < file_pos: file_pos = 0 # Handle rotation
            if current_size > file_pos:
                with open(LOG_FILE, 'r') as f:
                    f.seek(file_pos)
                    for line in f:
                        if not line.strip(): continue
                        try:
                            data = json.loads(line.strip())
                            eventid = data.get('eventid')
                            ts = data.get('timestamp', '').split('T')[-1][:8]
                            src = data.get('src_ip', '127.0.0.1')
                            sid = data.get('session', 'N/A')
                            msg, etype = None, "info"

                            if eventid == "cowrie.session.connect":
                                geo = resolve_geoip(src)
                                msg = f"Inbound connection from {src} ({geo.get('city')}, {geo.get('region')})"
                                rep = get_reputation(src) # Ensure rep is defined before use
                                log_session_to_db(src, sid, ts, geo, rep)
                                QUORUM_ALERTS.append({"time": ts, "msg": f"Target detected: {src}"})
                            elif eventid == "cowrie.command.input":
                                log_activity_to_db(sid, ts, data.get('input', ''))
                                msg = f"COMMAND: {data.get('input')}"
                                etype = "warn"
                                if any(x in data.get('input', '') for x in ['rm', 'sudo', 'wget']):
                                    QUORUM_ALERTS.append({"time": ts, "msg": f"Hostage Command from {src}"})
                            elif eventid == "cowrie.login.success":
                                msg = f"SUCCESSFUL LOGIN: {data.get('username')}"
                                etype = "success"
                            elif eventid == "cowrie.login.failed":
                                msg = f"FAILED LOGIN: {data.get('username')}:{data.get('password')}"
                                etype = "warn"
                            elif eventid == "cowrie.lure.access":
                                msg = data.get('msg', 'Lure accessed!')
                                etype = "danger"
                                QUORUM_ALERTS.append({"time": ts, "msg": f"CRITICAL: Lure Violation by {src}"})

                            if msg:
                                rep = get_reputation(src)
                                EVENT_HISTORY.insert(0, {
                                    "time": ts,
                                    "msg": msg,
                                    "type": etype,
                                    "src": src,
                                    "threat": rep.get("is_threat", False),
                                    "score": rep.get("abuse_score", 0)
                                })
                                EVENT_HISTORY = EVENT_HISTORY[:50]
                        except Exception as e: continue
                file_pos = current_size
        except: pass
        time.sleep(1)

threading.Thread(target=log_monitor, daemon=True).start()

# HTML Template (V5.0 PHOENIX HUD)
HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>CERBERUS CC | PHOENIX VR 5.0</title>
    <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;700&family=JetBrains+Mono:wght@400;700&display=swap" rel="stylesheet">
    <script src='https://unpkg.com/maplibre-gl@3.6.2/dist/maplibre-gl.js'></script>
    <link href='https://unpkg.com/maplibre-gl@3.6.2/dist/maplibre-gl.css' rel='stylesheet' />
    <style>
        :root {
            --bg: #030408; --panel: rgba(10, 12, 18, 0.95); --accent: #00f2ff;
            --danger: #ff4757; --success: #2ed573; --text: #e2e8f0;
            --dim: #64748b; --border: 1px solid rgba(255, 255, 255, 0.08);
        }
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { background-color: var(--bg); color: var(--text); font-family: 'Outfit', sans-serif; height: 100vh; overflow: hidden; }

        /* Login Interface */
        #login-overlay { position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: var(--bg); z-index: 9999; display: flex; align-items: center; justify-content: center; flex-direction: column; }
        .login-box { background: var(--panel); border: var(--border); padding: 2.5rem; border-radius: 12px; width: 380px; text-align: center; }
        input { background: rgba(255,255,255,0.05); border: var(--border); color: white; padding: 0.8rem; border-radius: 4px; width: 100%; outline: none; font-family: 'JetBrains Mono'; margin-top: 1rem; }
        .login-btn { background: var(--accent); color: var(--bg); border: none; padding: 0.8rem; width: 100%; border-radius: 4px; font-weight: 800; margin-top: 1rem; cursor: pointer; }

        header { height: 60px; border-bottom: var(--border); display: flex; align-items: center; justify-content: space-between; padding: 0 1.5rem; background: rgba(0,0,0,0.8); backdrop-filter: blur(20px); z-index: 100; }
        .logo-text { font-size: 1rem; letter-spacing: 4px; font-weight: 800; border-left: 3px solid var(--accent); padding-left: 1rem; }

        main { display: grid; grid-template-columns: 320px 1fr 380px; height: calc(100vh - 60px); padding: 1rem; gap: 1rem; }
        .card { background: var(--panel); border: var(--border); border-radius: 12px; display: flex; flex-direction: column; position: relative; overflow: hidden; }
        .card-header { padding: 0.8rem 1.2rem; border-bottom: var(--border); font-size: 0.7rem; font-weight: 800; color: var(--dim); text-transform: uppercase; letter-spacing: 2px; background: rgba(255,255,255,0.02); }

        /* Intelligence Terminal (IP List) */
        .intel-terminal { overflow-y: auto; flex-grow: 1; }
        .intel-item { padding: 0.8rem 1.2rem; border-bottom: 1px solid rgba(255,255,255,0.03); cursor: pointer; transition: 0.2s; position: relative; }
        .intel-item:hover { background: rgba(0,242,255,0.05); }
        .intel-item.active { border-left: 3px solid var(--accent); background: rgba(0,242,255,0.08); }
        .intel-ip { font-family: 'JetBrains Mono'; font-weight: 700; font-size: 0.8rem; color: var(--accent); }
        .intel-meta { font-size: 0.6rem; color: var(--dim); margin-top: 0.2rem; }

        /* Map Section */
        .map-section { position: relative; overflow: hidden; background: #000; border: 1px solid rgba(0,242,255,0.15); }
        #map { position: absolute; top: 0; left: 0; width: 100%; height: 100%; }

        .layer-selector { position: absolute; top: 1rem; right: 1rem; z-index: 100; background: var(--panel); border: var(--border); border-radius: 8px; padding: 0.4rem; display: flex; flex-direction: column; gap: 0.3rem; }
        .layer-btn { background: rgba(255,255,255,0.03); border: 1px solid rgba(255,255,255,0.08); color: var(--dim); padding: 0.4rem 0.8rem; border-radius: 4px; font-size: 0.55rem; font-weight: 800; cursor: pointer; }
        .layer-btn.active { color: var(--accent); border-color: var(--accent); background: rgba(0,242,255,0.08); }

        .recon-toggle { position: absolute; top: 1rem; left: 1rem; z-index: 100; background: var(--panel); border: 1px solid var(--danger); border-radius: 20px; padding: 4px 12px; font-size: 0.6rem; font-weight: 800; color: var(--danger); cursor: pointer; }
        .recon-toggle.active { background: var(--danger); color: white; box-shadow: 0 0 15px var(--danger); }

        /* Flight Markers */
        .flight-marker { width: 12px; height: 12px; border: 1px solid var(--accent); position: relative; display: flex; align-items: center; justify-content: center; }
        .flight-marker::after { content: ''; width: 4px; height: 4px; background: var(--accent); border-radius: 50%; }

        .ping-marker { width: 16px; height: 16px; background: var(--danger); border: 2px solid white; border-radius: 50%; box-shadow: 0 0 20px var(--danger); animation: ping-zoom 1.5s infinite; }
        @keyframes ping-zoom { 0% { transform: scale(1); opacity: 1; } 100% { transform: scale(4); opacity: 0; } }

        .feed { padding: 1rem; font-family: 'JetBrains Mono'; font-size: 0.6rem; overflow-y: auto; flex-grow: 1; }

        /* Tactical Status Bar */
        footer { position: fixed; bottom: 0; left: 0; width: 100%; height: 32px; background: rgba(0,0,0,0.9); border-top: var(--border); display: flex; align-items: center; padding: 0 1rem; font-size: 0.6rem; font-family: 'JetBrains Mono'; z-index: 200; gap: 2rem; }
        .status-pill { display: flex; align-items: center; gap: 0.5rem; }
        .progress-container { width: 150px; height: 8px; background: rgba(255,255,255,0.05); border-radius: 4px; overflow: hidden; border: 1px solid rgba(255,255,255,1); }
        .progress-fill { height: 100%; background: var(--accent); transition: width 0.5s ease; box-shadow: 0 0 10px var(--accent); }

        /* Mobile Responsiveness */
        @media (max-width: 1024px) {
            body { overflow-y: auto; height: auto; }
            main { display: flex; flex-direction: column; height: auto; padding: 0.5rem; gap: 0.5rem; margin-bottom: 60px; }
            aside { height: auto; min-height: 300px; }
            .card { height: auto; min-height: 400px; }
            .map-section { height: 400px; order: -1; } /* Map at top on mobile */
            header { height: auto; padding: 1rem; flex-direction: column; gap: 0.5rem; }
            footer { height: auto; flex-wrap: wrap; padding: 0.8rem; gap: 1rem; position: relative; }
            .progress-container { width: 80px; }
            .feed { height: 200px; }
        }

        /* Terminal Proxy */
        .term-proxy { background: #000; border: 1px solid var(--accent); color: var(--accent); font-family: 'JetBrains Mono'; font-size: 0.7rem; padding: 0.5rem; border-radius: 4px; margin-top: 0.5rem; display: flex; }
        .term-proxy input { background: transparent; border: none; padding: 0; margin: 0; flex-grow: 1; color: var(--accent); font-family: inherit; font-size: inherit; outline: none; }
        .term-prompt { color: var(--success); margin-right: 0.5rem; }
    </style>
</head>
<body>
    <div id="login-overlay">
        <div class="login-box">
            <div style="font-size: 0.7rem; color: var(--accent); letter-spacing: 4px; font-weight: 800; margin-bottom: 1rem;">PHOENIX_GATEWAY_V5.0</div>
            <input type="password" id="admin-pass" placeholder="ENTER_TACTICAL_KEY">
            <button class="login-btn" onclick="tryLogin()">AUTHENTICATE</button>
        </div>
    </div>

    <header>
        <div class="logo-text">CERBERUS // PHOENIX HUD</div>
        <div id="uptime" style="font-family: 'JetBrains Mono'; font-size: 0.7rem; color: var(--dim);">BOOT_TIME: 00:00:00</div>
    </header>

    <main>
        <aside class="card">
            <div style="padding: 1rem; height: 120px; border-bottom: var(--border);">
                <canvas id="flux-chart" width="280" height="80"></canvas>
            </div>
            <div id="flux-events" class="feed" style="height: 200px; padding: 0.5rem; color: var(--danger);"></div>
            <div class="card-header">Intelligence Terminal</div>
            <div class="intel-terminal" id="intel-list"></div>
        </aside>

        <section class="card map-section">
            <div id="map"></div>
            <div class="recon-toggle" id="recon-toggle" onclick="toggleDeepRecon()">DEEP_RECON: OFF</div>
            <div class="layer-selector">
                <button class="layer-btn active" onclick="setMapLayer('standard')">NASA_GLOBAL</button>
                <button class="layer-btn" onclick="setMapLayer('hybrid')">HYBRID_INTEL</button>
                <button class="layer-btn" onclick="setMapLayer('highres')">DEEP_SAT</button>
                <button class="layer-btn" onclick="setMapLayer('street')">STREET_LEVEL</button>
            </div>
        </section>

        <aside class="card">
            <div class="card-header">Target Deep-Intel</div>
            <div id="target-details" style="padding: 1rem; flex-grow: 0.5; border-bottom: var(--border);">
                <div style="color: var(--dim); text-align: center; font-size: 0.7rem;">SELECT TARGET FROM TERMINAL</div>
            </div>
            <div class="card-header">Real-Time Telemetry</div>
            <div class="feed" id="feed"></div>
            <div class="card-header">Honeypot Terminal Proxy [RESTRICTED]</div>
            <div style="padding: 1rem;">
                <div class="term-proxy">
                    <span class="term-prompt">root@cerberus:~$</span>
                    <input type="text" id="term-input" placeholder="EXECUTE_DECEPTION_CMD" onkeypress="handleTerm(event)">
                </div>
            </div>
        </aside>
    </main>

    <footer>
        <div class="status-pill">
            <span style="color: var(--dim)">DEPLOYMENT_STATUS:</span>
            <span id="deploy-target" style="color: var(--accent)">STABLE</span>
            <div class="progress-container">
                <div id="deploy-progress" class="progress-fill" style="width: 100%"></div>
            </div>
            <span id="deploy-pct" style="color: var(--accent)">100%</span>
        </div>
        <div class="status-pill">
            <span style="color: var(--dim)">CPU:</span>
            <span id="cpu-usage" style="color: var(--success)">0%</span>
        </div>
        <div class="status-pill">
            <span style="color: var(--dim)">RAM:</span>
            <span id="ram-usage" style="color: var(--success)">0%</span>
        </div>
        <div class="status-pill">
            <span style="color: var(--dim)">DISK:</span>
            <span id="disk-usage" style="color: var(--success)">0%</span>
        </div>
        <div style="flex-grow: 1"></div>
        <div class="status-pill">
            <span style="color: var(--success)">●</span> SYNC_ACTIVE
        </div>
    </footer>

    <script>
        let dashboardToken = sessionStorage.getItem('cerberus_token');
        let deepReconActive = false;
        let selectedIp = null;
        let flightMarkers = [];

        const LAYERS = {
            standard: { id: 'standard', url: 'https://gibs.earthdata.nasa.gov/wmts/epsg3857/best/BlueMarble_NextGeneration/default/2022-06-15/GoogleMapsCompatible_Level8/{z}/{y}/{x}.jpg', zoom: 8 },
            hybrid: { id: 'hybrid', base: 'https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}', overlay: 'https://a.basemaps.cartocdn.com/dark_only_labels/{z}/{x}/{y}{r}.png', zoom: 19 },
            highres: { id: 'highres', url: 'https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}', zoom: 19 },
            street: { id: 'street', url: 'https://a.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png', zoom: 18 }
        };

        const map = new maplibregl.Map({
            container: 'map',
            style: { version: 8, sources: { 'base': { type: 'raster', tiles: [LAYERS.standard.url], tileSize: 256 } }, layers: [{ id: 'base-layer', type: 'raster', source: 'base', minzoom: 0, maxzoom: 8 }] },
            center: [0, 20], zoom: 1.5
        });

        async function tryLogin() {
            const pass = document.getElementById('admin-pass').value;
            // Send plain password - server will hash it
            const res = await fetch('/api/data', { headers: { 'Authorization': `Bearer ${pass}` } });
            if (res.ok) {
                dashboardToken = pass;
                sessionStorage.setItem('cerberus_token', pass);
                document.getElementById('login-overlay').style.display = 'none';
                startDashboard();
            }
            else alert("AUTH_FAILURE");
        }

        if (dashboardToken) { document.getElementById('login-overlay').style.display = 'none'; startDashboard(); }

        function toggleDeepRecon() {
            deepReconActive = !deepReconActive;
            const btn = document.getElementById('recon-toggle');
            btn.classList.toggle('active', deepReconActive);
            btn.innerText = `DEEP_RECON: ${deepReconActive ? 'ON' : 'OFF'}`;
        }

        function setMapLayer(id) {
            const l = LAYERS[id];
            if (!deepReconActive && id !== 'standard') { alert("ENABLE DEEP_RECON"); return; }
            document.querySelectorAll('.layer-btn').forEach(b => b.classList.toggle('active', b.innerText.toLowerCase().includes(id.substring(0,3))));
            if (map.getLayer('base-layer')) map.removeLayer('base-layer');
            if (map.getSource('base')) map.removeSource('base');
            if (map.getLayer('overlay-layer')) map.removeLayer('overlay-layer');
            if (map.getSource('overlay')) map.removeSource('overlay');
            map.addSource('base', { type: 'raster', tiles: [l.base || l.url], tileSize: 256 });
            map.addLayer({ id: 'base-layer', type: 'raster', source: 'base', minzoom: 0, maxzoom: l.zoom });
            if(l.overlay) {
                map.addSource('overlay', { type: 'raster', tiles: [l.overlay], tileSize: 256 });
                map.addLayer({ id: 'overlay-layer', type: 'raster', source: 'overlay', minzoom: 0, maxzoom: l.zoom });
            }
        }

        function focusTarget(ip, lat, lon) {
            selectedIp = ip;
            if(!deepReconActive) toggleDeepRecon();
            setMapLayer('hybrid');
            map.flyTo({ center: [lon, lat], zoom: 16, speed: 1.2 });
        }

        async function update() {
            const res = await fetch('/api/data', { headers: { 'Authorization': `Bearer ${dashboardToken}` } });
            if (!res.ok) return;
            const d = await res.json();

            // Update Intel Terminal
            const uniqueIps = Array.from(new Set(d.events.map(e => e.src))).filter(ip => ip !== '127.0.0.1');
            document.getElementById('intel-list').innerHTML = uniqueIps.map(ip => {
                const lastEvent = d.events.find(e => e.src === ip);
                return `<div class="intel-item ${selectedIp === ip ? 'active' : ''}" onclick="fetchAndFocus('${ip}')">
                    <div class="intel-ip">${ip}</div>
                    <div class="intel-meta">${lastEvent.msg}</div>
                </div>`;
            }).join('');

            // Update Telemetry
            document.getElementById('feed').innerHTML = d.events.map(e => `<div>[${e.time}] ${e.msg}</div>`).join('');

            // Update Flight HUD
            flightMarkers.forEach(m => m.remove());
            flightMarkers = d.aircraft.map(a => {
                const el = document.createElement('div'); el.className = 'flight-marker';
                return new maplibregl.Marker({element: el}).setLngLat([a.lon, a.lat]).addTo(map);
            });

            // Update Tactical Status Bar
            document.getElementById('cpu-usage').innerText = `${d.system.cpu}%`;
            document.getElementById('ram-usage').innerText = `${d.system.ram}%`;
            document.getElementById('disk-usage').innerText = `${d.system.disk}%`;

            const deploy = d.deployment;
            document.getElementById('deploy-target').innerText = deploy.target;
            document.getElementById('deploy-progress').style.width = `${deploy.progress}%`;
            document.getElementById('deploy-pct').innerText = `${deploy.progress}%`;

            // Health colors
            document.getElementById('cpu-usage').style.color = d.system.cpu > 80 ? 'var(--danger)' : 'var(--success)';
            document.getElementById('ram-usage').style.color = d.system.ram > 80 ? 'var(--danger)' : 'var(--success)';

            if (deploy.active) {
                document.getElementById('deploy-target').style.color = 'var(--accent)';
                document.getElementById('deploy-progress').style.background = 'var(--accent)';
            } else {
                document.getElementById('deploy-target').style.color = 'var(--success)';
                document.getElementById('deploy-progress').style.background = 'var(--success)';
            }

            // Update Global Threat Flux
            document.getElementById('flux-events').innerHTML = d.quorum.map(q => `<div>[${q.time}] ${q.msg}</div>`).join('');
        }

        async function fetchAndFocus(ip) {
            const res = await fetch(`/api/intel?ip=${ip}`, { headers: { 'Authorization': `Bearer ${dashboardToken}` } });
            const data = await res.json();
            const geo = data.geo;
            const rep = data.rep;
            const shodan = data.shodan || {};
            const history = data.history || [];

            document.getElementById('target-details').innerHTML = `
                <div style="font-size: 1rem; color: var(--accent); font-weight: 800;">${ip}</div>
                <div style="font-size: 0.6rem; color: var(--dim); margin-top:0.4rem;">ISP: ${geo.isp}</div>
                <div style="font-size: 0.6rem; color: var(--dim);">LOC: ${geo.city}, ${geo.region}, ${geo.country}</div>
                <div style="font-size: 0.6rem; color: var(--dim);">HOST: ${geo.hostname || 'N/A'}</div>

                <hr style="border: 0; border-top: 1px solid #222; margin: 10px 0;">
                <div style="font-size: 0.7rem; color: ${rep.abuse_score > 20 ? 'var(--alert)' : 'var(--accent)'}; font-weight: bold;">
                    REPUTATION SCORE: ${rep.abuse_score}%
                </div>
                <div style="font-size: 0.6rem; color: var(--dim);">REPORTS: ${rep.reports} | LAST: ${rep.last_report.substring(0,10)}</div>

                ${shodan.ports && shodan.ports.length > 0 ? `
                <hr style="border: 0; border-top: 1px solid #222; margin: 10px 0;">
                <div style="font-size: 0.7rem; color: #00ccff; font-weight: bold;">COUNTER-SCAN [SHODAN]:</div>
                <div style="font-size: 0.6rem; color: var(--dim);">OS: ${shodan.os || 'Unknown'}</div>
                <div style="font-size: 0.6rem; color: var(--accent);">OPEN PORTS: ${shodan.ports.join(', ')}</div>
                ` : ''}

                <div style="margin-top: 15px; display: flex; gap: 10px;">
                    <button class="layer-btn" style="flex: 1; border-color: var(--accent);" onclick="generateAiReport('${ip}')">GENERATE AI REPORT</button>
                    <button class="layer-btn" style="flex: 1; border-color: var(--danger); color: var(--danger);" onclick="triggerBlackout('${ip}')">BLACKOUT</button>
                </div>
                <div id="ai-report-box" style="margin-top: 10px; font-size: 0.55rem; color: var(--text); font-family: 'JetBrains Mono'; background: rgba(0,0,0,0.4); padding: 5px; border-radius: 4px; display: none; white-space: pre-wrap;"></div>

                ${history.length > 0 ? `
                <hr style="border: 0; border-top: 1px solid #222; margin: 10px 0;">
                <div style="font-size: 0.6rem; color: var(--accent); font-weight: bold; margin-bottom: 5px;">DEEP ACTIVITY LOG:</div>
                <div style="font-size: 0.5rem; color: #888; max-height: 100px; overflow-y: auto;">
                    ${history.map(h => `<div>[${h.time}] ${h.cmd}</div>`).join('')}
                </div>` : ''}
            `;
            focusTarget(ip, geo.lat, geo.lon);
        }

        async function generateAiReport(ip) {
            const box = document.getElementById('ai-report-box');
            box.style.display = 'block';
            box.innerText = 'GENERATING_REPORT...';
            const res = await fetch(`/api/ai_report?ip=${ip}`, { headers: { 'Authorization': `Bearer ${dashboardToken}` } });
            const data = await res.json();
            box.innerText = data.report;
        }

        async function triggerBlackout(ip) {
            if (!confirm(`INITIALIZE PERMANENT BLACKOUT FOR ${ip}?`)) return;
            const res = await fetch('/api/blackout', {
                method: 'POST',
                headers: { 'Authorization': `Bearer ${dashboardToken}`, 'Content-Type': 'application/json' },
                body: JSON.stringify({ ip: ip })
            });
            const data = await res.json();
            alert(data.msg);
            update();
        }

        // Global Threat Flux Chart
        const fluxCanvas = document.getElementById('flux-chart');
        const fluxCtx = fluxCanvas.getContext('2d');
        let fluxData = Array(30).fill(0);

        function updateFluxChart() {
            fluxCtx.clearRect(0, 0, fluxCanvas.width, fluxCanvas.height);
            fluxData.push(Math.random() * 50 + (Math.sin(Date.now()/500) * 10));
            fluxData.shift();

            fluxCtx.beginPath();
            fluxCtx.strokeStyle = '#00f2ff';
            fluxCtx.lineWidth = 2;
            for(let i=0; i<fluxData.length; i++) {
                const x = (i / fluxData.length) * fluxCanvas.width;
                const y = fluxCanvas.height - (fluxData[i]/100 * fluxCanvas.height);
                if(i===0) fluxCtx.moveTo(x, y); else fluxCtx.lineTo(x, y);
            }
            fluxCtx.stroke();

            // Fill
            fluxCtx.lineTo(fluxCanvas.width, fluxCanvas.height);
            fluxCtx.lineTo(0, fluxCanvas.height);
            fluxCtx.fillStyle = 'rgba(0, 242, 255, 0.1)';
            fluxCtx.fill();
        }

        setInterval(updateFluxChart, 200);

        async function handleTerm(e) {
            if (e.key === 'Enter') {
                const cmd = e.target.value;
                e.target.value = '';
                const res = await fetch('/api/terminal', {
                    method: 'POST',
                    headers: { 'Authorization': `Bearer ${dashboardToken}`, 'Content-Type': 'application/json' },
                    body: JSON.stringify({ command: cmd })
                });
                const data = await res.json();
                const feed = document.getElementById('feed');
                feed.innerHTML += `<div style="color: var(--accent); margin-top: 5px;">$ ${cmd}</div>`;
                feed.innerHTML += `<div style="color: #888; white-space: pre-wrap;">${data.output}</div>`;
                feed.scrollTop = feed.scrollHeight;
            }
        }

        function startDashboard() {
            setInterval(update, 5000);
            update();
        }
    </script>
</body>
</html>
"""

@app.route('/')
def index(): return render_template_string(HTML_TEMPLATE)

@app.route('/api/data')
def api_data():
    if not check_auth(): return jsonify({"error": "Unauthorized"}), 401
    return jsonify({
        "profile": get_current_profile(),
        "services": get_docker_stats(),
        "events": EVENT_HISTORY,
        "quorum": QUORUM_ALERTS + SYSTEM_ERRORS[-5:],
        "neural": [0.8, 0.4],
        "aircraft": AIRCRAFT_DATA,
        "system": get_system_metrics(),
        "deployment": get_deployment_status()
    })

@app.route('/api/intel')
def api_intel():
    if not check_auth(): return jsonify({"error": "Unauthorized"}), 401
    ip = request.args.get('ip')

    # Get historical activities for this IP
    history = []
    try:
        conn = sqlite3.connect(DB_FILE)
        c = conn.cursor()
        c.execute("SELECT activities.timestamp, activities.input FROM activities JOIN sessions ON activities.session_id = sessions.session_id WHERE sessions.ip = ? ORDER BY timestamp DESC LIMIT 20", (ip,))
        history = [{"time": r[0], "cmd": r[1]} for r in c.fetchall()]
        conn.close()
    except: pass

    return jsonify({
        "geo": resolve_geoip(ip),
        "rep": get_reputation(ip),
        "shodan": get_shodan_intel(ip),
        "history": history
    })

@app.route('/api/ai_report')
def api_ai_report():
    if not check_auth(): return jsonify({"error": "Unauthorized"}), 401
    ip = request.args.get('ip')
    # Fetch history
    history = []
    try:
        conn = sqlite3.connect(DB_FILE)
        c = conn.cursor()
        c.execute("SELECT activities.input FROM activities JOIN sessions ON activities.session_id = sessions.session_id WHERE sessions.ip = ? ORDER BY timestamp DESC LIMIT 50", (ip,))
        history = [{"cmd": r[0]} for r in c.fetchall()]
        conn.close()
    except: pass

    report = generate_ai_report(ip, history)
    return jsonify({"report": report})

@app.route('/api/uptime')
def api_uptime():
    if not check_auth(): return jsonify({"error": "Unauthorized"}), 401
    uptime_seconds = time.time() - psutil.boot_time()
    return jsonify({"uptime": time.strftime('%H:%M:%S', time.gmtime(uptime_seconds))})

@app.route('/api/terminal', methods=['POST'])
def api_terminal():
    if not check_auth(): return jsonify({"error": "Unauthorized"}), 401
    cmd = request.json.get('command')
    # SECURE PROXY: Execute ONLY in honeypot container, NOT on host VM
    try:
        # We target the 'cerberus-cowrie' container
        output = subprocess.check_output(["docker", "exec", "cerberus-cowrie", "bash", "-c", cmd], stderr=subprocess.STDOUT, timeout=5).decode()
        return jsonify({"output": output})
    except subprocess.CalledProcessError as e:
        return jsonify({"output": e.output.decode()})
@app.route('/api/blackout', methods=['POST'])
def api_blackout():
    if not check_auth(): return jsonify({"error": "Unauthorized"}), 401
    ip = request.json.get('ip')
    if not ip or ip in ['127.0.0.1', '8.8.8.8']:
        return jsonify({"msg": "ERROR: Cannot blackout protected source."})

    try:
        # 1. Local IPTables Block
        subprocess.run(["sudo", "iptables", "-I", "INPUT", "-s", ip, "-j", "DROP"], timeout=5)
        # 2. Kill Active Docker Sessions
        subprocess.run(["docker", "exec", "cerberus-cowrie", "bash", "-c", f"pkill -u cowrie"], timeout=5)

        QUORUM_ALERTS.append({"time": time.strftime('%H:%M:%S'), "msg": f"BLACKOUT_ACTIVE: {ip}"})
        return jsonify({"msg": f"SUCCESS: IP {ip} has been neutralized and blocked."})
    except Exception as e:
        return jsonify({"msg": f"ERROR: Neutralization failed ({str(e)})"})

if __name__ == "__main__":
    app.run(host='0.0.0.0', port=8080, threaded=True)
