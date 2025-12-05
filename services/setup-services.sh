#!/bin/bash
# Setup script for individual CERBERUS services
# This script prepares service configurations and dependencies

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "Setting up CERBERUS services..."

# Setup Cowrie
setup_cowrie() {
    echo -e "${GREEN}Setting up Cowrie honeypot...${NC}"

    if [ ! -d "services/cowrie" ]; then
        mkdir -p services/cowrie/{logs,data,etc}
    fi

    # Create basic Cowrie config if it doesn't exist
    if [ ! -f "services/cowrie/etc/cowrie.cfg" ]; then
        cat > services/cowrie/etc/cowrie.cfg <<EOF
[output_jsonlog]
enabled = true
logfile = log/cowrie.json

[output_textlog]
enabled = true
logfile = log/cowrie.log
EOF
        echo "Created basic Cowrie configuration"
    fi

    # Clone Cowrie if not present (optional, can use Docker image)
    if [ ! -d "services/cowrie/cowrie" ]; then
        echo "Cowrie source not found. Using Docker image (recommended)."
        echo "To install Cowrie from source, run:"
        echo "  cd services/cowrie && git clone https://github.com/cowrie/cowrie.git"
    fi
}

# Setup Router Web UI
setup_router_web() {
    echo -e "${GREEN}Setting up fake router web UI...${NC}"

    mkdir -p services/fake-router-web/{html,conf}

    # Create nginx config
    cat > services/fake-router-web/conf/nginx.conf <<'EOF'
events {
    worker_connections 1024;
}

http {
    include       /etc/nginx/mime.types;
    default_type  application/octet-stream;

    sendfile        on;
    keepalive_timeout  65;

    server {
        listen       80;
        server_name  localhost;
        root         /usr/share/nginx/html;
        index        index.html;

        location / {
            try_files $uri $uri/ /index.html;
        }

        access_log  /var/log/nginx/router-access.log;
        error_log   /var/log/nginx/router-error.log;
    }
}
EOF

    # Create basic router HTML
    cat > services/fake-router-web/html/index.html <<'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>Router Admin Panel</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f0f0f0; }
        .container { background: white; padding: 20px; border-radius: 8px; max-width: 600px; }
        h1 { color: #333; }
        .status { color: green; }
    </style>
</head>
<body>
    <div class="container">
        <h1>Router Administration</h1>
        <p class="status">Status: Online</p>
        <p>Welcome to the router configuration panel.</p>
        <p><em>This is a honeypot emulation.</em></p>
    </div>
</body>
</html>
EOF

    echo "Router web UI configured"
}

# Setup Camera Web UI
setup_camera_web() {
    echo -e "${GREEN}Setting up fake camera web UI...${NC}"

    mkdir -p services/fake-camera-web/{html,conf}

    # Create nginx config
    cat > services/fake-camera-web/conf/nginx.conf <<'EOF'
events {
    worker_connections 1024;
}

http {
    include       /etc/nginx/mime.types;
    default_type  application/octet-stream;

    sendfile        on;
    keepalive_timeout  65;

    server {
        listen       80;
        server_name  localhost;
        root         /usr/share/nginx/html;
        index        index.html;

        location / {
            try_files $uri $uri/ /index.html;
        }

        access_log  /var/log/nginx/camera-access.log;
        error_log   /var/log/nginx/camera-error.log;
    }
}
EOF

    # Create basic camera HTML
    cat > services/fake-camera-web/html/index.html <<'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>CCTV Camera View</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #000; }
        .container { background: #1a1a1a; padding: 20px; border-radius: 8px; max-width: 800px; color: #fff; }
        h1 { color: #fff; }
        .video-placeholder { background: #333; padding: 100px; text-align: center; border-radius: 4px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>CCTV Camera Feed</h1>
        <div class="video-placeholder">
            <p>Camera Stream Placeholder</p>
            <p><em>RTSP: rtsp://localhost:554/stream</em></p>
        </div>
        <p><em>This is a honeypot emulation.</em></p>
    </div>
</body>
</html>
EOF

    echo "Camera web UI configured"
}

# Setup RTSP Server
setup_rtsp() {
    echo -e "${GREEN}Setting up RTSP server...${NC}"

    mkdir -p services/rtsp/{conf,media}

    # Create MediaMTX config
    cat > services/rtsp/conf/mediamtx.yml <<'EOF'
# MediaMTX configuration for CERBERUS honeynet
paths:
  all:
    source: publisher
    sourceOnDemand: yes
    sourceOnDemandStartTimeout: 10s
    sourceOnDemandCloseAfter: 10s
    runOnInit: echo "RTSP stream started"
    runOnInitRestart: yes
EOF

    echo "RTSP server configured"
}

# Main
main() {
    setup_cowrie
    echo ""
    setup_router_web
    echo ""
    setup_camera_web
    echo ""
    setup_rtsp
    echo ""
    echo -e "${GREEN}All services configured!${NC}"
    echo ""
    echo "Next steps:"
    echo "  1. Review configurations in services/*/"
    echo "  2. Start services: cd docker && docker compose up -d"
}

main
