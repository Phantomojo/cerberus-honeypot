# CERBERUS Honeynet - Dependencies & Setup Requirements

## System Requirements

- **OS**: Linux (Ubuntu/Debian recommended, Arch Linux supported)
- **Architecture**: x86_64
- **RAM**: Minimum 2GB, recommended 4GB+
- **Disk**: Minimum 5GB free space

## Core Dependencies

### Build Tools
- **GCC** (GNU Compiler Collection) - Version 9.0+ recommended
  - Required for compiling C modules (morph, quorum, utils)
  - Install: `sudo apt install build-essential` (Debian/Ubuntu) or `sudo pacman -S base-devel` (Arch)

### Container Runtime
- **Docker** - Version 20.10+
  - Required for running honeypot services in containers
  - Install: Follow [Docker official installation guide](https://docs.docker.com/get-docker/)
- **Docker Compose** - Version 2.0+
  - Required for orchestrating multiple services
  - Usually included with Docker Desktop, or install separately

### Web Server
- **nginx** - Version 1.18+
  - Used for serving fake router and camera web UIs
  - Install: `sudo apt install nginx` (Debian/Ubuntu) or `sudo pacman -S nginx` (Arch)
  - Note: Can also run in Docker containers

### Python Runtime (for Cowrie)
- **Python 3.8+** - Required for Cowrie honeypot
- **pip** - Python package manager
- **virtualenv** (recommended) - For isolated Python environments
  - Install: `sudo apt install python3-pip python3-venv` (Debian/Ubuntu)

## Service-Specific Dependencies

### Cowrie Honeypot
- Python 3.8+
- Python packages (see `requirements.txt`):
  - cowrie (honeypot framework)
  - twisted (network framework)
  - pyasn1 (ASN.1 library)
  - cryptography (encryption support)
  - Additional dependencies listed in Cowrie's requirements

### RTSP Server
- **mediamtx** (formerly rtsp-simple-server) or similar RTSP server
  - Can run as Docker container
  - Alternative: Custom RTSP server implementation

### Dashboard (Optional)
- **Flask** (Python) or **CGI** (C) - For web dashboard
- If using Flask: Python 3.8+, Flask, and related packages

## Network Requirements

- **Ports to expose** (configurable):
  - `22` - SSH (Cowrie)
  - `23` - Telnet (Cowrie)
  - `80` - HTTP (Router web UI)
  - `8080` - HTTP (Camera web UI, optional)
  - `554` - RTSP (Camera stream)
  - `5000` - Dashboard (if using Flask, optional)

⚠️ **Security Note**: Only expose these ports in a controlled environment (isolated network, VM, or cloud instance). Never expose honeypots on production networks.

## Development Dependencies (Optional)

- **Git** - Version control
- **Make** - Build automation (usually included with build-essential)
- **Valgrind** - Memory debugging (optional)
- **GDB** - Debugger (optional)

## Quick Install Commands

### Debian/Ubuntu
```bash
sudo apt update
sudo apt install -y build-essential python3 python3-pip python3-venv nginx git make
```

### Arch Linux
```bash
sudo pacman -Syu
sudo pacman -S base-devel python python-pip nginx git make
```

### Docker (all distributions)
Follow official Docker installation guide for your distribution.

## Verification

After installation, verify dependencies:
```bash
gcc --version
docker --version
docker compose version
python3 --version
nginx -v
```

## Next Steps

1. Run the setup script: `./setup.sh`
2. Build C modules: `make`
3. Start services: `cd docker && docker compose up -d`
