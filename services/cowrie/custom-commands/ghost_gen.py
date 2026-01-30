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
        self.baits = {
            "Router": [
                {"path": "/etc/config/wifi_keys.bak", "content": "config wifi-iface\n\toption key 'WPA_PSK_v5_A9B2'\n\toption ssid 'CERBERUS_NET'"},
                {"path": "/home/admin/admin_recovery.json", "content": "{\"admin\": \"recovery_admin_2026\", \"token\": \"0x8a2f...\"}"}
            ],
            "Camera": [
                {"path": "/etc/rtsp_auth.conf", "content": "admin:super_stealth_cam_99x\noperator:cam_remote_02"},
                {"path": "/var/www/html/private/stream_token", "content": "d6a7b8c9d0e1f2a3"}
            ],
            "Generic Device": [
                {"path": "/root/.ssh/id_rsa.bak", "content": "-----BEGIN RSA PRIVATE KEY-----\nMIIEpAIBAAKCAQEA7b...[TRUNCATED]..."},
                {"path": "/etc/shadow.bak", "content": "root:$6$rounds=5000$8723...:18330:0:99999:7:::"}
            ]
        }

    def _gen_who(self) -> str:
        """Simulates current active sessions."""
        active_users = random.sample(self.users, k=random.randint(1, 3))
        output = ""
        for i, user in enumerate(active_users):
            pts = f"pts/{i}"
            date = (datetime.now() - timedelta(minutes=random.randint(5, 120))).strftime("%Y-%m-%d %H:%M")
            ip = random.choice(self.ips)
            output += f"{user:<10} {pts:<10} {date} ({ip})\n"
        return output

    def generate(self, command: str, arch: str = "x86_64") -> str:
        cmd_words = command.strip().split()
        if not cmd_words:
            return "Command not found."

        base_cmd = cmd_words[0]

        if base_cmd == "who":
            return self._gen_who()
        if base_cmd == "whoami":
            return self._gen_whoami()
        if base_cmd == "last":
            return self._gen_last()
        if base_cmd in ["ps", "top"]:
            return self._gen_ps()
        if base_cmd == "uptime":
            return self._gen_uptime()
        if base_cmd == "uname":
            return self._gen_uname(arch)
        if base_cmd == "df":
            return self._gen_df()

        return f"sh: {base_cmd}: command not found"

    def _gen_whoami(self) -> str:
        return random.choice(self.users)

    def _gen_uptime(self) -> str:
        uptime = f"{random.randint(1, 400)} days, {random.randint(1, 23)}:{random.randint(10, 59)}"
        load = f"{random.uniform(0.1, 0.5):.2f}, {random.uniform(0.1, 0.5):.2f}, {random.uniform(0.1, 0.5):.2f}"
        return f" {time.strftime('%H:%M:%S')} up {uptime},  1 user,  load average: {load}"

    def _gen_uname(self, arch: str) -> str:
        return f"Linux CerberusNode 4.19.0-6-{arch} #1 SMP Debian 10.2-1 ({time.strftime('%Y-%m-%d')}) {arch} GNU/Linux"

    def _gen_df(self) -> str:
        return """Filesystem     1K-blocks      Used Available Use% Mounted on
/dev/sda1       41251136  12251136  29000000  30% /
tmpfs            4096000         0   4096000   0% /dev/shm
/dev/sdb1      104857600  45123440  59734160  44% /data"""

    def _gen_last(self, limit: int = 5) -> str:
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

    def _gen_ps(self) -> str:
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

    def get_baits(self, profile: str) -> list:
        """Get profile-specific honey-traps."""
        return self.baits.get(profile, self.baits["Generic Device"])
