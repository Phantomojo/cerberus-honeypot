# 🧠 CERBERUS Technical Specification

## Core Architecture: The "Rubik's Cube" Metaphor
CERBERUS is built on a 6-Phase Morphing Architecture. When the system "morphs," all six layers rotate simultaneously to maintain a consistent device identity.

| Phase | Layer          | Mechanism                                                           |
| ----- | -------------- | ------------------------------------------------------------------- |
| **1** | **Network**    | Generates `ifconfig`/`route` AND masks packet TTL/TCP Window.       |
| **2** | **Filesystem** | Swaps profiles AND masks nodes like `/dev/mtd`, `/proc/mounts`.     |
| **3** | **Processes**  | Simulates `ps`/`top` AND enforces **Architecture-Guard** execution. |
| **4** | **Behavior**   | Injects realistic command delays and failure modes.                 |
| **5** | **Temporal**   | Simulates system aging (uptime, log accumulation).                  |
| **6** | **Quorum**     | Implements **IP Pinning** to prevent return-visit discovery.        |

## Deception Layers
- **SSH/Telnet:** Powered by a customized Cowrie engine with dynamic banners.
- **Web UI:** Static Nginx themes that switch between Router (TP-Link/D-Link) and Camera (Hikvision/Dahua).
- **RTSP:** Simulated stream metadata for IoT camera profiles.

## Advanced Engines
- **Morphing Engine (`src/morph/`):** A C-based engine that regenerates configurations every 6-24 hours.
- **Quorum Engine (`src/quorum/`):** Monitors logs across all services to detect coordinated reconnaissance.
- **HoneyGPT (Opt):** LLM-based behavioral response for high-fidelity interaction.
- **Shodan Intelligence:** Automated profile generation using live data from the Shodan API.
- **LPE Simulator:** Intercepts Local Privilege Escalation attempts (e.g., DirtyCow) to capture offensive payloads while keeping the host secure.

---
> [!IMPORTANT]
> **Anti-Fingerprinting:** CERBERUS is designed to defeat tools like `cowrie_detect.py` by removing all default signatures and matching kernel/hardware responses to the active profile.
