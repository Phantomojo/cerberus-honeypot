"""
HoneyGPT Bridge Command for Cowrie
This script is executed by Cowrie when it encounters a command it doesn't know.
It forwards the command to the HoneyGPT AI service and returns the LLM-generated output.
"""

from __future__ import annotations
import socket
import sys

# HoneyGPT service address (inside Docker network)
HONEYGPT_HOST = "honeygpt"
HONEYGPT_PORT = 50051

def query_honeygpt(command: str) -> str:
    """Send command to HoneyGPT service and get response."""
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(30) # AI can be slow
            s.connect((HONEYGPT_HOST, HONEYGPT_PORT))
            s.sendall(command.encode('utf-8'))
            response = s.recv(8192).decode('utf-8')
            return response
    except Exception as e:
        return f"sh: {command}: command not found"

if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(1)

    cmd = " ".join(sys.argv[1:])
    output = query_honeygpt(cmd)
    print(output)
