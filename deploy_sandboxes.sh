#!/bin/bash

# Sandbox Deployment Script for Cerberus Honeypot Services
# This script sets up sandboxed environments for all honeypot services

echo "=== Deploying Cerberus Honeypot Sandboxes ==="

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Error: This script must be run as root for sandbox setup"
    exit 1
fi

# Create base sandbox directory
SANDBOX_BASE="/var/lib/cerberus"
echo "1. Creating sandbox base directory: $SANDBOX_BASE"
mkdir -p "$SANDBOX_BASE"

# Create sandbox users
echo "2. Creating sandbox users..."
for user in cowrie rtsp www-data; do
    if ! id "$user" >/dev/null 2>&1; then
        useradd -r -s /bin/false -d /nonexistent "$user"
        echo "   ✓ Created user: $user"
    else
        echo "   ✓ User already exists: $user"
    fi
done

# Setup Cowrie sandbox
echo "3. Setting up Cowrie sandbox..."
COWRIE_SANDBOX="$SANDBOX_BASE/cowrie-chroot"
mkdir -p "$COWRIE_SANDBOX"/{dev,proc,tmp,etc,var}

# Create minimal device nodes
mknod "$COWRIE_SANDBOX/dev/null" c 1 3 2>/dev/null
chmod 666 "$COWRIE_SANDBOX/dev/null" 2>/dev/null

# Copy essential configuration files
if [ -d "/etc/ssl" ]; then
    cp -r /etc/ssl "$COWRIE_SANDBOX/etc/" 2>/dev/null
    echo "   ✓ SSL certificates copied to Cowrie sandbox"
fi

# Setup RTSP sandbox
echo "4. Setting up RTSP sandbox..."
RTSP_SANDBOX="$SANDBOX_BASE/rtsp-chroot"
mkdir -p "$RTSP_SANDBOX"/{dev,proc,tmp,etc,var}

# Copy MediaMTX configuration
if [ -f "/etc/mediamtx.yml" ]; then
    cp /etc/mediamtx.yml "$RTSP_SANDBOX/etc/"
    echo "   ✓ MediaMTX configuration copied to RTSP sandbox"
fi

# Setup Web sandbox
echo "5. Setting up Web sandbox..."
WEB_SANDBOX="$SANDBOX_BASE/web-chroot"
mkdir -p "$WEB_SANDBOX"/{dev,proc,tmp,var,www}

# Create web directories
mkdir -p "$WEB_SANDBOX/var/www/html"
mkdir -p "$WEB_SANDBOX/var/log"

# Set proper permissions
echo "6. Setting proper permissions..."
chown -R cowrie:cowrie "$COWRIE_SANDBOX"
chown -R rtsp:rtsp "$RTSP_SANDBOX"
chown -R www-data:www-data "$WEB_SANDBOX"

chmod 755 "$SANDBOX_BASE"
chmod -R 750 "$COWRIE_SANDBOX"
chmod -R 750 "$RTSP_SANDBOX"
chmod -R 750 "$WEB_SANDBOX"

echo "   ✓ Permissions set for all sandboxes"

# Create systemd service files
echo "7. Creating systemd service files..."
cat > /etc/systemd/system/cerberus-cowrie@.service << 'EOL'
[Unit]
Description=Cerberus Cowrie Honeypot (Sandboxed)
After=network.target

[Service]
Type=simple
User=cowrie
Group=cowrie
ExecStart=/usr/bin/cowrie -c /etc/cowrie/cowrie.cfg
Restart=always
RestartSec=10
LimitNOFILE=1024
LimitAS=256M
LimitCPU=50%
PrivateTmp=yes
ProtectSystem=full
ProtectHome=yes
ReadWritePaths=/var/lib/cerberus/cowrie-chroot

[Install]
WantedBy=multi-user.target
EOL

cat > /etc/systemd/system/cerberus-rtsp@.service << 'EOL'
[Unit]
Description=Cerberus RTSP Honeypot (Sandboxed)
After=network.target

[Service]
Type=simple
User=rtsp
Group=rtsp
ExecStart=/usr/local/bin/mediamtx /etc/mediamtx.yml
Restart=always
RestartSec=10
LimitNOFILE=512
LimitAS=128M
LimitCPU=30%
PrivateTmp=yes
ProtectSystem=full
ProtectHome=yes
ReadWritePaths=/var/lib/cerberus/rtsp-chroot

[Install]
WantedBy=multi-user.target
EOL

cat > /etc/systemd/system/cerberus-web@.service << 'EOL'
[Unit]
Description=Cerberus Web Honeypot (Sandboxed)
After=network.target

[Service]
Type=simple
User=www-data
Group=www-data
ExecStart=/usr/bin/python3 /var/lib/cerberus/web-server.py
Restart=always
RestartSec=10
LimitNOFILE=256
LimitAS=64M
LimitCPU=20%
PrivateTmp=yes
ProtectSystem=full
ProtectHome=yes
ReadWritePaths=/var/lib/cerberus/web-chroot

[Install]
WantedBy=multi-user.target
EOL

# Reload systemd
systemctl daemon-reload
echo "   ✓ Systemd service files created"

# Create monitoring script
echo "8. Creating sandbox monitoring script..."
cat > /usr/local/bin/cerberus-sandbox-monitor.sh << 'EOM'
#!/bin/bash

# Sandbox Monitoring Script for Cerberus Honeypot
# Monitors sandbox integrity and resource usage

SERVICES=("cowrie" "rtsp" "web")
SANDBOX_BASE="/var/lib/cerberus"

check_sandbox_health() {
    local service=$1
    local sandbox_path="$SANDBOX_BASE/$service-chroot"

    # Check if sandbox directory exists
    if [ ! -d "$sandbox_path" ]; then
        echo "ALERT: Sandbox directory missing for $service"
        return 1
    fi

    # Check if service process is running
    if ! pgrep -f "$service" > /dev/null; then
        echo "ALERT: Service $service is not running"
        return 1
    fi

    # Check resource usage
    local pid=$(pgrep -f "$service" | head -1)
    if [ -n "$pid" ]; then
        local memory=$(ps -p "$pid" -o rss= | tail -1)
        local memory_mb=$((memory / 1024))

        case $service in
            "cowrie")
                if [ $memory_mb -gt 256 ]; then
                    echo "ALERT: Cowrie memory usage high: ${memory_mb}MB"
                fi
                ;;
            "rtsp")
                if [ $memory_mb -gt 128 ]; then
                    echo "ALERT: RTSP memory usage high: ${memory_mb}MB"
                fi
                ;;
            "web")
                if [ $memory_mb -gt 64 ]; then
                    echo "ALERT: Web memory usage high: ${memory_mb}MB"
                fi
                ;;
        esac
    fi

    return 0
}

# Main monitoring loop
while true; do
    for service in "${SERVICES[@]}"; do
        check_sandbox_health "$service"
    done

    sleep 30
done
EOM

chmod +x /usr/local/bin/cerberus-sandbox-monitor.sh

echo "   ✓ Sandbox monitoring script created"

echo ""
echo "=== Sandbox Deployment Complete ==="
echo "Summary:"
echo "- Sandboxes created: ✓"
echo "- Users created: ✓"
echo "- Permissions set: ✓"
echo "- Systemd services: ✓"
echo "- Monitoring script: ✓"
echo ""
echo "To start services:"
echo "  systemctl start cerberus-cowrie@.service"
echo "  systemctl start cerberus-rtsp@.service"
echo "  systemctl start cerberus-web@.service"
echo ""
echo "To start monitoring:"
echo "  systemctl start cerberus-sandbox-monitor"
echo ""
echo "Sandbox directories:"
echo "- Cowrie: $COWRIE_SANDBOX"
echo "- RTSP: $RTSP_SANDBOX"
echo "- Web: $WEB_SANDBOX"
