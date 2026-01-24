"""
Cerberus Architecture Guard for Cowrie
Intercepts script and binary execution to ensure architecture consistency.
"""

from __future__ import annotations
from cowrie.shell.command import HoneyPotCommand
from .cerberus_loader import get_current_arch
import random

class command_sh(HoneyPotCommand):
    def call(self):
        if not self.args:
            return

        script = self.args[0]
        arch = get_current_arch()

        # Check if the file looks like a binary we shouldn't be running
        # (Very simplified check for the 'proper thing')
        if script.endswith(".arm") and arch == "mips":
            self.write(f"sh: {script}: line 1: syntax error: unexpected \")\"\n")
            return

        if script.endswith(".mips") and arch == "arm":
            self.write(f"sh: {script}: line 1: syntax error: unexpected \")\"\n")
            return

        self.write(f"Running script {script}...\n")
        # In a real honeypot, we'd log this and then simulate a successful but hollow run
        self.write("Done.\n")

from cowrie.shell.command import HoneyPotCommand
commands = {}
commands['/bin/sh'] = command_sh
commands['sh'] = command_sh
