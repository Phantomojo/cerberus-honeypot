"""
Cerberus Dynamic Cat Command for Cowrie
Intercepts reads of sensitive system files to prevent leakage.
"""

from __future__ import annotations
from cowrie.shell.command import HoneyPotCommand
from .cerberus_loader import load_cerberus_output, cerberus_available, get_current_arch
import random

class command_cat(HoneyPotCommand):
    def call(self):
        if not self.args:
            return

        client_ip = self.protocol.transport.getPeer().host
        filename = self.args[0]

        # Intercept /proc and /dev nodes
        if "/proc/cpuinfo" in filename:
            output = load_cerberus_output("cpuinfo", ip=client_ip)
            if output:
                self.write(output)
                return

        if "/proc/meminfo" in filename:
            output = load_cerberus_output("meminfo", ip=client_ip)
            if output:
                self.write(output)
                return

        if "/proc/mounts" in filename:
            # Show SquashFS (common in IoT) instead of overlay/docker
            arch = get_current_arch()
            self.write(f"/dev/root / squashfs ro,relatime 0 0\n")
            self.write(f"proc /proc proc rw,relatime 0 0\n")
            self.write(f"tmpfs /tmp tmpfs rw,relatime 0 0\n")
            return

        if "/dev/mtd" in filename:
            # Emulate flash memory access
            self.write("\x7fELF" + "".join([chr(random.randint(0, 255)) for _ in range(64)]))
            self.write("\ncat: read error: Input/output error\n") # Real routers often flake here
            return

        if "/dev/gpio" in filename:
            self.write("1\n") # Pin state
            return

        if "/etc/os-release" in filename or "/etc/issue" in filename:
            # These are usually handled by honeyfs, but we can override for consistency
            output = load_cerberus_output("os-release", ip=client_ip)
            if output:
                self.write(output)
                return

        if "hosts" in filename:
            from .cerberus_loader import load_network_config
            config = load_network_config()
            if config:
                hosts_content = "127.0.0.1\tlocalhost\n"
                iface = config.get('interfaces', [{}])[0]
                hosts_content += f"{iface.get('ip', '127.0.1.1')}\t{self.protocol.hostname}\n"

                # Add Carrots!
                for neighbor in config.get('neighbors', []):
                    hosts_content += f"{neighbor.get('ip')}\t{neighbor.get('hostname')}\n"

                self.write(hosts_content)
                return

        # Fallback to standard honeyfs behavior
        self.write(f"cat: {filename}: No such file or directory\n")

from cowrie.shell.command import HoneyPotCommand
commands = {}
commands['/bin/cat'] = command_cat
commands['cat'] = command_cat
