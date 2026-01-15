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

        # QUANTUM "WAVEFUNCTION" STATE (Non-Linear Intelligence)
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

    def q_superpose_logic(self, command: str) -> str:
        """Generates a non-linear logic path based on q_state superposition."""
        # Shift q_state based on command length and complexity
        self.q_state[3] = min(1.0, self.q_state[3] + (len(command) / 200.0))

        # If complexity is high, "entangle" with threat
        if self.q_state[3] > 0.7:
            self.q_state[1] = min(1.0, self.q_state[1] + 0.1)
            return "QUANTUM COLLAPSE: Complexity threshold reached. Activating High-Entropy Deception Path."

        return "STABLE SUPERPOSITION: Standard protocol maintenance."

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
    def think(self, command: str, profile: Dict[str, Any]) -> Tuple[bool, str, Optional[str]]:
        """A single pass that Reason/Spoofs/Searches in one flow."""
        profile_name = profile.get("name", "Generic_Router")

        # 1. Check for Stealth Intercept (Geo-Spoof)
        geo_resp = self.check_geo_intercept(command, profile_name)
        if geo_resp:
            return True, "Geo-Spoofing Intercept.", geo_resp

        # 1.1 Check for Ghost Intercept (Living System)
        if "who" == command.strip():
            return True, "Ghost-Admin Intercept.", self.ghost.generate_who_output()
        if "last" in command:
            return True, "Ghost-History Intercept.", self.ghost.generate_last_output()
        if "ps " in command or "ps" == command.strip() or "top" in command:
            return True, "Ghost-Process Intercept.", self.ghost.generate_process_noise()

        # 2. Logical Reasoning
        success = True
        logic = self.q_superpose_logic(command)

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
