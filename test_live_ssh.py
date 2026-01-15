#!/usr/bin/env python3
"""
Live SSH test script for Cerberus honeypot
"""
import paramiko
import time
import sys

def test_ssh_commands():
    """Test SSH connection and custom commands"""

    # SSH connection details
    host = 'localhost'
    port = 2222
    username = 'root'
    password = 'admin'

    print("=" * 70)
    print("CERBERUS LIVE SSH TEST")
    print("=" * 70)
    print(f"\n[*] Connecting to {host}:{port}...")

    try:
        # Create SSH client
        client = paramiko.SSHClient()
        client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

        # Connect
        client.connect(host, port=port, username=username, password=password, timeout=10)
        print(f"[✓] Connected successfully!\n")

        # Commands to test
        commands = [
            ("uname -a", "System info (from Cerberus config)"),
            ("cat /proc/cpuinfo", "CPU info (from Cerberus honeyfs)"),
            ("route -n", "Routing table (CERBERUS CUSTOM COMMAND)"),
            ("docker ps", "Docker output (CERBERUS CUSTOM COMMAND)"),
            ("ifconfig", "Network interfaces"),
            ("uptime", "System uptime"),
            ("ps aux", "Process list"),
        ]

        for cmd, description in commands:
            print(f"\n{'=' * 70}")
            print(f"TEST: {description}")
            print(f"CMD:  {cmd}")
            print("-" * 70)

            try:
                stdin, stdout, stderr = client.exec_command(cmd, timeout=5)
                output = stdout.read().decode('utf-8', errors='ignore')
                error = stderr.read().decode('utf-8', errors='ignore')

                if output:
                    # Truncate long outputs
                    lines = output.split('\n')
                    if len(lines) > 15:
                        print('\n'.join(lines[:15]))
                        print(f"... ({len(lines) - 15} more lines)")
                    else:
                        print(output)

                if error:
                    print(f"STDERR: {error}")

                print(f"[✓] Command executed")
                time.sleep(0.5)

            except Exception as e:
                print(f"[✗] Error: {e}")

        client.close()
        print(f"\n{'=' * 70}")
        print("[✓] SSH session closed successfully")
        print("=" * 70)

        return True

    except paramiko.AuthenticationException:
        print("[✗] Authentication failed - credentials rejected")
        return False
    except paramiko.SSHException as e:
        print(f"[✗] SSH error: {e}")
        return False
    except Exception as e:
        print(f"[✗] Unexpected error: {e}")
        return False

if __name__ == "__main__":
    success = test_ssh_commands()
    sys.exit(0 if success else 1)
