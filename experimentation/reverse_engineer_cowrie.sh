#!/bin/bash
# CERBERUS Honeypot - Cowrie Reverse Engineering Suite
# This script uses every available reverse engineering tool

echo "╔══════════════════════════════════════════════════════════╗"
echo "║   COWRIE REVERSE ENGINEERING - COMPREHENSIVE ANALYSIS    ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""

COWRIE_DIR="research/cowrie"
OUTPUT_DIR="research/analysis"
mkdir -p "$OUTPUT_DIR"

echo "[*] Starting comprehensive reverse engineering..."
echo ""

# 1. CODE STRUCTURE ANALYSIS
echo "=== 1. CODE STRUCTURE ANALYSIS ==="
echo "[+] Analyzing directory structure..."
tree -L 3 "$COWRIE_DIR/src/cowrie" > "$OUTPUT_DIR/01_directory_structure.txt" 2>/dev/null || \
find "$COWRIE_DIR/src/cowrie" -type d | head -50 > "$OUTPUT_DIR/01_directory_structure.txt"

echo "[+] Counting code statistics..."
{
  echo "=== CODE STATISTICS ==="
  echo "Total Python files: $(find $COWRIE_DIR -name '*.py' | wc -l)"
  echo "Total lines of code: $(find $COWRIE_DIR -name '*.py' -exec wc -l {} + | tail -1)"
  echo "Total config files: $(find $COWRIE_DIR -name '*.cfg' -o -name '*.conf' | wc -l)"
  echo ""
} > "$OUTPUT_DIR/02_code_stats.txt"

# 2. DEPENDENCY ANALYSIS
echo "[+] Extracting dependencies..."
{
  echo "=== PYTHON DEPENDENCIES ==="
  cat "$COWRIE_DIR/requirements.txt" 2>/dev/null || echo "No requirements.txt"
  echo ""
  echo "=== IMPORTS ANALYSIS ==="
  grep -r "^import\|^from.*import" "$COWRIE_DIR/src" --include="*.py" | \
    cut -d: -f2 | sort | uniq -c | sort -rn | head -30
} > "$OUTPUT_DIR/03_dependencies.txt"

# 3. CLASS AND FUNCTION EXTRACTION
echo "[+] Extracting classes and functions..."
{
  echo "=== CLASSES ==="
  grep -r "^class " "$COWRIE_DIR/src" --include="*.py" | head -100
  echo ""
  echo "=== KEY FUNCTIONS ==="
  grep -r "^def " "$COWRIE_DIR/src" --include="*.py" | head -100
} > "$OUTPUT_DIR/04_classes_functions.txt"

# 4. SSH/TELNET PROTOCOL ANALYSIS
echo "[+] Analyzing protocol implementations..."
{
  echo "=== SSH IMPLEMENTATION ==="
  find "$COWRIE_DIR/src/cowrie/ssh" -name "*.py" -exec echo "File: {}" \; -exec head -20 {} \; -exec echo "" \;
  echo ""
  echo "=== TELNET IMPLEMENTATION ==="
  find "$COWRIE_DIR/src/cowrie/telnet" -name "*.py" -exec echo "File: {}" \; -exec head -20 {} \; -exec echo "" \;
} > "$OUTPUT_DIR/05_protocol_analysis.txt"

# 5. AUTHENTICATION MECHANISM
echo "[+] Analyzing authentication..."
grep -r "password\|auth\|login" "$COWRIE_DIR/src" --include="*.py" -A 3 -B 3 | head -200 > "$OUTPUT_DIR/06_authentication.txt"

# 6. COMMAND HANDLING
echo "[+] Analyzing command handling..."
{
  echo "=== AVAILABLE COMMANDS ==="
  ls -1 "$COWRIE_DIR/src/cowrie/commands/" 2>/dev/null
  echo ""
  echo "=== COMMAND IMPLEMENTATION SAMPLES ==="
  for cmd in ls cat wget curl; do
    echo "--- $cmd command ---"
    find "$COWRIE_DIR/src/cowrie/commands" -name "*${cmd}*" -exec cat {} \; 2>/dev/null | head -50
    echo ""
  done
} > "$OUTPUT_DIR/07_commands.txt"

# 7. FILESYSTEM EMULATION
echo "[+] Analyzing filesystem emulation..."
cat "$COWRIE_DIR/src/cowrie/shell/fs.py" 2>/dev/null | head -100 > "$OUTPUT_DIR/08_filesystem.txt"

# 8. SESSION MANAGEMENT
echo "[+] Analyzing session management..."
grep -r "session\|Session" "$COWRIE_DIR/src" --include="*.py" -A 5 | head -200 > "$OUTPUT_DIR/09_sessions.txt"

# 9. LOGGING MECHANISMS
echo "[+] Analyzing logging..."
{
  echo "=== LOGGING MODULES ==="
  ls -la "$COWRIE_DIR/src/cowrie/output/" 2>/dev/null
  echo ""
  echo "=== LOGGING CODE ==="
  grep -r "log\|Log" "$COWRIE_DIR/src/cowrie/output" --include="*.py" | head -100
} > "$OUTPUT_DIR/10_logging.txt"

# 10. CONFIGURATION PARSING
echo "[+] Analyzing configuration..."
{
  find "$COWRIE_DIR" -name "*.cfg" -o -name "*.cfg.dist" | while read cfg; do
    echo "=== $cfg ==="
    cat "$cfg"
    echo ""
  done
} > "$OUTPUT_DIR/11_configs.txt"

# 11. NETWORK CODE ANALYSIS
echo "[+] Analyzing network code..."
grep -r "socket\|Socket\|listen\|bind\|accept" "$COWRIE_DIR/src" --include="*.py" -A 3 | head -200 > "$OUTPUT_DIR/12_network.txt"

# 12. SECURITY MECHANISMS
echo "[+] Analyzing security features..."
grep -r "security\|sandbox\|chroot\|jail" "$COWRIE_DIR/src" --include="*.py" -A 5 | head -200 > "$OUTPUT_DIR/13_security.txt"

echo ""
echo "=== ANALYSIS COMPLETE ==="
echo "Results saved in: $OUTPUT_DIR/"
ls -lh "$OUTPUT_DIR/"
