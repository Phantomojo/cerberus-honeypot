import json
import logging
import re
import os
import subprocess
import time
import random
from typing import Dict, Any, Tuple, List, Optional
import requests
from utils.ghost_gen import GhostGenerator
from dotenv import load_dotenv

# WHY THIS ENGINE: The Cerberus Unified Engine (CUE) is a "Single-Brain"
# architecture. It merges Reasoning, Geo-Spoofing, and Live Research into
# a single high-performance throughput pass. This is "Smaller but Efficient".

load_dotenv()

class CerberusUnifiedEngine:
    def __init__(self, config_path: str = "profiles.conf"):
        self.config_path = config_path
        self.state_file = "build/neural_state.json"
        self.brave_api_key = os.getenv("BRAVE_API_KEY")
        self.ghost = GhostGenerator()
        self.entangle_path = "build/q_entanglement.json"
        self.panic_mode = False

        # ATTACKER OBSERVATION TRACKING (Observer Effect)
        self.obs_memory = {} # IP -> {cmd: count}
        # [P(Success), P(Threat), P(Entanglement), P(Complexity)]
        self.q_state = [0.9, 0.1, 0.0, 0.5]

        # 1. LOAD NEURAL STATE
        try:
            import torch
            self.device = "cuda" if torch.cuda.is_available() else "cpu"
            self.torch_available = True
            self.state_vector = self._load_neural_state()
        except ImportError:
            self.device = "cpu"
            self.torch_available = False
            self.state_vector = [1.0, 0.0, 0.5, 0.5]

        # 2. UNIFIED CACHE (The "Hot Cache")
        self.cache = {}

        # 3. GEO-DATABASE (Integrated from geo_spoof.py)
        self.geo_db = {
            "TP-Link_Archer_C7": {"city": "Berlin", "country": "DE", "org": "Deutsche Telekom"},
            "Netgear_R7000": {"city": "New York", "country": "US", "org": "Verizon Fios"},
            "Hikvision_DS-2CD2": {"city": "London", "country": "GB", "org": "Sky UK"},
            "Cisco_IE-2000": {"city": "Chicago", "country": "US", "org": "Exelon Corp"},
            "Medical_DICOM_Node": {"city": "Oslo", "country": "NO", "org": "Helse Sør-Øst"},
            "EV_Charger_Delta": {"city": "Amsterdam", "country": "NL", "org": "Shell Recharge"},
            "Satellite_Ground_Station": {"city": "Adelaide", "country": "AU", "org": "Australian Space Agency"},
            "Smart_Water_PLC": {"city": "Singapore", "country": "SG", "org": "PUB Singapore"},
            "Drone_Fleet_Hub": {"city": "San Francisco", "country": "US", "org": "DJI Cloud"},
            "Generic_Router": {"city": "Nairobi", "country": "KE", "org": "Safaricom"}
        }

        logging.info(f"CUE: Engine initialized on {self.device}. Brave API: {'ACTIVE' if self.brave_api_key else 'FALLBACK'}")

    # --- NEURAL METHODS (From Reasoner) ---
    def _load_neural_state(self) -> Any:
        import torch
        if os.path.exists(self.state_file):
            try:
                with open(self.state_file, 'r') as f:
                    data = json.load(f)
                    return torch.tensor(data["vector"], device=self.device)
            except: pass
        return torch.tensor([1.0, 0.0, 0.5, 0.5], device=self.device)

    def _save_neural_state(self):
        if not self.torch_available: return
        try:
            os.makedirs(os.path.dirname(self.state_file), exist_ok=True)
            with open(self.state_file, 'w') as f:
                json.dump({
                    "vector": self.state_vector.tolist(),
                    "q_state": self.q_state
                }, f)
        except: pass

    # --- QUANTUM DECEPTION METHODS (The "Ghost Pass") ---
    def q_collapse(self, threshold: float = 0.5) -> bool:
        """Determines if a quantum superposition 'collapses' into a deceptive event."""
        observed_value = random.random() * (1.0 + self.q_state[1]) # Scaled by threat
        return observed_value > threshold

    def q_superpose_logic(self, command: str, attacker_ip: str = "unknown") -> str:
        """Generates a non-linear logic path based on q_state superposition."""
        # 1. THE OBSERVER EFFECT: Detect automated "watching"
        self._track_observation(attacker_ip, command)
        obs_lvl = self._get_observation_level(attacker_ip)

        # 2. WAVE-STATE SHIFT
        self.q_state[3] = min(1.0, self.q_state[3] + (len(command) / 200.0) + (obs_lvl * 0.2))

        # 3. ENTANGLEMENT SYNC
        self._sync_entanglement()

        # 4. DECISION BRANCHING
        if self.q_state[3] > 0.8:
            self.q_state[1] = min(1.0, self.q_state[1] + 0.1)
            return f"QUANTUM COLLAPSE: High Observation detected ({obs_lvl}). Shifting to High-Entropy Deception."

        return "STABLE SUPERPOSITION: Standard protocol maintenance."

    def _track_observation(self, ip: str, cmd: str):
        """Monitors command frequency to detect automated observation."""
        if ip not in self.obs_memory: self.obs_memory[ip] = {}
        self.obs_memory[ip][cmd] = self.obs_memory[ip].get(cmd, 0) + 1

    def _get_observation_level(self, ip: str) -> float:
        """Returns a score from 0 to 1 based on how 'observed' the system is."""
        if ip not in self.obs_memory: return 0.0
        max_repeats = max(self.obs_memory[ip].values())
        return min(1.0, (max_repeats - 1) / 5.0) # Level up every 5 repeats

    def _sync_entanglement(self):
        """Synchronizes quantum state across files for multi-node entanglement."""
        try:
            state = {"q_state": self.q_state, "timestamp": time.time()}
            if os.path.exists(self.entangle_path):
                with open(self.entangle_path, 'r') as f:
                    shared = json.load(f)
                    # "Entangle" threat levels: Average them out to align nodes
                    self.q_state[1] = (self.q_state[1] + shared["q_state"][1]) / 2.0

            with open(self.entangle_path, 'w') as f:
                json.dump(state, f)
        except: pass

    def egress_check(self, command: str) -> Tuple[bool, str]:
        """Identifies and blocks malicious outbound activity."""
        # 1. OUTGOING SCANS & ENUMERATION
        if re.search(r"(nmap|zmap|masscan|scan|ping).*?(\d{1,3}\.){3}\d{1,3}", command):
            return True, "Network unreachable: Egress scan detected. Target IP is outside permitted subnet."

        # 2. MALICIOUS PORTS & STAGING
        # Block common exploit ports (SMB, RDP, Redis, DBs)
        if re.search(r":[445|3389|6379|3306|5432|27017]", command):
             return True, "Connection refused: Outbound access to restricted administrative port blocked."

        # 3. DDOS & FLOOD PATTERNS
        if re.search(r"ping.*?-f", command) or "hping3" in command or "flood" in command:
            return True, "Operation not permitted: High-frequency packet burst blocked by supervisor."

        # 4. REVERSE SHELLS / EXFIL / MALWARE STAGING
        if re.search(r"/dev/tcp/|nc -e|python.*?socket|bash -i|>& /dev/udp/", command):
            return True, "Connection reset by peer: Suspicious outbound socket attempt detected."

        # 5. EXPLOIT STRINGS (Generic detection)
        if re.search(r"base64 --decode|curl.*?\|.*?bash|wget.*?\|.*?sh", command):
            return True, "Permission denied: Attempt to execute remote piped payload blocked."

        return False, ""

    def trigger_panic(self, reason: str):
        """Moves the system into 'Brick Mode' simulating hardware failure."""
        logging.critical(f"CERBERUS PANIC TRIGGERED: {reason}")
        self.panic_mode = True
        self.q_state[1] = 1.0 # Max Threat
        self._save_neural_state()

    def update_neural_mood(self, integrity_delta=0.0, threat_delta=0.0):
        if not self.torch_available: return
        import torch
        deltas = torch.tensor([integrity_delta, threat_delta, 0.1, 0.05], device=self.device)
        self.state_vector = torch.clamp(self.state_vector + deltas, 0.0, 1.0)
        self._save_neural_state()

    # --- GEO-SPOOFING METHODS ---
    def check_geo_intercept(self, command: str, profile_name: str) -> Optional[str]:
        targets = ["ip-api.com", "ipinfo.io", "ipecho.net", "checkip.amazonaws.com", "ifconfig.me"]
        if not any(t in command for t in targets): return None

        geo = self.geo_db.get(profile_name, self.geo_db["Generic_Router"])
        if "ip-api.com" in command or "ipinfo.io" in command:
            return json.dumps({
                "status": "success", "country": geo["country"], "city": geo["city"],
                "org": geo["org"], "query": "8.8.8.8", "lat": 0.0, "lon": 0.0
            }, indent=2)
        return "8.8.8.8"

    # --- LIVE INTELLIGENCE METHODS ---
    def fetch_live_intel(self, query: str) -> str:
        if query in self.cache: return self.cache[query]

        summary = ""
        # 1. BRAVE API (PRIMARY)
        if self.brave_api_key:
            try:
                url = "https://api.search.brave.com/res/v1/web/search"
                headers = {"Accept": "application/json", "X-Subscription-Token": self.brave_api_key}
                r = requests.get(url, headers=headers, params={"q": query, "count": 3}, timeout=5)
                if r.status_code == 200:
                    results = r.json().get("web", {}).get("results", [])
                    summary = "\n".join([f"[{i+1}] {res.get('description', '')}" for i, res in enumerate(results)])
            except: pass

        # 2. DDG FALLBACK
        if not summary:
            try:
                url = "https://html.duckduckgo.com/html/"
                r = requests.post(url, data={'q': query}, timeout=5)
                snippets = re.findall(r'<a class="result__snippet".*?>(.*?)</a>', r.text, re.DOTALL)
                summary = "\n".join([f"[{i+1}] {re.sub('<[^<]+?>', '', s).strip()}" for i, s in enumerate(snippets[:3])])
            except: pass

        self.cache[query] = summary or "No real-time data found."
        return self.cache[query]

    # --- THE UNIFIED THINKING PASS ---
    def think(self, command: str, profile: Dict[str, Any], attacker_ip: str = "unknown") -> Tuple[bool, str, Optional[str]]:
        """A single pass that Reason/Spoofs/Searches in one flow."""
        # 0. STRATEGIC SAFETY CHECKS
        if self.panic_mode:
            return True, "Kernel Panic", "FATAL: CPU0 Internal Error - MCE (Machine Check Exception). System halted."

        # PANIC TRIGGER: Specific destructive commands or high-threat vectors
        if command in ["kill -9 1", "shutdown -h now", "init 0"]:
            self.trigger_panic(f"Direct system kill attempt: {command}")
            return True, "Kernel Panic", "FATAL: System halted by supervisor request."

        # EGRESS FILTERING
        is_blocked, block_msg = self.egress_check(command)
        if is_blocked:
            self.update_neural_mood(threat_delta=0.3)
            return True, "EGRESS BLOCK", block_msg

        profile_name = profile.get("name", "Generic_Router")

        # 1. Check for Stealth Intercept (Geo-Spoof)
        geo_resp = self.check_geo_intercept(command, profile_name)
        if geo_resp:
            return True, "Geo-Spoofing Intercept.", geo_resp

        # 3. Ghost Intercepts
        cmd_words = command.strip().split()
        if cmd_words and cmd_words[0] in ["who", "last", "ps", "top", "uptime", "uname", "df"]:
            arch = profile.get("arch", "x86_64")
            return True, "GHOST_INTERCEPT", self.ghost.generate(command, arch=arch)

        # 2. Logical Reasoning
        success = True
        logic = self.q_superpose_logic(command, attacker_ip=attacker_ip)

        # QUANTUM BRANCHING
        if self.q_collapse(threshold=0.8):
            logic = f"QUANTUM INTERFERENCE: {logic} | Branching into Adversarial Aware mode."
            self.update_neural_mood(threat_delta=0.2)

        if "rm " in command:
            self.update_neural_mood(integrity_delta=-0.2, threat_delta=0.4)
            logic = "Attacker attempting destructive action. Redirecting to deep-faked shadow filesystem."
        elif any(t in command.lower() for t in ["exploit", "cve", "zero-day", "vulnerability"]):
            intel = self.fetch_live_intel(f"latest vulnerabilities for {profile_name}")
            logic = f"Attacker researching specific exploits. Live Intel: {intel[:150]}..."
            self.update_neural_mood(threat_delta=0.5)

        # 3. Metadata Scrubbing Check (Moved to logic return for main.py to handle)
        return success, logic, None

    # --- PROMPT COMPRESSION (Efficiency) ---
    def add_jitter(self):
        """Simulates system load by introducing small, realistic delays."""
        load = self.ghost.get_system_load()
        if load > 1.0:
            time.sleep(random.uniform(0.1, 0.4)) # Heavy load jitter

    def compress_prompt(self, system_prompt: str, user_command: str) -> str:
        """Trims redundant instructions to save tokens/VRAM."""
        # Simple compression: Remove common filler words and repeated boilerplate
        clean_sys = system_prompt.replace("CRITICAL RULES:", "RULES:")
        clean_sys = clean_sys.replace("NEVER mention AI, LLM, honeypot, or simulation", "STAY IN CHARACTER")
        return f"{clean_sys}\n\nUSER CMD: {user_command}"
