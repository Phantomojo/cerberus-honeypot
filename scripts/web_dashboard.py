#!/usr/bin/env python3
import os
import subprocess
import threading
from datetime import datetime
from flask import Flask, render_template_string, jsonify

# ==============================================================================
# CERBERUS OPERATIONAL DASHBOARD (WEB)
# ==============================================================================

app = Flask(__name__)

# Project Paths
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOG_FILE = os.path.join(BASE_DIR, "services/cowrie/logs/cowrie.log")
STATE_FILE = os.path.join(BASE_DIR, "build/morph-state.txt")

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
    except:
        return []

def get_recent_events():
    events = []
    if os.path.exists(LOG_FILE):
        try:
            # Get last 10 interesting lines
            cmd = ["grep", "-E", "New connection|login attempt|CMD:", LOG_FILE]
            output = subprocess.check_output(cmd).decode().strip().split('\n')
            for line in reversed(output[-10:]):
                if not line: continue
                ts = line.split(']')[0][1:] if ']' in line else datetime.now().strftime("%H:%M:%S")
                msg = line.split(']')[-1].strip() if ']' in line else line
                
                type = "info"
                if "CMD:" in msg: type = "command"
                elif "succeeded" in msg: type = "success"
                elif "failed" in msg: type = "warn"
                
                events.append({"time": ts[:19], "msg": msg, "type": type})
        except: pass
    return events

# HTML Template with inline CSS/JS
HTML_TEMPLATE = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CERBERUS | Operational Dashboard</title>
    <script src="https://cdn.jsdelivr.net/npm/mermaid/dist/mermaid.min.js"></script>
    <link href="https://fonts.googleapis.com/css2?family=Outfit:wght@300;400;600;700&family=JetBrains+Mono:wght@400;700&display=swap" rel="stylesheet">
    <style>
        :root {
            --bg-dark: #0a0b10;
            --panel-bg: rgba(22, 24, 35, 0.7);
            --accent-primary: #00f2ff;
            --accent-secondary: #7000ff;
            --text-main: #e0e0e0;
            --text-dim: #94a3b8;
            --success: #10b981;
            --warning: #f59e0b;
            --danger: #ef4444;
            --glow: 0 0 20px rgba(0, 242, 255, 0.3);
        }

        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            background-color: var(--bg-dark);
            color: var(--text-main);
            font-family: 'Outfit', sans-serif;
            background-image: 
                radial-gradient(circle at 50% 50%, rgba(112, 0, 255, 0.05) 0%, transparent 50%),
                radial-gradient(circle at 10% 20%, rgba(0, 242, 255, 0.05) 0%, transparent 40%);
            min-height: 100vh;
            overflow-x: hidden;
        }

        header {
            padding: 2rem;
            display: flex;
            justify-content: space-between;
            align-items: center;
            border-bottom: 1px solid rgba(255, 255, 255, 0.1);
            backdrop-filter: blur(10px);
            position: sticky;
            top: 0;
            z-index: 100;
        }

        .logo-area { display: flex; align-items: center; gap: 1rem; }
        .logo-ring {
            width: 40px; height: 40px;
            border: 3px solid var(--accent-primary);
            border-radius: 50%;
            display: flex; align-items: center; justify-content: center;
            box-shadow: var(--glow);
            animation: pulse-ring 2s infinite;
        }
        @keyframes pulse-ring {
            0% { transform: scale(1); opacity: 1; }
            50% { transform: scale(1.1); opacity: 0.8; }
            100% { transform: scale(1); opacity: 1; }
        }

        .title h1 {
            font-size: 1.5rem; letter-spacing: 4px; font-weight: 700;
            background: linear-gradient(90deg, var(--accent-primary), var(--accent-secondary));
            -webkit-background-clip: text; -webkit-text-fill-color: transparent;
        }

        .status-badge {
            padding: 0.5rem 1rem; border-radius: 20px;
            background: rgba(16, 185, 129, 0.1);
            color: var(--success); font-weight: 600; font-size: 0.8rem;
            display: flex; align-items: center; gap: 0.5rem;
            border: 1px solid rgba(16, 185, 129, 0.3);
        }
        .status-dot { width: 8px; height: 8px; background: var(--success); border-radius: 50%; animation: blink 1s infinite; }
        @keyframes blink { 0% { opacity: 0.2; } 50% { opacity: 1; } 100% { opacity: 0.2; } }

        main { padding: 2rem; display: grid; grid-template-columns: 2fr 1fr; gap: 2rem; max-width: 1600px; margin: 0 auto; }

        .dashboard-grid { display: flex; flex-direction: column; gap: 2rem; }

        .stats-row { display: grid; grid-template-columns: repeat(3, 1fr); gap: 1.5rem; }
        .card {
            background: var(--panel-bg);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 16px;
            padding: 1.5rem;
            backdrop-filter: blur(12px);
            transition: transform 0.3s ease, border-color 0.3s ease;
        }
        .card:hover { border-color: var(--accent-primary); transform: translateY(-5px); }
        .card-label { font-size: 0.8rem; color: var(--text-dim); text-transform: uppercase; letter-spacing: 1px; margin-bottom: 0.5rem; }
        .card-value { font-size: 1.8rem; font-weight: 700; color: var(--accent-primary); }

        .topology-card { grid-column: span 2; padding: 2rem; min-height: 400px; display: flex; flex-direction: column; }
        .mermaid { flex-grow: 1; display: flex; align-items: center; justify-content: center; }

        .service-list { display: flex; flex-direction: column; gap: 1rem; margin-top: 1rem; }
        .service-item {
            display: flex; justify-content: space-between; align-items: center;
            padding: 1rem; border-radius: 12px; background: rgba(0,0,0,0.2);
            border: 1px solid rgba(255,255,255,0.05);
        }
        .svc-info h4 { font-size: 0.9rem; margin-bottom: 0.2rem; }
        .svc-info p { font-size: 0.7rem; color: var(--text-dim); }
        .svc-status { font-size: 0.7rem; font-weight: 600; padding: 0.3rem 0.6rem; border-radius: 4px; }
        .svc-status.up { background: rgba(16, 185, 129, 0.1); color: var(--success); }
        .svc-status.down { background: rgba(239, 68, 68, 0.1); color: var(--danger); }

        .event-feed { display: flex; flex-direction: column; gap: 1rem; height: 100%; }
        .feed-container { 
            flex-grow: 1; overflow-y: auto; max-height: 700px; 
            padding-right: 0.5rem; scrollbar-width: thin; scrollbar-color: var(--accent-primary) transparent;
        }
        .event-item {
            padding: 1rem; border-radius: 12px; background: rgba(0,0,0,0.2);
            border-left: 4px solid var(--text-dim); margin-bottom: 1rem;
            font-family: 'JetBrains Mono', monospace; font-size: 0.8rem;
        }
        .event-item.success { border-left-color: var(--success); }
        .event-item.warn { border-left-color: var(--warning); }
        .event-item.command { border-left-color: var(--accent-primary); }
        .event-time { color: var(--text-dim); font-size: 0.7rem; margin-bottom: 0.4rem; }

        @media (max-width: 1100px) {
            main { grid-template-columns: 1fr; }
            .stats-row { grid-template-columns: 1fr; }
        }
    </style>
</head>
<body>
    <header>
        <div class="logo-area">
            <div class="logo-ring">
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/>
                </svg>
            </div>
            <div class="title">
                <h1>CERBERUS</h1>
                <p style="font-size: 0.6rem; color: var(--text-dim); letter-spacing: 2px;">BIO-ADAPTIVE HONEYNET</p>
            </div>
        </div>
        <div class="status-badge">
            <div class="status-dot"></div>
            CORE ENGINE STATUS: ACTIVE
        </div>
    </header>

    <main>
        <section class="dashboard-grid">
            <div class="stats-row">
                <div class="card">
                    <div class="card-label">CURRENT PROFILE</div>
                    <div class="card-value" id="val-profile">...</div>
                </div>
                <div class="card">
                    <div class="card-label">MORCH PHASE</div>
                    <div class="card-value">6 / 6</div>
                </div>
                <div class="card">
                    <div class="card-label">SESSIONS</div>
                    <div class="card-value" id="val-sessions">0</div>
                </div>
            </div>

            <div class="card topology-card">
                <div class="card-label">HONEYNET TOPOLOGY</div>
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
        RTSP[RTSP Stream]
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

            <div class="card">
                <div class="card-label">DOCKER SERVICE GRID</div>
                <div class="service-list" id="service-grid">
                    <!-- Dynamic Services Here -->
                </div>
            </div>
        </section>

        <section class="event-feed">
            <div class="card" style="height: 100%;">
                <div class="card-label">REAL-TIME FEEDS</div>
                <div class="feed-container" id="event-feed">
                    <div class="event-item">Initializing system listeners...</div>
                </div>
            </div>
        </section>
    </main>

    <script>
        mermaid.initialize({ startOnLoad: true, theme: 'dark' });

        async function updateDashboard() {
            try {
                const response = await fetch('/api/data');
                const data = await response.json();

                // Update Profile
                document.getElementById('val-profile').innerText = data.profile;
                
                // Update Sessions count
                document.getElementById('val-sessions').innerText = data.events.filter(e => e.msg.includes('connection')).length;

                // Update Service Grid
                const grid = document.getElementById('service-grid');
                grid.innerHTML = data.services.map(s => `
                    <div class="service-item">
                        <div class="svc-info">
                            <h4>${s.name}</h4>
                            <p>${s.image}</p>
                        </div>
                        <div class="svc-status ${s.active ? 'up' : 'down'}">
                            ${s.active ? 'ACTIVE' : 'OFFLINE'}
                        </div>
                    </div>
                `).join('');

                // Update Event Feed
                const feed = document.getElementById('event-feed');
                feed.innerHTML = data.events.map(e => `
                    <div class="event-item ${e.type}">
                        <div class="event-time">${e.time}</div>
                        <div class="event-text">${e.msg}</div>
                    </div>
                `).join('');

            } catch (e) { console.error("Poll error", e); }
        }

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
        "events": get_recent_events()
    })

if __name__ == "__main__":
    print("🚀 Cerberus Operational Dashboard starting on http://localhost:5000")
    # Using threaded mode for development speed
    app.run(host='0.0.0.0', port=5000, threaded=True)
