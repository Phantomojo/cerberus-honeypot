from __future__ import annotations
from cowrie.shell.command import HoneyPotCommand
from .cerberus_ai_bridge import query_brain
import json

class command_ai_interceptor(HoneyPotCommand):
    """
    Cerberus AI Interceptor.
    This command class can be used to wrap ANY command in Cowrie.
    It forwards the command to the local AI 'Brain' and writes the response.
    """
    def call(self):
        # 1. Get the raw command and IP
        cmd_name = self.environ.get('COMMAND_NAME', 'unknown')
        full_command = f"{cmd_name} {' '.join(self.args)}"
        attacker_ip = self.protocol.transport.getPeer().host

        # 2. Query the Hybrid Brain (via Tailscale)
        # The bridge handles the socket comms
        result = query_brain(full_command, attacker_ip)

        # 3. Output the AI response
        if result.get("status") == "success":
            response = result.get("response", "")
            if response:
                self.write(response + "\n")
        else:
            # Fallback to 'command not found' if brain is unreachable
            self.write(f"bash: {cmd_name}: command not found\n")

commands = {"ai_interceptor": command_ai_interceptor}
