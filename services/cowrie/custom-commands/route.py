"""
Route command - powered by Cerberus
"""

from __future__ import annotations
from cowrie.shell.command import HoneyPotCommand

commands = {}


class Command_route(HoneyPotCommand):
    """
    Route command that reads output from Cerberus morphing engine
    """
    
    def call(self):
        # Try to load from Cerberus first
        try:
            from cowrie.commands.cerberus_loader import load_cerberus_output
            output = load_cerberus_output("route", self.args)
            if output:
                self.write(output)
                if not output.endswith('\n'):
                    self.write('\n')
                return
        except Exception:
            pass
        
        # Fallback: basic route output
        if not self.args or self.args[0] == "-n":
            self.write("Kernel IP routing table\n")
            self.write("Destination     Gateway         Genmask         Flags Metric Ref    Use Iface\n")
            self.write("0.0.0.0         192.168.1.1     0.0.0.0         UG    0      0        0 eth0\n")
            self.write("192.168.1.0     0.0.0.0         255.255.255.0   U     0      0        0 eth0\n")
        else:
            self.write(f"route: invalid option -- '{self.args[0]}'\n")


commands['/usr/bin/route'] = Command_route
commands['/bin/route'] = Command_route
commands['/sbin/route'] = Command_route
commands['route'] = Command_route
