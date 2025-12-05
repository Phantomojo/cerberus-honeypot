#!/bin/bash
# Deep Reverse Engineering Analysis

OUTPUT="experimentation/analysis"
COWRIE="experimentation/cowrie"

echo "╔══════════════════════════════════════════════════════════╗"
echo "║       DEEP REVERSE ENGINEERING ANALYSIS                  ║"
echo "╚══════════════════════════════════════════════════════════╝"

# 1. Call graph analysis
echo "[+] Analyzing function call relationships..."
{
  echo "=== FUNCTION CALL GRAPH ==="
  grep -r "def \|    self\." "$COWRIE/src/cowrie/ssh" --include="*.py" | head -100
} > "$OUTPUT/14_call_graph.txt"

# 2. Data flow analysis
echo "[+] Analyzing data flow..."
{
  echo "=== DATA FLOW: AUTH -> SHELL -> COMMAND ==="
  echo ""
  echo "--- Authentication Flow ---"
  grep -A 10 "checkPassword\|passwordReceived" "$COWRIE/src/cowrie/ssh/userauth.py" "$COWRIE/src/cowrie/telnet/userauth.py" 2>/dev/null
  echo ""
  echo "--- Shell Initialization ---"
  grep -A 10 "lineReceived\|execCommand" "$COWRIE/src/cowrie/shell"/*.py 2>/dev/null | head -50
} > "$OUTPUT/15_data_flow.txt"

# 3. State machine extraction
echo "[+] Extracting state machines..."
{
  echo "=== STATE MACHINES ==="
  grep -r "state\|State\|STATE" "$COWRIE/src" --include="*.py" -B 2 -A 2 | head -200
} > "$OUTPUT/16_state_machines.txt"

# 4. Configuration deep dive
echo "[+] Deep configuration analysis..."
{
  echo "=== CONFIGURATION PARAMETERS ==="
  grep -r "CowrieConfig.get\|config.get" "$COWRIE/src" --include="*.py" | head -100
} > "$OUTPUT/17_config_params.txt"

# 5. Command execution flow
echo "[+] Analyzing command execution..."
{
  echo "=== COMMAND EXECUTION FLOW ==="
  grep -A 20 "def lineReceived" "$COWRIE/src/cowrie/shell/honeypot.py"
  echo ""
  echo "=== COMMAND DISPATCH ==="
  grep -A 20 "def call_command" "$COWRIE/src/cowrie/shell/honeypot.py"
} > "$OUTPUT/18_command_execution.txt"

# 6. Filesystem operations
echo "[+] Analyzing filesystem operations..."
{
  echo "=== FILESYSTEM API ==="
  grep "def " "$COWRIE/src/cowrie/shell/fs.py" | head -50
} > "$OUTPUT/19_filesystem_api.txt"

# 7. Session lifecycle
echo "[+] Tracing session lifecycle..."
{
  echo "=== SESSION LIFECYCLE ==="
  grep -r "connectionMade\|connectionLost\|lineReceived\|terminalProtocol" "$COWRIE/src/cowrie/ssh" --include="*.py" -A 5 | head -100
} > "$OUTPUT/20_session_lifecycle.txt"

# 8. Security mechanisms
echo "[+] Analyzing security implementations..."
{
  echo "=== SECURITY CHECKS ==="
  grep -r "verify\|Verify\|check\|Check\|validate\|Validate" "$COWRIE/src" --include="*.py" -B 2 -A 5 | head -200
} > "$OUTPUT/21_security_checks.txt"

# 9. Output/logging plugins
echo "[+] Analyzing output plugins..."
{
  echo "=== OUTPUT PLUGINS ==="
  ls -la "$COWRIE/src/cowrie/output/"
  echo ""
  for plugin in "$COWRIE/src/cowrie/output"/*.py; do
    echo "--- $(basename "$plugin") ---"
    head -30 "$plugin"
    echo ""
  done
} > "$OUTPUT/22_output_plugins.txt"

# 10. Protocol message handlers
echo "[+] Extracting protocol handlers..."
{
  echo "=== SSH MESSAGE HANDLERS ==="
  grep "ssh_" "$COWRIE/src/cowrie/ssh"/*.py | head -100
  echo ""
  echo "=== TELNET NEGOTIATION ==="
  grep "telnet\|IAC\|WILL\|WON'T" "$COWRIE/src/cowrie/telnet"/*.py | head -100
} > "$OUTPUT/23_protocol_handlers.txt"

# 11. Create architecture diagram (text-based)
echo "[+] Generating architecture diagram..."
{
  cat << 'DIAGRAM'
=== COWRIE ARCHITECTURE (Text Diagram) ===

┌─────────────────────────────────────────────────────────────┐
│                    EXTERNAL CONNECTIONS                      │
│                    (Attacker connects)                       │
└─────────────┬───────────────────────────┬───────────────────┘
              │                           │
              │ SSH (port 2222)           │ Telnet (port 2323)
              │                           │
    ┌─────────▼─────────┐       ┌────────▼─────────┐
    │  SSH Transport    │       │ Telnet Transport │
    │  (transport.py)   │       │  (transport.py)  │
    └─────────┬─────────┘       └────────┬─────────┘
              │                           │
    ┌─────────▼─────────┐       ┌────────▼─────────┐
    │  SSH UserAuth     │       │ Telnet UserAuth  │
    │  (userauth.py)    │       │  (userauth.py)   │
    └─────────┬─────────┘       └────────┬─────────┘
              │                           │
              └───────────┬───────────────┘
                          │
                ┌─────────▼─────────┐
                │   Session Manager │
                │   (session.py)    │
                └─────────┬─────────┘
                          │
                ┌─────────▼─────────┐
                │  Shell Protocol   │
                │  (protocol.py)    │
                │  - lineReceived() │
                │  - call_command() │
                └─────────┬─────────┘
                          │
         ┌────────────────┼────────────────┐
         │                │                │
┌────────▼────────┐ ┌────▼────┐  ┌───────▼────────┐
│ Command Handler │ │ VirtFS  │  │ File Transfer  │
│ (commands/*.py) │ │ (fs.py) │  │ (filetransfer) │
└────────┬────────┘ └────┬────┘  └───────┬────────┘
         │               │                │
         └───────────────┼────────────────┘
                         │
                ┌────────▼────────┐
                │  Output Plugins │
                │  (output/*.py)  │
                │  - JSON log     │
                │  - Text log     │
                │  - MySQL        │
                │  - Splunk       │
                └─────────────────┘

DATA FLOW:
1. Connection → Transport → Auth → Session
2. Command Input → Protocol → Command Handler
3. Command Output → Protocol → Transport → Attacker
4. Logging → Output Plugins → Storage

KEY FILES:
- ssh/transport.py     : SSH protocol negotiation
- ssh/userauth.py      : SSH authentication
- telnet/transport.py  : Telnet protocol
- shell/protocol.py    : Command line handling
- shell/honeypot.py    : Main shell logic (21K lines!)
- shell/fs.py          : Virtual filesystem
- commands/*.py        : Individual command impls
- output/*.py          : Logging backends
DIAGRAM
} > "$OUTPUT/24_architecture_diagram.txt"

# 12. Extract key algorithms
echo "[+] Extracting key algorithms..."
{
  echo "=== KEY ALGORITHMS ==="
  echo ""
  echo "--- Command Parser ---"
  grep -A 30 "def parse_command\|def lineReceived" "$COWRIE/src/cowrie/shell/honeypot.py" | head -50
  echo ""
  echo "--- Path Resolution ---"
  grep -A 20 "def resolve_path\|def get_path" "$COWRIE/src/cowrie/shell/fs.py" | head -40
} > "$OUTPUT/25_algorithms.txt"

# 13. Extract interesting patterns
echo "[+] Extracting code patterns..."
{
  echo "=== INTERESTING CODE PATTERNS ==="
  echo ""
  echo "--- Twisted Deferred Usage ---"
  grep -r "defer\|Deferred" "$COWRIE/src/cowrie/ssh" --include="*.py" -A 3 | head -50
  echo ""
  echo "--- Error Handling ---"
  grep -r "try:\|except\|raise" "$COWRIE/src/cowrie/shell/honeypot.py" -A 2 | head -50
} > "$OUTPUT/26_code_patterns.txt"

echo ""
echo "=== DEEP ANALYSIS COMPLETE ==="
ls -lh "$OUTPUT/"
