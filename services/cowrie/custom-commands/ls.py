"""
Cerberus Dynamic Ls Command for Cowrie
Shows realistic device nodes and filesystem layouts.
"""

from __future__ import annotations
from cowrie.shell.command import HoneyPotCommand
from .cerberus_loader import get_current_arch

class command_ls(HoneyPotCommand):
    def call(self):
        path = self.args[0] if self.args else "."

        if path == "/dev":
            self.write("console  gpio  mem  mtd0  mtd0ro  mtdblock0  null  ptmx  random  tty  urandom  zero\n")
            return

        if path == "/proc":
            self.write("1  cpuinfo  devices  filesystems  interrupts  ioports  kmsg  meminfo  mounts  net  self  stat  uptime  version\n")
            return

        # Fallback to Cowrie's honeyfs
        self.write(f"ls: {path}: No such file or directory\n")

from cowrie.shell.command import HoneyPotCommand
commands = {}
commands['/bin/ls'] = command_ls
commands['ls'] = command_ls
