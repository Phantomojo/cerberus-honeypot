"""
Cerberus CVE Information Command
Displays simulated vulnerabilities for the current profile.
"""

from __future__ import annotations
from cowrie.shell.command import HoneyPotCommand
from .cerberus_loader import load_behavior_config

class command_vulns(HoneyPotCommand):
    def call(self):
        behavior = load_behavior_config()
        cves = behavior.get("cves", "")

        if not cves:
            self.write("No known vulnerabilities found for this kernel version.\n")
            return

        self.write(f"Vulnerability Scan Report for Kernel {self.protocol.kernel_version}\n")
        self.write("-" * 40 + "\n")
        for cve in cves.split(','):
            self.write(f"[LOW/HIGH] {cve.strip()} - Potential exploit available\n")
        self.write("-" * 40 + "\n")
        self.write("Total vulnerabilities detected: " + str(len(cves.split(','))) + "\n")

from cowrie.shell.command import HoneyPotCommand
commands = {}
commands['check_vulns'] = command_vulns
commands['/usr/bin/check_vulns'] = command_vulns
