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

# ==============================================================================
# CERBERUS COMMAND CENTER (V5.0 - PHOENIX TACTICAL HUD)
# ==============================================================================

app = Flask(__name__)

# Tactical Config
ADMIN_PASSWORD = "CERBERUS_THREAT_OMEGA_99X"
AUTH_TOKEN = hashlib.sha256(ADMIN_PASSWORD.encode()).hexdigest()

# Project Paths
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOG_FILE = os.path.join(BASE_DIR, "services/cowrie/logs/cowrie.json")
STATE_FILE = os.path.join(BASE_DIR, "build/morph-state.txt")
MORPH_BIN = os.path.join(BASE_DIR, "build/morph")
QUORUM_BIN = os.path.join(BASE_DIR, "build/quorum")

# Global State
HISTORY_FILE = os.path.join(BASE_DIR, "build/web_history.json")
EVENT_HISTORY = []
CREDENTIAL_COUNTS = {}
QUORUM_ALERTS = []
DASHBOARD_START_TIME = time.time()
SYSTEM_ERRORS = []
GEO_CACHE = {}
AIRCRAFT_DATA = []

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
        return {"city": "Local Network", "lat": 0, "lon": 0, "isp": "Internal", "org": "Private Cloud", "proxy": False}
    if ip in GEO_CACHE: return GEO_CACHE[ip]
    try:
        url = f"http://ip-api.com/json/{ip}?fields=status,message,country,regionName,city,lat,lon,isp,org,proxy,hosting,mobile"
        with urllib.request.urlopen(url, timeout=5) as response:
            data = json.loads(response.read().decode())
            if data.get("status") == "success":
                GEO_CACHE[ip] = data
                return data
    except Exception as e: log_system_error(f"GeoIP resolution failed for {ip}: {str(e)}")
    return {"city": "Unknown", "lat": 0, "lon": 0, "isp": "Unknown", "org": "Unknown", "proxy": False}

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
        return [{"name": l.split('|')[0], "stat": l.split('|')[1], "active": "Up" in l.split('|')[1]} for l in output.split('\n') if '|' in l]
    except: return []

# Background Logic: Monitoring and Parsing
def log_monitor():
    global EVENT_HISTORY, CREDENTIAL_COUNTS, QUORUM_ALERTS
    if not os.path.exists(LOG_FILE):
        os.makedirs(os.path.dirname(LOG_FILE), exist_ok=True)
        open(LOG_FILE, 'a').close()
    file_pos = os.path.getsize(LOG_FILE)
    while True:
        try:
            current_size = os.path.getsize(LOG_FILE)
            if current_size > file_pos:
                with open(LOG_FILE, 'r') as f:
                    f.seek(file_pos)
                    for line in f:
                        try:
                            data = json.loads(line.strip())
                            eventid = data.get('eventid')
                            ts = data.get('timestamp', '').split('T')[-1][:8]
                            src = data.get('src_ip', '127.0.0.1')
                            msg, etype = None, "info"
                            if eventid == "cowrie.session.connect":
                                geo = resolve_geoip(src)
                                msg = f"Inbound connection from {src} ({geo['city']})"
                            elif eventid == "cowrie.login.success":
                                msg = f"SUCCESSFUL LOGIN: {data.get('username')}"
                                etype = "success"
                            elif eventid == "cowrie.login.failed":
                                msg = f"FAILED LOGIN: {data.get('username')}:{data.get('password')}"
                                etype = "warn"
                            if msg:
                                EVENT_HISTORY.insert(0, {"time": ts, "msg": msg, "type": etype, "src": src})
                                EVENT_HISTORY = EVENT_HISTORY[:50]
                        except: continue
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
            <div class="card-header">Global Threat Flux</div>
            <div style="padding: 1rem; height: 120px;">
                <canvas id="flux-chart" width="280" height="80"></canvas>
            </div>
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
        </aside>
    </main>

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
        }

        async function fetchAndFocus(ip) {
            const res = await fetch(`/api/intel?ip=${ip}`, { headers: { 'Authorization': `Bearer ${dashboardToken}` } });
            const geo = await res.json();
            document.getElementById('target-details').innerHTML = `
                <div style="font-size: 1rem; color: var(--accent); font-weight: 800;">${ip}</div>
                <div style="font-size: 0.6rem; color: var(--dim); margin-top:0.4rem;">ORG: ${geo.isp}</div>
                <div style="font-size: 0.6rem; color: var(--dim);">LOC: ${geo.city}, ${geo.country}</div>
                <div style="font-size: 0.6rem; color: ${geo.proxy?'var(--danger)':'var(--success)'};">ANONYMITY: ${geo.proxy?'VPN_DETECTED':'CLEAR'}</div>
            `;
            focusTarget(ip, geo.lat, geo.lon);
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
        "aircraft": AIRCRAFT_DATA
    })

@app.route('/api/intel')
def api_intel():
    if not check_auth(): return jsonify({"error": "Unauthorized"}), 401
    ip = request.args.get('ip')
    return jsonify(resolve_geoip(ip))

@app.route('/api/uptime')
def api_uptime():
    if not check_auth(): return jsonify({"error": "Unauthorized"}), 401
    uptime_seconds = time.time() - psutil.boot_time()
    return jsonify({"uptime": time.strftime('%H:%M:%S', time.gmtime(uptime_seconds))})

if __name__ == "__main__":
    app.run(host='0.0.0.0', port=5000, threaded=True)
