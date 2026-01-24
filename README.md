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

1. **Clone the repo:**
   ```sh
   git clone https://github.com/Phantomojo/cerberus-honeypot.git
   cd cerberus-honeypot
   ```

2. **Run setup script:**
   ```sh
   ./setup.sh
   ```
   This will:
   - Check/install dependencies (use `./setup.sh --install-deps` to auto-install)
   - Set up Python virtual environment
   - Configure service directories
   - Build C modules

3. **Configure services:**
   ```sh
   ./services/setup-services.sh
   ```
   This creates initial configurations for all honeypot services.

4. **Start services (requires Docker):**
   ```sh
   cd docker
   docker compose up -d
   ```

5. **Run morphing/quorum engines:**
   ```sh
   ./build/morph    # Test morphing engine
   ./build/quorum   # Test quorum engine
   ```
   (Later: set up as daemons or cron jobs)

6. **View logs:**  
   Service logs in `logs/` and `services/*/logs/`, morph/quorum events in `build/`.

7. **Run tests:**
   ```sh
   make test    # Run automated test suite
   ```
   See `TESTING.md` for comprehensive testing guide.

**For detailed setup instructions, see:**
- `DEPENDENCIES.md` - System requirements and dependencies
- `SETUP_PLAN.md` - Architecture and implementation plan
- `TESTING.md` - Comprehensive testing guide

---

## Main Components

- **Morphing Engine (`src/morph/`):**  
  Rotates device fingerprints (banners, HTML themes) according to a schedule.
- **Quorum Engine (`src/quorum/`):**  
  Correlates attack events across honeypot services, detecting coordinated probing/attacks.
- **External IoT Emulators:**  
  - Cowrie (SSH/Telnet honeypot with **Deep Hardware Emulation**)
  - Static nginx web UIs (router/camera)
  - RTSP server

- **Autonomous Intelligence:**
  - **AIA Engine:** Self-hunting identity acquisition via Shodan web-scraping.
  - **Precision Cloner:** 1:1 metadata and CVE mirroring for high-fidelity lures.

---

## Core Design

- **All primary logic in C.**
- **Supporting scripts/configs (Docker, simple web) in standard formats.**
- **All logs extracted and correlated by C modules.**
- **Fully containerized, school-project friendly, and easy to test or extend!**

---

## 📚 Documentation
- **[INSTALL.md](./INSTALL.md)**: The Golden Path to deployment.
- **[TECHNICAL_SPEC.md](./TECHNICAL_SPEC.md)**: Deep dive into the morphing architecture.
- **[docs/context/](./docs/context/)**: Presentation slides and briefing documents.
- **[docs/archive/](./docs/archive/)**: Historical research and sprint logs.

---

Licensed for educational purposes.
