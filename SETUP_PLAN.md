# CERBERUS Honeynet - Setup & Architecture Plan

## Overview

This document outlines the setup plan, architecture, and implementation roadmap for the CERBERUS bio-adaptive IoT honeynet project.

## Architecture Components

### 1. Core C Modules (Built Locally)
- **Morphing Engine** (`build/morph`)
  - Rotates device fingerprints
  - Modifies Cowrie banners
  - Swaps web UI themes
  - Runs as daemon or cron job
  
- **Quorum Engine** (`build/quorum`)
  - Scans logs from all services
  - Detects coordinated attacks (same IP across multiple services)
  - Generates alerts
  - Runs as daemon or cron job

- **Utilities** (`src/utils/`)
  - Logging functions
  - File I/O helpers
  - Network utilities

### 2. Honeypot Services (Docker Containers)

#### Cowrie (SSH/Telnet Honeypot)
- **Purpose**: Emulate vulnerable IoT device SSH/Telnet
- **Ports**: 2222 (SSH), 2323 (Telnet)
- **Logs**: `/services/cowrie/logs/`
- **Config**: `/services/cowrie/etc/`
- **Morphing**: C engine modifies banners in config

#### Fake Router Web UI (nginx)
- **Purpose**: Emulate router admin interface
- **Port**: 80
- **HTML**: `/services/fake-router-web/html/`
- **Morphing**: C engine swaps HTML themes

#### Fake Camera Web UI (nginx)
- **Purpose**: Emulate CCTV camera interface
- **Port**: 8080
- **HTML**: `/services/fake-camera-web/html/`
- **Morphing**: C engine swaps HTML themes

#### RTSP Server (MediaMTX)
- **Purpose**: Emulate camera video stream
- **Ports**: 554 (RTSP), 8554 (alternative), 1935 (RTMP)
- **Config**: `/services/rtsp/conf/`
- **Media**: `/services/rtsp/media/`

#### Dashboard (Optional - Flask)
- **Purpose**: View logs and alerts
- **Port**: 5000
- **Location**: `/dashboard/`
- **Data**: Reads from `/logs/` and `/build/`

## Setup Workflow

### Phase 1: Initial Setup
1. ✅ Clone repository
2. ✅ Run `./setup.sh` to install dependencies
3. ✅ Run `./services/setup-services.sh` to configure services
4. ✅ Build C modules: `make`

### Phase 2: Service Configuration
1. Configure Cowrie:
   - Edit `services/cowrie/etc/cowrie.cfg`
   - Set up banners for morphing
   
2. Configure Web UIs:
   - Create multiple HTML themes in `services/fake-router-web/html/themes/`
   - Create multiple HTML themes in `services/fake-camera-web/html/themes/`
   
3. Configure RTSP:
   - Add test video files to `services/rtsp/media/`
   - Configure stream paths in `services/rtsp/conf/mediamtx.yml`

### Phase 3: Docker Deployment
1. Review `docker/docker-compose.yml`
2. Start services: `cd docker && docker compose up -d`
3. Verify services are running: `docker compose ps`
4. Check logs: `docker compose logs -f`

### Phase 4: C Engine Integration
1. Implement morphing logic:
   - File modification functions
   - Banner rotation
   - Theme swapping
   - Logging
   
2. Implement quorum logic:
   - Log file parsing
   - IP address extraction
   - Cross-service correlation
   - Alert generation

3. Set up daemons/cron:
   - Create systemd service files or cron jobs
   - Schedule morphing events
   - Schedule quorum checks

## Log Flow

```
Services (Docker)
    ↓
Logs written to: services/*/logs/
    ↓
C Quorum Engine scans logs
    ↓
Detects patterns → Alerts
    ↓
Logs to: build/quorum-alerts.log
    ↓
Dashboard reads and displays
```

## Morphing Flow

```
C Morph Engine (scheduled)
    ↓
Reads current device profile
    ↓
Selects new profile (rotation)
    ↓
Modifies:
  - Cowrie banners (services/cowrie/etc/)
  - Router HTML (services/fake-router-web/html/)
  - Camera HTML (services/fake-camera-web/html/)
    ↓
Restarts/reloads services (via Docker API or signals)
    ↓
Logs morph event
```

## File Structure

```
cerberus-honeypot/
├── src/                    # C source code
├── include/                # C headers
├── build/                  # Compiled binaries
├── services/               # Service configs and data
│   ├── cowrie/
│   ├── fake-router-web/
│   ├── fake-camera-web/
│   └── rtsp/
├── docker/                 # Docker Compose
├── logs/                   # Aggregated logs
├── dashboard/              # Dashboard (optional)
├── docs/                   # Documentation
├── setup.sh                # Main setup script
├── requirements.txt         # Python dependencies
└── DEPENDENCIES.md         # Dependency documentation
```

## Security Considerations

⚠️ **IMPORTANT**: 
- Never expose honeypots on production networks
- Use isolated VMs, containers, or cloud instances
- Monitor all network traffic
- Review logs regularly
- Use firewall rules to limit exposure
- Consider using VPN or private networks only

## Testing Plan

1. **Unit Tests**: Test C modules individually
2. **Integration Tests**: Test service interactions
3. **Morphing Tests**: Verify device profile changes
4. **Quorum Tests**: Verify attack detection
5. **End-to-End Tests**: Full system simulation

## Next Implementation Steps

1. ✅ Create dependencies documentation
2. ✅ Create setup scripts
3. ✅ Create Docker Compose file
4. ⏳ Implement morphing engine logic
5. ⏳ Implement quorum detection logic
6. ⏳ Create dashboard (optional)
7. ⏳ Create systemd/cron configurations
8. ⏳ Write tests
9. ⏳ Create deployment documentation

## Notes

- All core logic in C for performance
- Services containerized for portability
- Logs centralized for analysis
- Morphing happens on schedule (configurable)
- Quorum checks run periodically (configurable)

