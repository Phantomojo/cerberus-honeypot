#!/usr/bin/env python3
import os
import time
import subprocess
import threading
from datetime import datetime

# ANSI Colors
CLR_RESET = "\033[0m"
CLR_BOLD  = "\033[1m"
CLR_RED   = "\033[31m"
CLR_GRN   = "\033[32m"
CLR_YLW   = "\033[33m"
CLR_BLU   = "\033[34m"
CLR_CYN   = "\033[36m"
CLR_WHT   = "\033[37m"

# Project Paths
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOG_FILE = os.path.join(BASE_DIR, "services/cowrie/logs/cowrie.log")
STATE_FILE = os.path.join(BASE_DIR, "build/morph-state.txt")
MORPH_LOG = os.path.join(BASE_DIR, "build/morph-events.log")

class CerberusDashboard:
    def __init__(self):
        self.lock = threading.Lock()
        self.ssh_events = []
        self.max_events = 10
        self.current_profile = "UNKNOWN"
        self.docker_status = {}
        self.running = True

    def get_docker_status(self):
        try:
            cmd = ["docker", "ps", "--format", "{{.Names}}|{{.Status}}"]
            output = subprocess.check_output(cmd).decode().strip()
            status = {}
            for line in output.split('\n'):
                if '|' in line:
                    name, stat = line.split('|')
                    status[name] = stat
            return status
        except:
            return {}

    def get_current_profile(self):
        try:
            if os.path.exists(STATE_FILE):
                with open(STATE_FILE, 'r') as f:
                    content = f.read().strip()
                    # Expecting current_profile=X
                    if '=' in content:
                        return content.split('=')[1]
                    return content
            return "N/A"
        except:
            return "ERR"

    def tail_logs(self):
        """Threaded function to tail Cowrie logs"""
        if not os.path.exists(LOG_FILE):
            # Create empty if not exists
            os.makedirs(os.path.dirname(LOG_FILE), exist_ok=True)
            open(LOG_FILE, 'a').close()

        # Simple tail -f implementation
        with subprocess.Popen(['tail', '-n', '0', '-F', LOG_FILE], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True) as p:
            while self.running:
                line = p.stdout.readline()
                if line:
                    line = line.strip()
                    # Filter for interesting events
                    if "New connection" in line or "login attempt" in line or "CMD:" in line:
                        with self.lock:
                            timestamp = datetime.now().strftime("%H:%M:%S")
                            clean_line = line.split(']')[-1].strip() if ']' in line else line
                            self.ssh_events.append(f"[{timestamp}] {clean_line}")
                            if len(self.ssh_events) > self.max_events:
                                self.ssh_events.pop(0)
                else:
                    time.sleep(0.1)

    def draw(self):
        os.system('clear')
        print(f"{CLR_BOLD}{CLR_CYN}╔══════════════════════════════════════════════════════════════════════════════╗{CLR_RESET}")
        print(f"{CLR_BOLD}{CLR_CYN}║              🛡️  CERBERUS LIVE DECEPTION DASHBOARD                    ║{CLR_RESET}")
        print(f"{CLR_BOLD}{CLR_CYN}╚══════════════════════════════════════════════════════════════════════════════╝{CLR_RESET}")

        # Top Section: Info
        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        profile = self.get_current_profile()
        print(f" TIME: {CLR_WHT}{now}{CLR_RESET} | PROFILE: {CLR_BOLD}{CLR_GRN}{profile}{CLR_RESET} | STATUS: {CLR_BOLD}{CLR_GRN}ONLINE{CLR_RESET}")
        print("─" * 80)

        # Docker Status
        print(f"{CLR_BOLD}[ DOCKER SERVICES ]{CLR_RESET}")
        self.docker_status = self.get_docker_status()
        services = ["cerberus-cowrie", "cerberus-router-web", "cerberus-camera-web", "cerberus-rtsp"]

        for svc in services:
            stat = self.docker_status.get(svc, f"{CLR_RED}Offline{CLR_RESET}")
            icon = "🟢" if "Up" in stat else "🔴"
            color = CLR_GRN if "Up" in stat else CLR_RED
            print(f" {icon} {svc:<25} : {color}{stat}{CLR_RESET}")

        print("─" * 80)

        # Live SSH Events
        print(f"{CLR_BOLD}[ LIVE SSH ACTIVITY (Real-time) ]{CLR_RESET}")
        with self.lock:
            if not self.ssh_events:
                print(f" {CLR_YLW}Waiting for attacker activity...{CLR_RESET}")
            for event in reversed(self.ssh_events):
                if "login attempt" in event and "succeeded" in event:
                    print(f" {CLR_GRN}{event}{CLR_RESET}")
                elif "CMD:" in event:
                    print(f" {CLR_CYN}{event}{CLR_RESET}")
                elif "New connection" in event:
                    print(f" {event}")
                else:
                    print(f" {event}")

        print("─" * 80)
        print(f" Press {CLR_BOLD}Ctrl+C{CLR_RESET} to exit dashboard | Refresh: 1s")

    def run(self):
        # Start log tailing thread
        self.tail_thread = threading.Thread(target=self.tail_logs, daemon=True)
        self.tail_thread.start()

        try:
            while True:
                self.draw()
                time.sleep(1)
        except KeyboardInterrupt:
            self.running = False
            print("\nExiting Cerberus Dashboard...")

if __name__ == "__main__":
    dash = CerberusDashboard()
    dash.run()
