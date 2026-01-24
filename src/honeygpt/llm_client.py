import os
import logging
import requests
import time
from typing import Optional, List, Dict
from dotenv import load_dotenv

load_dotenv()

# WHY THIS FIX: Abstracting the LLM client allows us to swap between
# OpenAI and local models (Ollama) without changing the core logic.
# Local LLMs (Ollama) are prioritized to avoid token costs.
# GPU acceleration with Qwen2.5 provides <5s response times.

class LLMClient:
    def __init__(self, model: str = "qwen2.5:3b-instruct-q4_K_M"):
        self.ollama_url = os.getenv("OLLAMA_URL", "http://localhost:11434/api/generate")
        self.model = model
        self.cache = {}
        self.response_times = []  # Track performance for benchmarking
        self.attacker_memory = {}  # IP -> List of last 5 commands

        logging.info(f"Initialized LLMClient on {self.ollama_url}")

        # DYNAMIC MODEL SELECTION (Polish)
        self.available_models = self._get_available_models()
        if self.model not in self.available_models and self.available_models:
             # Try to find a variant
             qwen_models = [m for m in self.available_models if "qwen" in m.lower()]
             if qwen_models:
                 self.model = qwen_models[0]
                 logging.info(f"CUE: Selected available variant model: {self.model}")

        # Warm-up: Send a test query to load model into VRAM
        # try:
        #     logging.info("Warming up GPU model (loading into VRAM)...")
        #     self._warmup()
        #     logging.info("Model warm-up complete")
        # except Exception as e:
        #     logging.warning(f"Model warm-up failed: {e}. Will load on first query.")

    def _warmup(self):
        """Pre-load model into GPU VRAM for faster first response."""
        warmup_payload = {
            "model": self.model,
            "prompt": "echo test",
            "stream": False,
            "options": {
                "num_predict": 10,
                "temperature": 0.1
            }
        }
        requests.post(self.ollama_url, json=warmup_payload, timeout=180)

    def query(self, prompt: str, history: List[Dict[str, str]] = None, attacker_ip: str = "unknown", raw_command: str = "") -> Optional[str]:
        """Query the Local LLM (Ollama) with GPU acceleration and adversarial memory."""
        # Update Attacker Memory
        if attacker_ip not in self.attacker_memory:
            self.attacker_memory[attacker_ip] = []

        # We don't need the whole prompt in memory, just a summary or the command extracted
        # For now, let's just log the interaction
        # PERFORMANCE OPTIMIZATION: Keep memory lean for high-throughput
        if len(self.attacker_memory[attacker_ip]) >= 10:
            self.attacker_memory[attacker_ip].pop(0)

        # WE STORE AS DICT FOR CONSISTENCY
        import time
        self.attacker_memory[attacker_ip].append({
            "command": prompt[:50],
            "timestamp": time.time()
        })

        if prompt in self.cache:
            logging.debug(f"Cache hit for prompt: {prompt[:50]}...")
            return self.cache[prompt]

        start_time = time.time()

        try:
            # Ollama API structure with GPU-optimized parameters
            payload = {
                "model": self.model,
                "prompt": prompt,
                "stream": False,
                "options": {
                    "temperature": 0.7,
                    "num_predict": 64,        # FASTER: Shorter responses for terminal
                    "num_ctx": 512,           # VRAM OPTIMAL: Balanced for IoT context
                    "num_gpu": -1,            # ALL LAYERS ON GPU
                    "top_p": 0.9,
                    "top_k": 40
                }
            }

            response = requests.post(self.ollama_url, json=payload, timeout=300)
            response.raise_for_status()

            result = response.json().get("response", "")

            # Log response time for benchmarking
            response_time = time.time() - start_time
            self.response_times.append(response_time)
            logging.info(f"LLM response time: {response_time:.2f}s (GPU-accelerated)")

            # Cache successful responses
            self.cache[prompt] = result
            return result

        except Exception as e:
            logging.error(f"Ollama Query failed: {e}. Falling back to MOCK.")
            return self._mock_fallback(prompt, raw_command)

    def _get_available_models(self) -> List[str]:
        """Fetch list of actually loaded models in Ollama."""
        try:
            tags_url = self.ollama_url.replace("/generate", "/tags")
            r = requests.get(tags_url, timeout=2)
            if r.status_code == 200:
                models = r.json().get("models", [])
                return [m["name"] for m in models]
        except: pass
        return []

    def _mock_fallback(self, prompt: str, command: str = "") -> str:
        """Fallback for testing with specialized deceptive logic."""
        # Check command first for specific matches
        cmd_lower = command.lower()

        # 1. Broaden Capabilities: Adversarial Context Detection
        if "integrity drift" in prompt.lower():
             logging.info("Fallback: Integrity Drift context detected")

        if cmd_lower == "who":
             return self._gen_who()
        if cmd_lower == "whoami":
             return "root"
        if cmd_lower == "last":
             return self._gen_last()
        if cmd_lower in ["ps", "top"]:
             return self._gen_ps()
        if cmd_lower == "uptime":
             return self._gen_uptime()
        if cmd_lower == "uname":
             arch = "x86_64"
             if "mips" in prompt.lower(): arch = "mips"
             return self._gen_uname(arch)
        if cmd_lower == "df":
             return self._gen_df()
        if cmd_lower.startswith("nvram"):
             return "NETGEAR_5G"

        # 2. Device-Specific personalities (Exact matches)
        if cmd_lower == "dicom":
            return "GE_HEALTHCARE_IMAGE_NODE_v4.1. Access Denied."
        if cmd_lower == "ocpp":
             return "OCPP 1.6J JSON-over-WebSocket active. Status: Operating."

        # Base Realistic Mocks
        if cmd_lower == "ls":
            return "bin  etc  home  lib  proc  root  sbin  sys  tmp  usr  var"

        return "Command executed successfully (Cerberus Adaptive Mock)"

    def get_avg_response_time(self) -> float:
        """Get average response time for benchmarking."""
        if not self.response_times:
            return 0.0
        return sum(self.response_times) / len(self.response_times)
