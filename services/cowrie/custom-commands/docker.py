"""
Docker command - powered by Cerberus
"""

from __future__ import annotations
from cowrie.shell.command import HoneyPotCommand

commands = {}


class Command_docker(HoneyPotCommand):
    """
    Docker command that reads output from Cerberus morphing engine
    """
    
    def call(self):
        # Try to load from Cerberus first
        try:
            from cowrie.commands.cerberus_loader import load_cerberus_output
            output = load_cerberus_output("docker", self.args)
            if output:
                self.write(output)
                if not output.endswith('\n'):
                    self.write('\n')
                return
        except Exception:
            pass
        
        # Fallback: basic docker output
        if not self.args:
            self.write("Usage: docker [OPTIONS] COMMAND [ARG...]\n")
            self.write("       docker [ --help | -v | --version ]\n\n")
            self.write("A self-sufficient runtime for containers.\n")
        elif self.args[0] == "ps":
            self.write("CONTAINER ID   IMAGE     COMMAND   CREATED   STATUS    PORTS     NAMES\n")
        elif self.args[0] == "--version":
            self.write("Docker version 20.10.7, build f0df350\n")
        else:
            self.write(f"docker: '{self.args[0]}' is not a docker command.\n")
            self.write("See 'docker --help'\n")


commands['/usr/bin/docker'] = Command_docker
commands['docker'] = Command_docker
