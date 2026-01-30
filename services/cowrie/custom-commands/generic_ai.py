from __future__ import annotations
from cowrie.shell.command import HoneyPotCommand
from .cerberus_ai_bridge import query_brain
import json

class Command_generic_ai(HoneyPotCommand):
    """
    Cerberus Hybrid AI Bridge handler.
    Catches commands and forwards them to the remote local brain.
    """
    def call(self):
        # 1. Reconstruct the full command line
        cmd_name = self.environ.get('COMMAND_NAME', 'unknown')
        full_command = " ".join([cmd_name] + self.args) if self.args else cmd_name
        attacker_ip = "unknown"
        if hasattr(self.protocol, 'transport'):
            attacker_ip = self.protocol.transport.getPeer().host

        # 2. Query the Remote Brain (Local PC)
        result = query_brain(full_command, attacker_ip)

        # 3. Handle response
        if result.get("status") == "success":
            response = result.get("response", "")
            if response:
                self.write(response + "\n")

            # IDENTITY SYNC: Update shell hostname if the AI morphed
            if "hostname" in result and hasattr(self.protocol, 'hostname'):
                self.protocol.hostname = result["hostname"]
        else:
            # Fallback for offline brain
            self.write(f"bash: {cmd_name}: command not found\n")

# Entry point for Cowrie - Mapping all common "Intel" commands to AI
commands = {}
aicmd = Command_generic_ai

# Massive target list for AI deception
targets = [
    'python', 'python3', 'pip', 'wget', 'curl', 'iptables', 'ip', 'ifconfig',
    'tcpdump', 'nmap', 'nc', 'netcat', 'gcc', 'g++', 'make', 'sudo', 'apt',
    'apt-get', 'yum', 'df', 'top', 'free', 'uptime', 'dmesg', 'mysql', 'ps',
    'hostname', 'uname', 'nvram', 'show', 'enable', 'configure', 'config',
    'router', 'wireless', 'vlan', 'interface', 'ls', 'cat', 'sh', 'bash'
]

for t in targets:
    commands[t] = aicmd
    commands[f'/bin/{t}'] = aicmd
    commands[f'/usr/bin/{t}'] = aicmd
    commands[f'/sbin/{t}'] = aicmd
    commands[f'/usr/sbin/{t}'] = aicmd
