#!/usr/bin/env bash
# Add missing commands to Cowrie's dynamic filesystem (relative paths)
# This script creates fake command outputs that will be served by Cowrie.
# Call this after running `make all` so build directory exists.

set -euo pipefail

# Resolve repository root to ensure relative paths work regardless of current dir
REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build"
DYNAMIC_DIR="${BUILD_DIR}/cowrie-dynamic"

# Create bin directories if needed
mkdir -p "${DYNAMIC_DIR}/bin"
mkdir -p "${DYNAMIC_DIR}/sbin"
mkdir -p "${DYNAMIC_DIR}/usr/bin"

# docker - container management
cat > "${DYNAMIC_DIR}/bin/docker" << 'EOF'
CONTAINER ID   IMAGE               COMMAND                  CREATED        STATUS
a1b2c3d4e5f6   ubuntu:20.04        "/bin/bash"              2 days ago     Up 2 days
f6e5d4c3b2a1   nginx:latest        "nginx -g daemon..."     1 week ago     Up 1 week
b7c8d9e0f1a2   postgres:13         "docker-entrypoint..."   1 week ago     Up 1 week
EOF

# systemctl - service management
cat > "${DYNAMIC_DIR}/bin/systemctl" << 'EOF'
  UNIT                     LOAD   ACTIVE SUB    DESCRIPTION
  ssh.service              loaded active running OpenSSH server
  nginx.service            loaded active running A high performance web server
  postgresql.service       loaded active running PostgreSQL Database Server
  docker.service           loaded active running Docker Application Container Engine
  rsyslog.service          loaded active running System Logging Service
EOF

# python --version
cat > "${DYNAMIC_DIR}/usr/bin/python" << 'EOF'
Python 3.8.10
EOF
chmod +x "${DYNAMIC_DIR}/usr/bin/python"

# git --version
cat > "${DYNAMIC_DIR}/bin/git" << 'EOF'
git version 2.34.1
EOF
chmod +x "${DYNAMIC_DIR}/bin/git"

# curl --version
cat > "${DYNAMIC_DIR}/bin/curl" << 'EOF'
curl 7.68.0 (x86_64-pc-linux-gnu) libcurl/7.68.0
Release-Date: 2020-01-08
Protocols: file ftp ftps http https imap imaps pop3 pop3s rtsp smb smbs smtp smtps telnet tftp
Features: AsynchDNS GSS-API HTTP2 HTTPS-only Kerberos Largefile libz NTLM NTLM_WB SPNEGO SSL UnixSockets
EOF
chmod +x "${DYNAMIC_DIR}/bin/curl"

# java --version
cat > "${DYNAMIC_DIR}/usr/bin/java" << 'EOF'
openjdk version "11.0.11" 2021-04-06
OpenJDK Runtime Environment (build 11.0.11+9-post-Ubuntu-0ubuntu1.20.04)
OpenJDK 64-Bit Server VM (build 11.0.11+9-post-Ubuntu-0ubuntu1.20.04, mixed mode, sharing)
EOF
chmod +x "${DYNAMIC_DIR}/usr/bin/java"

# node --version
cat > "${DYNAMIC_DIR}/usr/bin/node" << 'EOF'
v14.17.0
EOF
chmod +x "${DYNAMIC_DIR}/usr/bin/node"

# Optional: provide a few common system commands
cat > "${DYNAMIC_DIR}/bin/ls" << 'EOF'
bin  sbin  usr
EOF
chmod +x "${DYNAMIC_DIR}/bin/ls"

cat > "${DYNAMIC_DIR}/bin/find" << 'EOF'
/bin/docker
/bin/systemctl
/usr/bin/python
/usr/bin/java
/usr/bin/node
/bin/git
/bin/curl
EOF
chmod +x "${DYNAMIC_DIR}/bin/find"

cat > "${DYNAMIC_DIR}/bin/netstat" << 'EOF'
Active Internet connections (servers and established)
Proto Recv-Q Send-Q Local Address           Foreign Address         State
tcp        0      0 0.0.0.0:ssh             0.0.0.0:*               LISTEN
tcp        0      0 127.0.0.1:postgres      0.0.0.0:*               LISTEN
EOF
chmod +x "${DYNAMIC_DIR}/bin/netstat"

cat > "${DYNAMIC_DIR}/bin/route" << 'EOF'
Kernel IP routing table
Destination     Gateway         Genmask         Flags Metric Ref    Use Iface
0.0.0.0         192.168.1.1     0.0.0.0         UG    100    0        0 eth0
EOF
chmod +x "${DYNAMIC_DIR}/bin/route"

# Summary output
echo "[+] Dynamic commands added to ${DYNAMIC_DIR}"
echo "[+] Files created:"
find "${DYNAMIC_DIR}" -type f -exec ls -lh {} \;
