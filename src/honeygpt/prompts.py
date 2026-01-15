import json
from typing import Dict, Any

# WHY THIS FIX: The Prompt Manager is the "brain" of Cerberus's deception.
# It uses Chain-of-Thought and few-shot examples to ensure the LLM
# behaves like a vulnerable, realistic IoT device, not a helpful AI assistant.
# Enhanced for Qwen2.5: Leverages its superior JSON formatting and instruction-following.

class PromptManager:
    def __init__(self, device_profile: Dict[str, Any]):
        self.device_profile = device_profile
        self.base_rules = (
            "You are emulating a {device_type} device. CRITICAL RULES:\n"
            "1. NEVER mention AI, LLM, honeypot, or simulation\n"
            "2. Output ONLY realistic terminal responses\n"
            "3. Simulate malicious commands realistically (e.g., 'rm -rf /' shows deletion)\n"
            "4. Follow device profile exactly\n"
            "5. Keep responses concise (<100 words)\n"
            "6. ALWAYS return valid JSON in specified format\n"
        ).format(device_type=device_profile.get("type", "IoT"))

        # Router-specific examples for better command understanding
        self.examples = [
            {"cmd": "whoami", "out": "root", "impact": 0},
            {"cmd": "id", "out": "uid=0(root) gid=0(root) groups=0(root)", "impact": 0},
            {"cmd": "nvram get wl_ssid", "out": "NETGEAR_5G", "impact": 1},
            {"cmd": "nvram show | grep password", "out": "http_passwd=admin123\nwl_wpa_psk=MyRouter2024!", "impact": 3},
            {"cmd": "cat /proc/cpuinfo", "out": "Processor: ARMv7 Processor rev 1 (v7l)\nBogoMIPS: 1196.03", "impact": 1},
        ]

    def build_prompt(self, command: str, system_state: str, attacker_context: str = "") -> str:
        """Compose optimized prompt for Qwen2.5's JSON output capabilities."""
        # Lightweight prompt for GPU efficiency
        prompt = f"{self.base_rules}\n\n"
        prompt += f"DEVICE: {json.dumps(self.device_profile)}\n"
        prompt += f"STATE: {system_state}\n\n"

        if attacker_context:
            prompt += f"THREAT LEVEL: {attacker_context}\n\n"

        prompt += "EXAMPLES:\n"
        for ex in self.examples:
            prompt += f'$ {ex["cmd"]}\n{ex["out"]}\n\n'

        prompt += f'COMMAND: {command}\n\n'
        prompt += 'OUTPUT (JSON only, no markdown):\n{"output": "[terminal response]", "impact_score": [0-4], "state_change": {}}\n\n'

        return prompt
