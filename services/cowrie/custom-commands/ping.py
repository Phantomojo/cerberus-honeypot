"""
Cerberus Dynamic Ping Command for Cowrie
Simulates connectivity to neighboring 'carrot' targets.
"""

from __future__ import annotations
import random
import time
from cowrie.shell.command import HoneyPotCommand
from .cerberus_loader import load_network_config

class command_ping(HoneyPotCommand):
    def call(self):
        if not self.args:
            self.write("Usage: ping [-c count] destination\n")
            return

        destination = self.args[-1]
        count = 4

        # Check if -c is specified
        if "-c" in self.args:
            try:
                idx = self.args.index("-c")
                count = int(self.args[idx+1])
            except (ValueError, IndexError):
                pass

        config = load_network_config()
        neighbors = config.get('neighbors', []) if config else []
        neighbor_ips = [n.get('ip') for n in neighbors]
        neighbor_hosts = [n.get('hostname') for n in neighbors]

        is_neighbor = destination in neighbor_ips or destination in neighbor_hosts

        self.write(f"PING {destination} ({destination}) 56(84) bytes of data.\n")

        if is_neighbor or destination == "127.0.0.1":
            for i in range(count):
                time.sleep(0.1 + random.random() * 0.2)
                icmp_seq = i + 1
                ttl = 64
                rtt = 10.0 + random.random() * 20.0
                self.write(f"64 bytes from {destination}: icmp_seq={icmp_seq} ttl={ttl} time={rtt:.1f} ms\n")

            self.write(f"\n--- {destination} ping statistics ---\n")
            self.write(f"{count} packets transmitted, {count} received, 0% packet loss, time {count*100}ms\n")
            self.write(f"rtt min/avg/max/mdev = 10.0/20.0/30.0/5.0 ms\n")
        else:
            time.sleep(2.0)
            self.write(f"From {self.protocol.hostname} icmp_seq=1 Destination Host Unreachable\n")

from cowrie.shell.command import HoneyPotCommand
commands = {}
commands['/bin/ping'] = command_ping
commands['ping'] = command_ping
