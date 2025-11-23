# CERBERUS: Bio-Adaptive IoT Honeynet — MVP Prototype

This project is a bio-adaptive IoT honeynet designed to emulate typical smart-home devices (Router + CCTV). It demonstrates dynamic device “morphing,” coordinated attack detection (“quorum sensing”), and a basic local dashboard, with all core adaptation/correlation code written in C.

---

## Project Structure

```
honeynet/
├── docs/              # Documentation, design diagrams
├── src/
│   ├── morph/         # Morphing engine (C)
│   ├── quorum/        # Quorum logic (C)
│   └── utils/         # Helper utilities (C)
├── include/           # Shared C headers
├── services/
│   ├── cowrie/        # External Cowrie honeypot (Python)
│   ├── fake-router-web/ # Static router UI (nginx)
│   ├── fake-camera-web/ # Static camera UI (nginx)
│   └── rtsp/             # RTSP fake server
├── dashboard/         # (Optional) Flask or C/CGI dashboard
├── logs/              # Captured log data (gitignored, .gitkeep)
├── build/             # C build outputs, binaries
├── docker/            # Docker Compose, configs
│   └── docker-compose.yml
├── Makefile
└── README.md
```

---

## Quick Start

1. **Clone the repo**

2. **Build C modules:**
   ```sh
   make
   ```

3. **Start services (requires Docker):**
   ```sh
   cd docker
   docker compose up -d
   ```

4. **Run morphing/quorum engines as daemons or cron jobs.**

5. **View logs:**  
   Service logs in `logs/`, morph/quorum events in `build/`.

---

## Main Components

- **Morphing Engine (`src/morph/`):**  
  Rotates device fingerprints (banners, HTML themes) according to a schedule.
- **Quorum Engine (`src/quorum/`):**  
  Correlates attack events across honeypot services, detecting coordinated probing/attacks.
- **External IoT Emulators:**  
  - Cowrie (SSH/Telnet honeypot)
  - Static nginx web UIs (router/camera)
  - RTSP server

---

## Core Design

- **All primary logic in C.**
- **Supporting scripts/configs (Docker, simple web) in standard formats.**
- **All logs extracted and correlated by C modules.**
- **Fully containerized, school-project friendly, and easy to test or extend!**

---

**For setup details, see `docs/`, and for architecture see the system diagram inside.**

---

Licensed for educational purposes.