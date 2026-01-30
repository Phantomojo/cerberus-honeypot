import paramiko
import sys

def prove_fix():
    print("[*] Connecting to Cowrie SSH honeypot (localhost:2222)...")
    client = paramiko.SSHClient()
    client.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    try:
        # Cowrie accepts any password by default if not configured otherwise,
        # but our userdb has root/root.
        client.connect('localhost', port=2222, username='root', password='root', timeout=5)

        # Run a single string with all commands to avoid channel closing issues
        cmd = 'uname -a; hostname; arch'
        stdin, stdout, stderr = client.exec_command(cmd)
        output = stdout.read().decode('utf-8').strip()
        print("-" * 40)
        print(f"$ {cmd}")
        print(output)
        print("-" * 40)

        client.close()
    except Exception as e:
        print(f"[!] Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    prove_fix()
