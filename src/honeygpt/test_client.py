import socket
import sys

def test_honeygpt(command="uname -a"):
    host = 'localhost'
    port = 50051

    try:
        print(f"[*] Connecting to HoneyGPT on {host}:{port}...")
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.settimeout(10)
        client.connect((host, port))

        print(f"[*] Sending command: {command}")
        client.sendall(command.encode('utf-8'))

        response = client.recv(4096).decode('utf-8')
        print(f"[✓] Received Response:\n{response}")

    except Exception as e:
        print(f"[✗] Error: {e}")
    finally:
        client.close()

if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else "uname -a"
    test_honeygpt(cmd)
