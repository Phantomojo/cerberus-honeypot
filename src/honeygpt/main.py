import os
import json
import logging
import signal
import sys
import socket
from prompts import PromptManager
from llm_client import LLMClient
from state_sync import StateManager

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s [%(levelname)s] HoneyGPT: %(message)s',
    handlers=[
        logging.FileHandler("build/honeygpt.log"),
        logging.StreamHandler()
    ]
)

# WHY THIS FIX: Implementing a real socket listener allows Cowrie to
# send commands to HoneyGPT in real-time. This is the "bridge" that
# enables the hybrid AI terminal experience.

class HoneyGPTProxy:
    def __init__(self, host='0.0.0.0', port=50051):
        self.profile = self._load_profile()
        self.prompts = PromptManager(self.profile)
        self.llm = LLMClient()
        self.state = StateManager()
        self.running = True
        self.host = host
        self.port = port
        self.server_socket = None

    def _load_profile(self) -> dict:
        """Load current device profile from shared state file."""
        # In a real deployment, we'd read profiles.conf and build/current_profile.state
        return {
            "name": "Netgear R7000",
            "type": "router",
            "os": "OpenWrt 19.07",
            "kernel": "4.14.171"
        }

    def handle_command(self, command: str) -> str:
        """Process an incoming attacker command via LLM."""
        logging.info(f"Processing command: {command}")

        # Build prompt with current state
        system_state = self.state.get_context()
        prompt = self.prompts.build_prompt(command, system_state)

        # Query LLM (Ollama)
        response_json = self.llm.query(prompt)

        try:
            # The LLM is instructed to return JSON
            data = json.loads(response_json)
            output = data.get("output", "Command executed.")
            impact = data.get("impact_score", 0)
            state_changes = data.get("state_change", {})

            if impact > 2:
                logging.warning(f"HIGH IMPACT command detected (Score {impact}): {command}")

            if state_changes:
                self.state.update_state(state_changes)

            return output
        except Exception:
            # Fallback if LLM doesn't return valid JSON
            return response_json

    def run(self):
        """Socket listener for command interception."""
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.bind((self.host, self.port))
        self.server_socket.listen(5)

        logging.info(f"HoneyGPT AI Bridge listening on {self.host}:{self.port}")

        while self.running:
            try:
                client_socket, addr = self.server_socket.accept()
                data = client_socket.recv(4096).decode('utf-8')
                if data:
                    response = self.handle_command(data.strip())
                    client_socket.send(response.encode('utf-8'))
                client_socket.close()
            except Exception as e:
                logging.error(f"Error in listener loop: {e}")
                if not self.running:
                    break

    def stop(self):
        self.running = False
        if self.server_socket:
            self.server_socket.close()
        logging.info("HoneyGPT Proxy Stopping...")

if __name__ == "__main__":
    proxy = HoneyGPTProxy()

    def signal_handler(sig, frame):
        proxy.stop()
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)
    proxy.run()
