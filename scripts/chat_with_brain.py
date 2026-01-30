import socket
import json
import sys

def chat_with_brain(command):
    host = "127.0.0.1"
    port = 50051

    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(60)
            s.connect((host, port))
            s.sendall(command.encode('utf-8'))
            response = s.recv(8192).decode('utf-8')

            try:
                data = json.loads(response)
                print(f"\n[BRAIN HOSTNAME]: {data.get('hostname', 'unknown')}")
                print(f"[RESPONSE]:\n{data.get('response', 'No response')}\n")
            except:
                print(f"\n[RAW RESPONSE]:\n{response}\n")

    except Exception as e:
        print(f"Error connecting to brain: {e}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        cmd = " ".join(sys.argv[1:])
        chat_with_brain(cmd)
    else:
        print("Cerberus Brain CLI - Type 'exit' to quit")
        while True:
            cmd = input("Attacker Shell> ")
            if cmd.lower() in ["exit", "quit"]:
                break
            chat_with_brain(cmd)
