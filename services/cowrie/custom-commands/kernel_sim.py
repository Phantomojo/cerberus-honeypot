"""
Cerberus Kernel Vulnerability Simulator
Intercepts LPE exploit attempts and provides realistic feedback.
"""

from __future__ import annotations
from cowrie.shell.command import HoneyPotCommand
import time
import random

class command_exploit(HoneyPotCommand):
    """
    Simulates a generic exploit run.
    """
    def call(self):
        cmd = self.input_data.strip() if hasattr(self, 'input_data') else ""
        from .cerberus_loader import load_behavior_config
        behavior = load_behavior_config()
        is_vulnerable = behavior.get("is_vulnerable", "0") == "1"

        # Simulate exploit progress
        self.write("[*] Finding kernel symbols...\n")
        time.sleep(1.0)

        if not is_vulnerable:
            self.write("[-] Error: Kernel symbols not found for this architecture.\n")
            self.write("[-] Exploit failed: Target not vulnerable.\n")
            return

        self.write("[*] Preparing exploit payload...\n")
        time.sleep(1.5)
        self.write("[*] Triggering race condition...\n")
        time.sleep(2.0)

        if random.random() < 0.3:
            # 30% chance of Kernel Oops (Crash)
            self.write("\n[   42.123456] BUG: unable to handle kernel paging request at ffff880000000000\n")
            self.write("[   42.123470] IP: [<ffffffff81001234>] exploit_payload+0x45/0x90\n")
            self.write("[   42.123485] PGD 1234067 PUD 1235067 PMD 0\n")
            self.write("[   42.123500] Oops: 0000 [#1] SMP\n")
            self.write("[   42.123515] Modules linked in: mtd_storage bridge stp llc\n")
            self.write("[   42.123530] ---[ end trace 1234567890abcdef ]---\n")
            self.protocol.terminal.loseConnection()
        else:
            # 70% success - give fake root
            self.write("[+] Exploit successful! Enjoy your root shell.\n")
            self.protocol.user = "root"
            # Update prompt for root
            self.protocol.prompt = f"{self.protocol.hostname}:/root# "

class command_id(HoneyPotCommand):
    """Override id to show root if exploit succeeded."""
    def call(self):
        if self.protocol.user == "root":
            self.write("uid=0(root) gid=0(root) groups=0(root)\n")
        else:
            self.write("uid=1000(user) gid=1000(user) groups=1000(user)\n")

from cowrie.shell.command import HoneyPotCommand
commands = {}
# Hook into common exploit filenames
commands['dirtycow'] = command_exploit
commands['./dirtycow'] = command_exploit
commands['exploit'] = command_exploit
commands['id'] = command_id
