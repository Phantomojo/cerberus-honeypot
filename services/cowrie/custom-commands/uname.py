"""
Cerberus Dynamic Uname Command for Cowrie
"""

from __future__ import annotations
from cowrie.shell.command import HoneyPotCommand
from .cerberus_loader import load_cerberus_output, cerberus_available

class command_uname(HoneyPotCommand):
    def help(self):
        self.write("Usage: uname [OPTION]...\n")
        self.write("Print certain system information.  With no OPTION, same as -s.\n\n")
        self.write("  -a, --all                print all information\n")
        self.write("  -s, --kernel-name        print the kernel name\n")
        self.write("  -n, --nodename           print the network node hostname\n")
        self.write("  -r, --kernel-release     print the kernel release\n")
        self.write("  -v, --kernel-version     print the kernel version\n")
        self.write("  -m, --machine            print the machine hardware name\n")
        self.write("  -p, --processor          print the processor type (non-portable)\n")
        self.write("  -i, --hardware-platform  print the hardware platform (non-portable)\n")
        self.write("  -o, --operating-system   print the operating system\n")
        self.write("--help     display this help and exit\n")

    def call(self):
        if not cerberus_available():
            # Fallback to default Cowrie behavior if Cerberus not present
            self.write("Linux\n")
            return

        client_ip = self.protocol.transport.getPeer().host
        # Load dynamic output from Cerberus
        if any(arg in ['-a', '--all'] for arg in self.args):
            output = load_cerberus_output("uname_a", ip=client_ip)
            if output:
                self.write(output + "\n")
                return

        if any(arg in ['-r', '--kernel-release'] for arg in self.args):
            output = load_cerberus_output("uname_r", ip=client_ip)
            if output:
                self.write(output + "\n")
                return

        if any(arg in ['-m', '--machine'] for arg in self.args):
            output = load_cerberus_output("uname_m", ip=client_ip)
            if output:
                self.write(output + "\n")
                return

        # Default to kernel name
        self.write("Linux\n")

commands = {"uname": command_uname}
