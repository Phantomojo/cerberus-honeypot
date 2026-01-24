# TITLE OF THE PROJECT: CERBERUS: A Bio-Adaptive IoT Honeynet for Moving Target Defense and Strategic Threat Intelligence

**AUTHORS:** Phantomojo, [Team Member 2], [Team Member 3], [Team Member 4]

**Legend:** Project Proposal Submitted to the Department of Computer Science for Partial Fulfillment for the Award of Bachelors of Science in Computer Science of Open University of Kenya

**Year:** January, 2026

---

## Page (ii): Declaration Page

### Students Declaration
We declare that this project proposal is our original work and has not been presented in any other university/institution for Academic.

**Signature:** ______________________________ **Date:** ________________________
*Phantomojo [Regno: OUK/CS/001/2025], Department of Computer Science*

**Signature:** ______________________________ **Date:** ________________________
*[Author 2 Name] [Regno], Department of Computer Science*

**Signature:** ______________________________ **Date:** ________________________
*[Author 3 Name] [Regno], Department of Computer Science*

**Signature:** ______________________________ **Date:** ________________________
*[Author 4 Name] [Regno], Department of Computer Science*

### Supervisors’ Approval
This project proposal has been submitted with our approval as University supervisors.

**Signature:** ______________________________ **Date:** ________________________
*First Supervisor, Department of Computer Science, Open University of Kenya*

**Signature:** ______________________________ **Date:** ________________________
*Second Supervisor, Department of Computer Science, Open University of Kenya*

---

## Page (iii): ABSTRACT
The rapid expansion of the Internet of Things (IoT) has introduced significant security challenges, as static defense mechanisms fail to keep pace with evolving malware. This project addresses the critical problem of "Honeypot Evasion," where malicious actors easily identify and bypass traditional deception systems using automated fingerprinting scripts. The purpose of this project is to develop CERBERUS, a distributed bio-adaptive honeynet that emulates IoT hardware while dynamically morphing its identity. The system-level objectives include implementing a 6-phase morphing engine, a quorum-based alert validator, and high-fidelity emulators for SSH, Telnet, and RTSP protocols. The technology stack comprises C for core performance, Python for behavioral scripts, Docker for containerized isolation, and Redis for quorum state. The methodology follows a Prototype SDLC model focusing on the "Rubik's Cube" architecture of synchronized deception layers. Intended beneficiaries include the National Intelligence Service (NIS) and academic researchers. The expected outcome is a resilient threat intelligence platform capable of reducing automated detection by 75% and providing deep insights into 0-day IoT exploits.

---

## Page (iv): TABLE OF CONTENTS
1.  ABSTRACT ............................................................ iii
2.  LIST OF FIGURES ..................................................... v
3.  ABBREVIATIONS AND ACRONYMS .......................................... vii
4.  OPERATIONAL DEFINITION OF TERMS ..................................... viii
5.  CHAPTER ONE: INTRODUCTION .......................................... 1
6.  CHAPTER TWO: LITERATURE REVIEW ..................................... 5
7.  CHAPTER THREE: METHODOLOGY ......................................... 8
8.  REFERENCES .......................................................... 12
9.  APPENDICES .......................................................... 14

---

## Page (v): LIST OF FIGURES
- Figure 1: Global IoT Attack Trends (2023-2025)
- Figure 2: The CERBERUS 6-Phase Morphing Architecture
- Figure 3: Quorum Engine Message Flow
- Figure 4: System Implementation Roadmap (Gantt Chart)

---

## Page (vii): ABBREVIATIONS AND ACRONYMS
- **API:** Application Programming Interface
- **ARP:** Address Resolution Protocol
- **CCTV:** Closed-Circuit Television
- **IDE:** Integrated Development Environment
- **IoT:** Internet of Things
- **MAC:** Media Access Control
- **NIS:** National Intelligence Service
- **OUK:** Open University of Kenya
- **RTSP:** Real-Time Streaming Protocol
- **SDLC:** System Development Life Cycle
- **TTL:** Time To Live

---

## Page (viii): OPERATIONAL DEFINITION OF TERMS
- **Bio-Adaptive:** A system’s ability to change its characteristics in response to environmental stimuli (attack patterns), mimicking biological defense.
- **Honeynet:** A network of honeypots designed to be probed, attacked, or compromised to gather intelligence.
- **Morphing:** The process of changing a system’s digital fingerprint (banners, MACs, OS versions) to avoid detection.
- **Quorum:** A consensus mechanism where multiple nodes must agree on a threat before an alert is triggered.

---

# CHAPTER ONE: INTRODUCTION

## 1.1 Introduction (Background of the Project)
Technology plays a transformative role in modern human life, enhancing organizational efficiency and service delivery through ubiquitous connectivity. **Globally**, the adoption of IoT has surpassed 15 billion devices, creating an interconnected ecosystem that powers smart grids and healthcare. **Regionally**, Africa is experiencing an IoT boom, with smart agricultural and logistics systems becoming essential to economic growth. **Nationally**, Kenya’s "Digital Masterplan 2022-2032" prioritizes the deployment of smart infrastructure across all counties. **Institutionally**, at the Open University of Kenya (OUK), there is a strategic mandate to develop localized cybersecurity solutions that protect this burgeoning infrastructure. However, this evolution has also empowered cyber-criminals who utilize botnets to orchestrate massive DDoS attacks, establishing the concept of honeynets as a vital defense mechanism in academic and intelligence institutions.

## 1.2 Statement of the Problem
The current state of IoT defense relies on static honeypots that present the same digital signature for years. These systems operate as simple lures with fixed banners and rigid directory structures. Malicious actors now use automated fingerprinting tools that can distinguish a honeypot from a real device in less than 50 milliseconds. The limitations of this approach include high blacklisting rates, poor engagement time with sophisticated attackers, and the inability to capture "zero-day" lateral movement patterns. There is a profound inefficiency in existing procedures that leads to a loss of high-fidelity logs, justifying the need for a system that can deceive attackers at a deeper behavioral level.

## 1.3 Purpose of the Project
The purpose of this project is to develop an adaptive, distributed honeynet system that employs moving-target defense to capture high-fidelity IoT threat intelligence while remaining undetectable to automated scanners.

### 1.3.1 Developer-Level Objectives
- **System Analysis:** Evaluate current IoT malware fingerprinting techniques.
- **System Design:** Create a modular 6-Phase architecture for front-end emulators and back-end logic.
- **Coding & Integration:** Develop core engines in C and integrate them with Docker-based service layers.
- **System Testing:** Perform rigorous debugging and evasion validation using Nmap and Shodan scripts.

### 1.3.2 System-Level Objectives
- **Morphing Module:** Reconfigure service banners, MAC addresses, and OS kernels on a rule-based schedule.
- **Quorum Module:** Correlate events across multiple nodes to eliminate false positives and coordinate cluster morphs.
- **Behavioral Simulation Module:** Inject realistic latencies and command failures based on device profile.
- **Temporal Management:** Generate system-wide aging artifacts, including uptime consistent logs and file timestamps.
- **Lure-Neighborhood Module:** Emulate high-value network neighbors (e.g., NAS, Admin PCs) via ARP and ICMP simulation to tempt lateral movement.
- **Deep Hardware Emulation:** Simulate low-level device nodes (/dev/mtd, /dev/gpio) and enforce architecture-specific binary execution failures to defeat advanced anti-honeypot checks.
- **Autonomous Identity Acquisition (AIA):** A self-healing harvesting engine that utilizes session-cookie hijacking to bypass API search restrictions, scrape live IPs from the Shodan web interface, and automatically ingest high-fidelity profiles.
- **Kernel Vulnerability Simulation:** Simulate vulnerable kernel states (e.g., LPE vectors) and provide realistic "exploit success" or "kernel oops" feedback to capture advanced attacker post-exploitation toolsets.

## 1.4 Project Justification (Rationale)
The proposed system is necessary because traditional defenses are losing the "arms race" against IoT malware. By addressing the identified problem of static signatures, it offers superior intelligence-gathering capabilities. It provides the National Intelligence Service (NIS) with a "digital trap" that feels real, thereby attracting more sophisticated threats and providing early warnings for national security.

## 1.5 Scope of the Project
- **Technological Scope:** C11 for core logic, Python 3.10 for command handlers, Docker for isolation, and Redis for quorum state tracking.
- **Application Scope:** To be deployed in isolated VPS environments and monitored by OUK and NIS security operation centers.
- **Functional Scope:** 4 major modules: Morphing Engine, Quorum Coordinator, Deep Interceptor, and the Intelligence Dashboard.

## 1.6 Beneficiaries
- **National Intelligence Service (NIS):** Will benefit from high-fidelity data on emerging IoT threats targeting Kenya.
- **Open University of Kenya (OUK):** Will benefit from a state-of-the-art research platform for cybersecurity fellows and students.

## 1.7 Functional Requirements
The system SHALL consist of the following four major modules:
1.  **Identity Rotation Module (IRM):** SHALL update SSH/Telnet/Web banners every 12 hours.
2.  **Kernel Interceptor (KI):** SHALL simulate custom outputs for `uname -a`, `/proc/version`, and `os-release`.
3.  **Quorum Voting Service (QVS):** SHALL require at least 2 nodes to report an IP before marking it as "threat-actor."
4.  **Behavioral Delay Engine (BDE):** SHALL inject 100ms-500ms jitter into shell responses for realism.

## 1.8 Non-Functional Requirements
1.  **Security:** The system SHALL use Docker-in-Docker isolation to prevent host compromise.
2.  **Connectivity:** The Quorum engine SHALL maintain heartbeat signals with nodes every 60 seconds.
3.  **Usability:** The system SHALL provide a Markdown-based report generator for daily attack summaries.

---

# CHAPTER TWO: LITERATURE REVIEW

## 2.1 Introduction
Existing literature highlights the "Uncanny Valley" in honeypot design, where minor inconsistencies (like a modern Ubuntu kernel on a 2012 D-Link router profile) lead to detection.

## 2.5 Current Technologies
Technologies supporting the system include **MediaMTX** for RTSP emulation, **Cowrie** for shell interaction, and **Nginx** with swappable themes. The development environment uses **GCC** for performance-critical engines.

## 2.6 Summary
The reviewed literature supports the project’s focus on **Moving Target Defense (MTD)**, confirming that frequent, synchronized rotations are the only way to remain invisible to modern botnets.

---

# CHAPTER THREE: METHODOLOGY

## 3.1 Introduction
The project employs a **Prototyping SDLC** model. This is justified by the complex, experimental nature of bio-adaptive morphing which requires frequent validation and adjustment of deception layers.

### Step-by-Step Development Procedures:
1.  **Scaffold Implementation:** Setting up the Dockerized node templates.
2.  **Logic Development:** Encoding the 6 morphing vectors in C.
3.  **Refinement:** Tailoring emulators (RTSP, SSH) to match specific IoT profiles.
4.  **Intelligence Integration:** Hooking into the **Shodan API** to fetch real-world vulnerability profiles and network metadata.
5.  **Validation:** Running automated scanners against the prototype.

### High-Level System Architecture
The system utilizes a "Rubik's Cube" model where the **Morphing Controller** acts as the central brain, rotating the **Network**, **Filesystem**, **Process**, **Behavior**, **Temporal**, and **Quorum** layers in unison.

### Data Structure Design
The system uses a `device_profile_t` struct to store all identity markers and a `history_db` (linked list/hash map) to track IP-to-Profile persistence.

### User Interface Design Principles
The administration UI follows a "Glassmorphic" design with clear, real-time data panels showing "Infection Propagation" and "Morphing Logs" for easy human monitoring.

### System Requirements
- **Hardware:** CPU 2.0GHz+, 2GB RAM (Minimum), 20GB SSD.
- **Software:** Ubuntu 22.04 LTS, Docker Engine v24.
- **Development Tools:** VS Code, GCC, Python 3.10, Git.

---

# REFERENCES
- Open University of Kenya (2025). *Strategic Deception Guide*. OUK Press.
- NIS (2024). *IoT Threat Landscape Report*. Government of Kenya.
- [APA 7th Edition style citations for additional research...]

---

# APPENDICES or ANNEXES

## ANNEX 1: Project Schedule
| Phase   | Month (2026) | Activities                            |
| ------- | ------------ | ------------------------------------- |
| Phase 1 | January      | System Analysis & Scaffold Setup      |
| Phase 2 | February     | Coding of Morph & Quorum Engines      |
| Phase 3 | March        | Integration & Security Stress Testing |
| Phase 4 | April        | Final Documentation & Presentation    |

## ANNEX 2: Budget
| Item        | Description           | Unit Cost | Qty | Total Cost      |
| ----------- | --------------------- | --------- | --- | --------------- |
| Laptop      | Developer Workstation | 85,000    | 1   | 85,000          |
| External HD | 2TB Backup Drive      | 8,500     | 1   | 8,500           |
| Internet    | Monthly Subscription  | 2,500     | 4   | 10,000          |
| VPS         | Hosting (4 months)    | 2,000     | 4   | 8,000           |
| Submission  | DVDs for Project      | 500       | 1   | 500             |
| **Total**   |                       |           |     | **112,000 KES** |

## ANNEX 3: Team Profile and Competencies
- **Phantomojo:** Lead Developer (C/Python Specialist, Network Security).
- **Member 2:** System Architect (Docker & DevOps).
- **Member 3:** Data Analyst (Log Parsing & Visualization).
- **Member 4:** Documentation & QA Lead.
