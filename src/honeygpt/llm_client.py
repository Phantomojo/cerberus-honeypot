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
        self.ollama_url = os.getenv("OLLAMA_URL", "http://ollama:11434/api/generate")
        self.model = model
        self.cache = {}
        self.response_times = []  # Track performance for benchmarking

        logging.info(f"Initialized LLMClient with GPU-accelerated model: {self.model}")
        logging.info(f"Ollama URL: {self.ollama_url}")

        # Warm-up: Send a test query to load model into VRAM
        try:
            logging.info("Warming up GPU model (loading into VRAM)...")
            self._warmup()
            logging.info("Model warm-up complete")
        except Exception as e:
            logging.warning(f"Model warm-up failed: {e}. Will load on first query.")

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

    def query(self, prompt: str, history: List[Dict[str, str]] = None) -> Optional[str]:
        """Query the Local LLM (Ollama) with GPU acceleration."""
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
                    "num_predict": 128,       # Shorter responses for honeypot commands
                    "num_ctx": 1024,          # Reduced context window to fit more layers in VRAM
                    "num_gpu": -1,            # Use all available GPU layers
                    "top_p": 0.9,
                    "top_k": 40
                }
            }

            response = requests.post(self.ollama_url, json=payload, timeout=120)
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
            response_time = time.time() - start_time
            logging.error(f"Ollama Query failed after {response_time:.2f}s: {e}. Falling back to MOCK.")
            return self._mock_response(prompt)

    def _mock_response(self, prompt: str) -> str:
        """Fallback for testing without a running LLM service."""
        if "ls" in prompt:
            return "bin  etc  home  lib  proc  root  sbin  sys  tmp  usr  var"
        if "uname" in prompt:
            return "Linux cerberus-iot 5.15.0-generic #1 SMP x86_64 GNU/Linux"
        if "nvram" in prompt:
            return "NETGEAR_5G"
        return "Command executed successfully (Mock Response)"

    def get_avg_response_time(self) -> float:
        """Get average response time for benchmarking."""
        if not self.response_times:
            return 0.0
        return sum(self.response_times) / len(self.response_times)
