"""
Generic AI Command - powered by HoneyGPT
This handler catches commands and forwards them to the AI bridge.
"""

from __future__ import annotations
import socket
from cowrie.shell.command import HoneyPotCommand

# HoneyGPT service address (Connect to Host)
HONEYGPT_HOST = "172.17.0.1"
HONEYGPT_PORT = 50051

class Command_generic_ai(HoneyPotCommand):
    """
    Generic command that forwards input to the HoneyGPT AI service.
    """

    def call(self):
        full_command = " ".join([self.args[0]] + self.args[1:]) if self.args else self.environ.get('COMMAND_LINE', '')
        if not full_command:
            # Try to reconstruct from call info if possible
            full_command = self.environ.get('last_command', 'unknown')

        # Connect to HoneyGPT Bridge
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.settimeout(30)
                s.connect((HONEYGPT_HOST, HONEYGPT_PORT))
                s.sendall(full_command.encode('utf-8'))
                raw_response = s.recv(8192).decode('utf-8')

                import json
                try:
                    data = json.loads(raw_response)
                    text = data.get("response", "")
                    self.write(text)
                    if not text.endswith('\n'):
                        self.write('\n')

                    # DYNAMIC PROMPT SYNC: Update the shell hostname if the AI morphed
                    if "hostname" in data and hasattr(self.protocol, 'hostname'):
                        self.protocol.hostname = data["hostname"]
                except:
                    # Fallback for plain text
                    self.write(raw_response)
                    if not raw_response.endswith('\n'):
                        self.write('\n')
        except Exception as e:
            # LOG THE ERROR: Don't just fail silently!
            try:
                with open("/var/log/cowrie/ai_errors.log", "a") as f:
                    f.write(f"[{datetime.now()}] AI Bridge Error ({full_command}): {str(e)}\n")
            except: pass
            self.write(f"sh: {full_command}: command not found\n")

# Entry point for Cowrie
commands = {}
aicmd = Command_generic_ai

# Map common missing commands to the AI
targets = [
    'python', 'python3', 'pip', 'wget', 'curl',
    'iptables', 'ip', 'ifconfig', 'tcpdump',
    'nmap', 'nc', 'netcat', 'gcc', 'g++', 'make',
    'sudo', 'apt', 'apt-get', 'yum', 'df', 'top',
    'free', 'uptime', 'dmesg', 'mysql', 'ps', 'hostname', 'uname',
    # Router-specific commands
    'nvram', 'show', 'enable', 'configure', 'config',
    'router', 'wireless', 'vlan', 'interface'
]

for t in targets:
    commands[t] = aicmd
    commands[f'/bin/{t}'] = aicmd
    commands[f'/usr/bin/{t}'] = aicmd
    commands[f'/sbin/{t}'] = aicmd
    commands[f'/usr/sbin/{t}'] = aicmd
