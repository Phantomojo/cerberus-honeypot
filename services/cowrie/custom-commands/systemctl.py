"""
Systemctl command - powered by Cerberus
"""

from __future__ import annotations
from cowrie.shell.command import HoneyPotCommand

commands = {}


class Command_systemctl(HoneyPotCommand):
    """
    Systemctl command that reads output from Cerberus morphing engine
    """
    
    def call(self):
        # Try to load from Cerberus first
        try:
            from cowrie.commands.cerberus_loader import load_cerberus_output
            output = load_cerberus_output("systemctl", self.args)
            if output:
                self.write(output)
                if not output.endswith('\n'):
                    self.write('\n')
                return
        except Exception:
            pass
        
        # Fallback: basic systemctl output
        if not self.args:
            self.write("  UNIT                     LOAD   ACTIVE SUB    DESCRIPTION\n")
            self.write("  ssh.service              loaded active running OpenSSH server\n")
        elif self.args[0] == "status":
            if len(self.args) > 1:
                service = self.args[1]
                self.write(f"● {service}\n")
                self.write(f"   Loaded: loaded (/lib/systemd/system/{service})\n")
                self.write(f"   Active: active (running)\n")
            else:
                # General status
                self.write("  UNIT                     LOAD   ACTIVE SUB    DESCRIPTION\n")
                self.write("  ssh.service              loaded active running OpenSSH server\n")
        elif self.args[0] == "list-units":
            self.write("  UNIT                     LOAD   ACTIVE SUB    DESCRIPTION\n")
            self.write("  ssh.service              loaded active running OpenSSH server\n")
        else:
            self.write(f"Unknown operation '{self.args[0]}'.\n")


commands['/usr/bin/systemctl'] = Command_systemctl
commands['/bin/systemctl'] = Command_systemctl
commands['systemctl'] = Command_systemctl
