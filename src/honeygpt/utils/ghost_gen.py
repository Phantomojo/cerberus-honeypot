import random
import time
from datetime import datetime, timedelta

# WHY THIS UTILITY: Most honeypots are "dead" environments (no active users,
# static process lists). The GhostGenerator creates a "Living System"
# simulation by generating dynamic logs, active sessions, and process noise.

class GhostGenerator:
    def __init__(self):
        self.users = ["admin", "root", "operator", "sys_maint", "dev_user"]
        self.ips = ["192.168.1.10", "10.0.0.5", "172.16.0.2", "127.0.0.1"]
        self.start_time = datetime.now() - timedelta(days=2)

    def generate_who_output(self) -> str:
        """Simulates current active sessions."""
        active_users = random.sample(self.users, k=random.randint(1, 3))
        output = ""
        for i, user in enumerate(active_users):
            pts = f"pts/{i}"
            date = (datetime.now() - timedelta(minutes=random.randint(5, 120))).strftime("%Y-%m-%d %H:%M")
            ip = random.choice(self.ips)
            output += f"{user:<10} {pts:<10} {date} ({ip})\n"
        return output

    def generate_last_output(self, limit: int = 5) -> str:
        """Simulates recent login history."""
        output = ""
        for i in range(limit):
            user = random.choice(self.users)
            pts = f"pts/{random.randint(0, 5)}"
            ip = random.choice(self.ips)
            time_str = (datetime.now() - timedelta(hours=i*2)).strftime("%a %b %d %H:%M")
            status = "still logged in" if i == 0 else f"down   ({random.randint(1, 59)}:00)"
            output += f"{user:<10} {pts:<10} {ip:<15} {time_str}   {status}\n"
        output += "\nwtmp begins Thu Jan  1 00:00:01 2026\n"
        return output

    def generate_process_noise(self) -> str:
        """Simulates a living kernel with active background tasks."""
        noise = [
            "[kworker/0:1-eve]",
            "[jbd2/sda1-8]",
            "syslogd -n",
            "/usr/sbin/sshd -D",
            "/usr/bin/python3 /opt/maint/health_check.py",
            "[ext4-rsv-conver]",
            "watchdog/0"
        ]
        chosen = random.sample(noise, k=random.randint(3, 5))
        output = ""
        for proc in chosen:
            pid = random.randint(100, 999)
            output += f"root      {pid:<6} 0.0  0.1  12450  2450 ?        S    Jan15   0:00 {proc}\n"
        return output

    def get_system_load(self) -> float:
        """Simulates fluctuating system load for realistic Jitter."""
        return round(random.uniform(0.1, 1.5), 2)
