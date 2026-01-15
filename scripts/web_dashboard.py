#!/usr/bin/env python3
import os
import subprocess
import threading
import time
import json
import random
from datetime import datetime
from flask import Flask, render_template_string, jsonify, request

# ==============================================================================
# CERBERUS COMMAND CENTER (WEB)
# ==============================================================================

app = Flask(__name__)

# Project Paths
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOG_FILE = os.path.join(BASE_DIR, "services/cowrie/logs/cowrie.json")
STATE_FILE = os.path.join(BASE_DIR, "build/morph-state.txt")
MORPH_BIN = os.path.join(BASE_DIR, "build/morph")
QUORUM_BIN = os.path.join(BASE_DIR, "build/quorum")

# Global State with Persistence
HISTORY_FILE = os.path.join(BASE_DIR, "build/web_history.json")
EVENT_HISTORY = []
CREDENTIAL_COUNTS = {}
ACTIVE_SESSIONS = {}
QUORUM_ALERTS = []

def load_persistence():
    global EVENT_HISTORY, CREDENTIAL_COUNTS, QUORUM_ALERTS
    if os.path.exists(HISTORY_FILE):
        try:
            with open(HISTORY_FILE, 'r') as f:
                data = json.load(f)
                EVENT_HISTORY = data.get('events', [])
                CREDENTIAL_COUNTS = data.get('credentials', {})
                QUORUM_ALERTS = data.get('quorum', [])
        except: pass

def save_persistence():
    try:
        with open(HISTORY_FILE, 'w') as f:
            json.dump({
                "events": EVENT_HISTORY[:50],
                "credentials": CREDENTIAL_COUNTS,
                "quorum": QUORUM_ALERTS[-10:]
            }, f)
    except: pass

load_persistence()

# Mock Locations for 127.0.0.1 visuals
MOCK_LOCATIONS = [
    {"city": "Singapore", "lat": 1.35, "lng": 103.8},
    {"city": "Frankfurt", "lat": 50.11, "lng": 8.68},
    {"city": "San Francisco", "lat": 37.77, "lng": -122.41},
    {"city": "Tokyo", "lat": 35.68, "lng": 139.65},
    {"city": "London", "lat": 51.50, "lng": -0.12},
    {"city": "Sydney", "lat": -33.86, "lng": 151.2},
]

def get_current_profile():
    if os.path.exists(STATE_FILE):
        try:
            with open(STATE_FILE, 'r') as f:
                c = f.read().strip()
                return c.split('=')[1] if '=' in c else c
        except: return "Generic Device"
    return "Initializing..."

def get_docker_stats():
    try:
        cmd = ["docker", "ps", "--format", "{{.Names}}|{{.Status}}|{{.Image}}"]
        output = subprocess.check_output(cmd).decode().strip()
        services = []
        for line in output.split('\n'):
            if '|' in line:
                name, stat, img = line.split('|')
                services.append({
                    "name": name,
                    "status": stat,
                    "image": img,
                    "active": "Up" in stat
                })
        return services
    except: return []

# Background Logic: Monitoring and Parsing
def log_monitor():
    global EVENT_HISTORY, CREDENTIAL_COUNTS, QUORUM_ALERTS

    if not os.path.exists(LOG_FILE):
        os.makedirs(os.path.dirname(LOG_FILE), exist_ok=True)
        open(LOG_FILE, 'a').close()

    file_pos = os.path.getsize(LOG_FILE)

    while True:
        # 1. Check Cowrie JSON Logs
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

                        msg = None
                        etype = "info"

                        if eventid == "cowrie.session.connect":
                            loc = random.choice(MOCK_LOCATIONS)
                            msg = f"Inbound connection from {src} ({loc['city']})"
                            etype = "info"
                        elif eventid == "cowrie.login.success":
                            user = data.get('username', 'root')
                            pwd = data.get('password', 'admin')
                            msg = f"SUCCESSFUL LOGIN: {user}:{pwd}"
                            etype = "success"
                            CREDENTIAL_COUNTS[pwd] = CREDENTIAL_COUNTS.get(pwd, 0) + 1
                        elif eventid == "cowrie.login.failed":
                            user = data.get('username', 'root')
                            pwd = data.get('password', 'guest')
                            msg = f"Login Failed: {user}:{pwd}"
                            etype = "warn"
                            CREDENTIAL_COUNTS[pwd] = CREDENTIAL_COUNTS.get(pwd, 0) + 1
                        elif eventid == "cowrie.command.input":
                            msg = f"CMD: {data.get('input')}"
                            etype = "command"

                        if msg:
                            EVENT_HISTORY.insert(0, {"time": ts, "msg": msg, "type": etype, "src": src})
                            EVENT_HISTORY = EVENT_HISTORY[:50] # Keep last 50
                            save_persistence()
                    except: continue
            file_pos = current_size

        # 2. Check Quorum Engine
        try:
            res = subprocess.run([QUORUM_BIN], capture_output=True, text=True, timeout=2)
            if res.returncode == 1:
                alert = res.stdout.split("ALERT:")[1].split("---")[0].strip() if "ALERT:" in res.stdout else "Coordinated attack detected!"
                if not any(a['msg'] == alert for a in QUORUM_ALERTS[-5:]):
                    QUORUM_ALERTS.append({"time": datetime.now().strftime("%H:%M:%S"), "msg": alert})
                    save_persistence()
        except: pass

        time.sleep(1)

# Start background monitor
threading.Thread(target=log_monitor, daemon=True).start()

# HTML Template
HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>CERBERUS | COMMAND CENTER</title>
    <script src="https://cdn.jsdelivr.net/npm/mermaid/dist/mermaid.min.js"></script>
    <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;700&family=JetBrains+Mono:wght@400;700&display=swap" rel="stylesheet">
    <style>
        :root {
            --bg-dark: #05060a;
            --panel-bg: rgba(15, 18, 28, 0.85);
            --accent-primary: #00f2ff;
            --accent-red: #ff3e3e;
            --accent-purple: #7000ff;
            --text-main: #e2e8f0;
            --text-dim: #64748b;
            --success: #10b981;
            --danger: #ef4444;
            --glass-border: 1px solid rgba(255, 255, 255, 0.1);
        }

        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            background-color: var(--bg-dark);
            color: var(--text-main);
            font-family: 'Outfit', sans-serif;
            background-image:
                radial-gradient(circle at 10% 10%, rgba(0, 242, 255, 0.05) 0%, transparent 40%),
                radial-gradient(circle at 90% 90%, rgba(112, 0, 255, 0.05) 0%, transparent 40%);
            min-height: 100vh;
            overflow: hidden;
        }

        /* Header & Navigation */
        header {
            height: 70px; display: flex; align-items: center; justify-content: space-between;
            padding: 0 2rem; border-bottom: var(--glass-border); background: rgba(0,0,0,0.5); backdrop-filter: blur(10px);
        }
        .logo { display: flex; align-items: center; gap: 1rem; }
        .logo-box { width: 32px; height: 32px; border: 2px solid var(--accent-primary); border-radius: 4px; display: flex; align-items: center; justify-content: center; box-shadow: 0 0 15px rgba(0,242,255,0.4); }
        .logo h1 { font-size: 1.2rem; letter-spacing: 5px; font-weight: 800; }

        .morph-control {
            background: rgba(239, 68, 68, 0.1); border: 1px solid var(--danger); color: var(--danger);
            padding: 0.6rem 1.2rem; border-radius: 8px; font-weight: 700; font-size: 0.8rem;
            cursor: pointer; transition: all 0.3s ease; letter-spacing: 1px;
        }
        .morph-control:hover { background: var(--danger); color: white; box-shadow: 0 0 20px rgba(239, 68, 68, 0.4); transform: scale(1.05); }
        .morph-control:active { transform: scale(0.95); }

        /* Main Grid */
        main {
            display: grid; grid-template-columns: 280px 1fr 340px; height: calc(100vh - 70px); gap: 1rem; padding: 1rem;
        }

        .panel { background: var(--panel-bg); border: var(--glass-border); border-radius: 12px; display: flex; flex-direction: column; overflow: hidden; }
        .panel-header { padding: 1rem; border-bottom: var(--glass-border); font-size: 0.7rem; color: var(--text-dim); text-transform: uppercase; letter-spacing: 2px; font-weight: 700; display: flex; justify-content: space-between; align-items: center; }

        /* Left Panel: Inventory & Status */
        .inventory-list { padding: 1rem; display: flex; flex-direction: column; gap: 1rem; }
        .status-card { background: rgba(0,0,0,0.3); padding: 1rem; border-radius: 8px; border-left: 3px solid var(--accent-primary); }
        .status-card h4 { font-size: 0.7rem; color: var(--text-dim); margin-bottom: 0.2rem; }
        .status-card p { font-size: 1.1rem; font-weight: 700; color: var(--accent-primary); }

        .svc-item { display: flex; justify-content: space-between; align-items: center; font-size: 0.75rem; padding: 0.7rem; background: rgba(255,255,255,0.02); border-radius: 6px; margin-bottom: 0.5rem; border: 1px solid rgba(255,255,255,0.05); }
        .svc-name { color: var(--text-main); font-weight: 700; letter-spacing: 1px; font-family: 'JetBrains Mono', monospace; }
        .svc-status { display: flex; align-items: center; gap: 0.5rem; }
        .svc-badge { font-size: 0.6rem; padding: 2px 6px; border-radius: 4px; font-weight: 800; text-transform: uppercase; }
        .badge-up { background: rgba(16, 185, 129, 0.2); color: var(--success); border: 1px solid var(--success); }
        .badge-down { background: rgba(239, 68, 68, 0.2); color: var(--danger); border: 1px solid var(--danger); }
        .svc-led { width: 6px; height: 6px; border-radius: 50%; animation: pulse 2s infinite; }
        @keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.4; } 100% { opacity: 1; } }

        /* Center: World Map & Topology */
        .center-stack { display: grid; grid-template-rows: 1fr 1fr; gap: 1rem; }

        .map-area { position: relative; display: flex; align-items: center; justify-content: center; overflow: hidden; background: radial-gradient(circle at center, #0a0b14 0%, #05060a 100%); }
        .map-bg { width: 95%; height: 95%; opacity: 0.15; filter: drop-shadow(0 0 10px var(--accent-primary)); }
        .map-overlay { position: absolute; top: 0; left: 0; width: 100%; height: 100%; pointer-events: none; }
        .attack-dot { position: absolute; width: 6px; height: 6px; background: var(--accent-red); border-radius: 50%; box-shadow: 0 0 15px var(--accent-red); animation: ping 2s infinite; }
        @keyframes ping { 0% { transform: scale(1); opacity: 1; } 100% { transform: scale(5); opacity: 0; } }

        .topology-area { padding: 0; overflow: hidden; display: flex; align-items: center; justify-content: center; background: rgba(0,0,0,0.2); }
        .mermaid { transform: scale(0.85); transform-origin: center; filter: drop-shadow(0 0 5px var(--accent-primary)); }

        /* Right Panel: Feeds & Intelligence */
        .intel-stack { display: grid; grid-template-rows: 2fr 1fr; gap: 1rem; overflow: hidden; }

        .log-feed { flex-grow: 1; overflow-y: auto; padding: 1rem; font-family: 'JetBrains Mono', monospace; font-size: 0.75rem; scrollbar-width: none; }
        .log-item { margin-bottom: 0.8rem; padding-bottom: 0.5rem; border-bottom: 1px solid rgba(255,255,255,0.03); }
        .log-ts { color: var(--accent-primary); margin-right: 0.5rem; }
        .log-item.success { color: var(--success); }
        .log-item.warn { color: #facc15; }
        .log-item.command { color: #fb923c; }

        .threat-board { padding: 1rem; background: rgba(239, 68, 68, 0.05); }
        .threat-item { font-size: 0.75rem; color: var(--danger); padding: 0.4rem 0; font-weight: 600; border-left: 2px solid var(--danger); padding-left: 0.5rem; margin-bottom: 0.5rem; animation: flicker 0.5s infinite; }
        @keyframes flicker { 0% { opacity: 0.5; } 50% { opacity: 1; } 100% { opacity: 0.5; } }

        .heatmap { padding: 1rem; }
        .heat-row { display: flex; justify-content: space-between; font-size: 0.8rem; margin-bottom: 0.4rem; padding: 0.3rem 0; }
        .heat-bar { height: 4px; background: var(--accent-purple); border-radius: 2px; transition: width 0.5s ease; }

        /* Alerts Overlay */
        #flash-overlay {
            position: fixed; top: 0; left: 0; width: 100%; height: 100%;
            pointer-events: none; border: 10px solid var(--danger);
            opacity: 0; z-index: 999;
        }
        .critical-flash { animation: screen-flash 1s 3; }
        @keyframes screen-flash { 0% { opacity: 0; } 50% { opacity: 0.3; } 100% { opacity: 0; } }
    </style>
</head>
<body>
    <div id="flash-overlay"></div>

    <header>
        <div class="logo">
            <div class="logo-box">
                <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3">
                    <path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5M2 12l10 5 10-5"/>
                </svg>
            </div>
            <h1>CERBERUS</h1>
        </div>
        <div style="display: flex; gap: 2rem; align-items: center;">
            <div id="uptime" style="font-family: 'JetBrains Mono'; font-size: 0.8rem; color: var(--text-dim);">SYSTEM_UP: 00:00:00</div>
            <button class="morph-control" onclick="triggerMorph()">⚡ TRIGGER INITIALIZE MORPH</button>
        </div>
    </header>

    <main>
        <!-- LEFT PANEL -->
        <aside class="panel">
            <div class="panel-header">System Health <span>V6.2</span></div>
            <div class="inventory-list">
                <div class="status-card">
                    <h4>DEVICE IDENTITY</h4>
                    <p id="val-profile">LOADING...</p>
                </div>
                <div class="status-card" style="border-left-color: var(--accent-purple);">
                    <h4>ACTIVE THREATS</h4>
                    <p id="val-threats">0</p>
                </div>

                <div style="margin-top: 1rem;">
                    <div class="panel-header" style="padding: 0.5rem 0; border: none;">Service Grid</div>
                    <div id="service-grid">
                        <!-- Services dynamically here -->
                    </div>
                </div>

                <div style="margin-top: 1rem;">
                    <div class="panel-header" style="padding: 0.5rem 0; border: none;">Credential Heatmap</div>
                    <div id="heatmap-list">
                        <!-- Top PWs here -->
                    </div>
                </div>
            </div>
        </aside>

        <!-- CENTER AREA -->
        <div class="center-stack">
            <div class="panel map-area">
                <!-- Simplified World Map SVG (Embedded for Reliability) -->
                <svg class="map-bg" viewBox="0 0 1000 500" xmlns="http://www.w3.org/2000/svg">
                    <path d="M150,150 L180,140 L200,160 L220,150 L250,180 L230,220 L200,240 L160,230 L140,200 Z" fill="currentColor" opacity="0.4"/>
                    <path d="M400,100 L450,110 L480,150 L500,200 L480,250 L420,280 L380,240 L350,180 L380,130 Z" fill="currentColor" opacity="0.4"/>
                    <path d="M700,200 L750,180 L800,200 L820,250 L780,300 L720,320 L680,280 L670,230 Z" fill="currentColor" opacity="0.4"/>
                    <path d="M200,350 L250,340 L300,380 L280,450 L220,460 L180,420 Z" fill="currentColor" opacity="0.4"/>
                    <path d="M750,380 L800,370 L830,410 L810,460 L760,450 L730,410 Z" fill="currentColor" opacity="0.4"/>
                    <!-- Decorative Dots for 'Tech' look -->
                    <circle cx="200" cy="150" r="2" fill="var(--accent-primary)"/>
                    <circle cx="450" cy="200" r="2" fill="var(--accent-primary)"/>
                    <circle cx="750" cy="250" r="2" fill="var(--accent-primary)"/>
                    <circle cx="100" cy="100" r="1" fill="var(--text-dim)"/>
                    <circle cx="900" cy="400" r="1" fill="var(--text-dim)"/>
                </svg>
                <div class="map-overlay" id="map-pings"></div>
                <div style="position: absolute; bottom: 1rem; left: 1rem; font-size: 0.6rem; color: var(--text-dim); text-transform: uppercase; letter-spacing: 2px;">
                    <span style="color: var(--accent-primary); animation: flicker 2s infinite;">●</span> GEO-SIMULATION: ACTIVE (Local Projection)
                </div>
            </div>

            <div class="panel topology-area">
                <div class="mermaid" id="topo-graph">
graph TD
    subgraph Deception Core
        Core[Cerberus Core]
        Morph[Morphing Engine]
        Quorum[Quorum Engine]
    end

    subgraph IoT Interfaces
        Cowrie[Cowrie SSH]
        Router[Fake Router Admin]
        Camera[Fake Camera UI]
    end

    subgraph Intelligence
        GPT[HoneyGPT AI]
        Ollama[Llama-3 LLM]
    end

    Core --> Morph
    Core --> Quorum
    Morph -.-> Cowrie
    Morph -.-> Router
    Morph -.-> Camera
    Cowrie <--> GPT
    GPT <--> Ollama

    style Core fill:#00f2ff22,stroke:#00f2ff,stroke-width:2px;
    style Cowrie fill:#7000ff22,stroke:#7000ff,stroke-width:2px;
    style GPT fill:#10b98122,stroke:#10b981,stroke-width:2px;
    style Router fill:#f59e0b22,stroke:#f59e0b;
                </div>
            </div>
        </div>

        <!-- RIGHT PANEL -->
        <aside class="intel-stack">
            <div class="panel">
                <div class="panel-header">Live Telemetry <span>Auto-Sync</span></div>
                <div class="log-feed" id="log-feed">
                    <!-- Events here -->
                </div>
            </div>

            <div class="panel">
                <div class="panel-header" style="color: var(--danger);">Quorum Threat Board</div>
                <div class="threat-board" id="threat-list">
                    <div style="color: var(--text-dim); font-size: 0.7rem; text-align: center; margin-top: 1rem;">No coordinated attacks detected.</div>
                </div>
            </div>
        </aside>
    </main>

    <script>
        mermaid.initialize({ startOnLoad: true, theme: 'dark' });

        async function triggerMorph() {
            if(!confirm("Warning: This will rotate the honeypot's entire identity and drop current sessions. Proceed?")) return;
            const btn = document.querySelector('.morph-control');
            btn.innerText = "🔄 MORPHING... PLEASE WAIT";
            btn.style.opacity = '0.5';

            try {
                const res = await fetch('/api/morph', {method: 'POST'});
                const data = await res.json();
                if(data.success) {
                    setTimeout(() => {
                        window.location.reload();
                    }, 2000);
                }
            } catch(e) { alert("Morph Error: " + e); }
        }

        async function updateDashboard() {
            try {
                const response = await fetch('/api/data');
                const data = await response.json();

                // 1. Profile & Basic Stats
                document.getElementById('val-profile').innerText = data.profile;
                document.getElementById('val-threats').innerText = data.quorum.length;

                if (data.quorum.length > 0) {
                    document.getElementById('flash-overlay').classList.add('critical-flash');
                } else {
                    document.getElementById('flash-overlay').classList.remove('critical-flash');
                }

                // 2. Service Grid
                const grid = document.getElementById('service-grid');
                grid.innerHTML = data.services.map(s => `
                    <div class="svc-item">
                        <span class="svc-name">${s.name.replace('cerberus-', '')}</span>
                        <div class="svc-status">
                            <span class="svc-badge ${s.active ? 'badge-up' : 'badge-down'}">${s.active ? 'UP' : 'DOWN'}</span>
                            <div class="svc-led" style="background: ${s.active ? 'var(--success)' : 'var(--danger)'}; box-shadow: 0 0 8px ${s.active ? 'var(--success)' : 'var(--danger)'}"></div>
                        </div>
                    </div>
                `).join('');

                // 3. Log Feed
                const feed = document.getElementById('log-feed');
                feed.innerHTML = data.events.map(e => `
                    <div class="log-item ${e.type}">
                        <span class="log-ts">${e.time}</span>
                        <span>${e.msg}</span>
                    </div>
                `).join('');

                // 4. Heatmap
                const heat = document.getElementById('heatmap-list');
                const max = Math.max(...Object.values(data.heatmap), 1);
                heat.innerHTML = Object.entries(data.heatmap)
                    .sort((a,b) => b[1] - a[1])
                    .slice(0, 5)
                    .map(([pw, count]) => `
                        <div class="heat-row">
                            <span style="font-family: 'JetBrains Mono';">${pw}</span>
                            <span style="color: var(--text-dim);">${count} hits</span>
                        </div>
                        <div class="heat-bar" style="width: ${(count/max)*100}%"></div>
                    `).join('');

                // 5. Threat Board
                const threats = document.getElementById('threat-list');
                if (data.quorum.length > 0) {
                    threats.innerHTML = data.quorum.map(q => `
                        <div class="threat-item">[${q.time}] ALERT: ${q.msg}</div>
                    `).join('');
                }

                // 6. Map Pings
                const map = document.getElementById('map-pings');
                if (data.events.length > 0 && Math.random() > 0.7) {
                    const ping = document.createElement('div');
                    ping.className = 'attack-dot';
                    ping.style.left = Math.random() * 90 + '%';
                    ping.style.top = Math.random() * 80 + '%';
                    map.appendChild(ping);
                    setTimeout(() => ping.remove(), 2000);
                }

            } catch (e) { console.error("Poll error", e); }
        }

        // Timer
        let start = Date.now();
        setInterval(() => {
            let diff = Math.floor((Date.now() - start) / 1000);
            let h = String(Math.floor(diff/3600)).padStart(2,'0');
            let m = String(Math.floor((diff%3600)/60)).padStart(2,'0');
            let s = String(diff%60).padStart(2,'0');
            document.getElementById('uptime').innerText = `SYSTEM_UP: ${h}:${m}:${s}`;
        }, 1000);

        setInterval(updateDashboard, 2000);
        updateDashboard();
    </script>
</body>
</html>
"""

@app.route('/')
def index():
    return render_template_string(HTML_TEMPLATE)

@app.route('/api/data')
def api_data():
    return jsonify({
        "profile": get_current_profile(),
        "services": get_docker_stats(),
        "events": EVENT_HISTORY,
        "heatmap": CREDENTIAL_COUNTS,
        "quorum": QUORUM_ALERTS
    })

@app.route('/api/morph', methods=['POST'])
def trigger_morph():
    try:
        # Stop existing cowrie to allow morph to run
        subprocess.run(["docker", "stop", "cerberus-cowrie"], timeout=10)
        # Execute morph binary
        subprocess.run([MORPH_BIN], timeout=15)
        # Restart cowrie
        subprocess.run(["docker", "start", "cerberus-cowrie"], timeout=10)
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"success": False, "error": str(e)})

if __name__ == "__main__":
    print("🚀 CERBERUS COMMAND CENTER starting on http://localhost:5000")
    app.run(host='0.0.0.0', port=5000, threaded=True)
