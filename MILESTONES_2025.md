# CERBERUS Honeypot - 2025 Milestone Tracking

**Project Status:** In Development | **Last Updated:** Nov 27, 2025

---

## Executive Summary

CERBERUS is a multi-protocol, AI-adaptive honeypot system with morphing capabilities. This document tracks all milestones, sprints, and implementation progress toward a production-grade threat intelligence platform.

**Current Goal:** Deploy Cowrie + RIoTPot + adaptive behavioral system by Q1 2026

---

## Phase Overview (10 Phases)

| Phase | Name | Status | Start | End | Duration |
|-------|------|--------|-------|-----|----------|
| 1 | Advanced Cowrie Customization | 🔵 **In Progress** | Nov 27 | Dec 11 | 2 weeks |
| 2 | Device Profile Emulation | 🟡 **Queued** | Dec 12 | Dec 26 | 2 weeks |
| 3 | Behavioral Randomization | 🟡 **Queued** | Dec 27 | Jan 9 | 2 weeks |
| 4 | AI-Driven Detection | 🟡 **Queued** | Jan 10 | Feb 6 | 4 weeks |
| 5 | Detection Evasion | 🟡 **Queued** | Jan 10 | Jan 23 | 2 weeks |
| 6 | Multi-Stage Attack Detection | 🟡 **Queued** | Feb 7 | Feb 27 | 3 weeks |
| 7 | Threat Intelligence Integration | 🟡 **Queued** | Mar 1 | Mar 28 | 4 weeks |
| 8 | Cloud Deployment & Scaling | 🟡 **Future** | Apr 1 | May 31 | 8 weeks |
| 9 | Metrics & Monitoring | 🟡 **Queued** | Feb 1 | Feb 28 | 4 weeks |
| 10 | Security Hardening | 🔵 **In Progress** | Nov 1 | Dec 31 | 2 months |

**Legend:** 🟢 Completed | 🔵 In Progress | 🟡 Queued | 🔴 Blocked | 🟣 Not Started

---

## Detailed Sprint Schedule

### **SPRINT 1: Quick Wins (Nov 27 - Dec 11, 2025)**

**Goal:** Implement high-impact, low-effort improvements to evade detection tools.

| Task | Effort | Status | Owner | Details |
|------|--------|--------|-------|---------|
| Change Cowrie port 2222 → 22 | 15 min | 🟡 Queued | - | Edit docker-compose.yml + cowrie.cfg |
| Randomize SSH version strings | 30 min | 🟡 Queued | - | Implement src/morph/cowrie_versions.c |
| Extend session timeout (180s → 600s) | 10 min | 🟡 Queued | - | Update cowrie.cfg |
| Add 5 missing commands | 20 min | 🟡 Queued | - | docker, systemctl, python, git, curl |
| Create device-profile filesystems | 30 min | 🟡 Queued | - | honeyfs-router/ and honeyfs-camera/ |
| Add random response delays | 15 min | 🟡 Queued | - | Python usleep() in command handlers |
| Setup attack monitoring script | 30 min | 🟡 Queued | - | scripts/monitor-attacks.sh |
| **Total Time:** | **2 hours** | - | - | **Expected Impact:** 50% detection evasion |

**Deliverables:**
- [ ] docker-compose.yml updated with port 22
- [ ] SSH version randomization working
- [ ] 5 new commands responding
- [ ] Device-profile file swap on morphing
- [ ] Monitoring script deployed
- [ ] Before/after cowrie_detect.py test results

**Success Criteria:**
- Cowrie no longer detected on port 2222
- SSH version changes per restart
- Session lasts > 10 minutes
- docker ps shows all containers running

---

### **SPRINT 2: Behavioral Randomization (Dec 12 - Dec 26, 2025)**

**Goal:** Make honeypot behavior less predictable and more realistic.

| Task | Effort | Status | Dependencies |
|------|--------|--------|--------------|
| Response time variability (SSH) | 2 hours | 🟡 Queued | Sprint 1 complete |
| Response time variability (HTTP) | 2 hours | 🟡 Queued | Sprint 1 complete |
| Realistic failure modes | 3 hours | 🟡 Queued | Sprint 1 complete |
| Time-based behavior (cron simulation) | 2 hours | 🟡 Queued | Sprint 1 complete |
| Device profile-specific latency | 1 hour | 🟡 Queued | Sprint 1 complete |
| **Total:** | **10 hours** | - | - |

**Deliverables:**
- [ ] Commands return 100-500ms delays
- [ ] Occasional "permission denied" errors
- [ ] Rate limiting after N failed logins
- [ ] Simulated cron jobs in logs
- [ ] Router/Camera profiles have different response times

---

### **SPRINT 3: IoT Protocol Support (Jan 1 - Jan 15, 2026)**

**Goal:** Add RIoTPot for MQTT, Modbus, CoAP coverage.

| Task | Effort | Status | Details |
|------|--------|--------|---------|
| Deploy RIoTPot container | 2 hours | 🟡 Queued | Add to docker-compose.yml |
| Integrate MQTT emulation | 3 hours | 🟡 Queued | Test with MQTT scanners |
| Integrate Modbus emulation | 3 hours | 🟡 Queued | Test with Modbus clients |
| Integrate CoAP emulation | 2 hours | 🟡 Queued | Test with CoAP tools |
| Update morphing engine | 3 hours | 🟡 Queued | Apply device profiles to RIoTPot |
| Testing & validation | 2 hours | 🟡 Queued | Multi-protocol attack simulation |
| **Total:** | **15 hours** | - | **Week 1 of Jan 2026** |

**Deliverables:**
- [ ] RIoTPot running in docker-compose
- [ ] MQTT broker responding to connections
- [ ] Modbus TCP listening
- [ ] CoAP UDP responding
- [ ] Logs capturing attacks on all protocols

---

### **SPRINT 4: Advanced Cowrie Customization (Dec 27 - Jan 9, 2026)**

**Goal:** Eliminate known Cowrie detection signatures.

| Task | Effort | Status | Details |
|------|--------|--------|---------|
| Randomize SSH algorithms | 2 hours | 🟡 Queued | Change key exchange, encryption, MAC |
| Create profile-specific command outputs | 4 hours | 🟡 Queued | arch, uname, lsb_release per device |
| Add fake security tool outputs | 2 hours | 🟡 Queued | fail2ban, snort, iptables fake output |
| Implement decoys within honeypot | 3 hours | 🟡 Queued | Fake IDS alerts, fake firewall rules |
| Test against cowrie_detect.py | 2 hours | 🟡 Queued | Measure evasion improvement |
| **Total:** | **13 hours** | - | **Week 2 of Jan 2026** |

**Deliverables:**
- [ ] SSH algorithms rotated per connection
- [ ] Device profile outputs differ significantly
- [ ] Fake security tool responses logged
- [ ] Detection tool shows LOW confidence

---

### **SPRINT 5: Threat Intelligence Integration (Jan 16 - Feb 6, 2026)**

**Goal:** Connect to threat feeds and submit intelligence.

| Task | Effort | Status | Details |
|------|--------|--------|---------|
| Add threat feed API client | 4 hours | 🟡 Queued | VirusTotal, abuse.ch, AlienVault OTX |
| Implement IOC generation | 3 hours | 🟡 Queued | IPs, hashes, domains in STIX 2.1 |
| Auto-submit malware samples | 2 hours | 🟡 Queued | VirusTotal API integration |
| Create threat dashboard | 4 hours | 🟡 Queued | Grafana integration |
| **Total:** | **13 hours** | - | **Late Jan 2026** |

**Deliverables:**
- [ ] Captured IPs checked against threat feeds
- [ ] Malware hashes submitted to VirusTotal
- [ ] IOCs exported as JSON/CSV
- [ ] Grafana dashboard showing threat status

---

## Current Implementation Status

### ✅ Completed (Foundation)
- [x] Cowrie SSH/Telnet honeypot deployed
- [x] Docker compose multi-container setup
- [x] Device profile system (router/camera)
- [x] Morphing engine (basic)
- [x] Quorum detection (IP correlation)
- [x] Systemd automation (timers)
- [x] Compiler optimizations (-O2, -march=native)
- [x] Security hardening (systemd, resource limits)

### 🔵 In Progress
- [ ] Port configuration change (2222 → 22)
- [ ] SSH version randomization
- [ ] Missing command implementations
- [ ] Device profile filesystem swap

### 🟡 Queued / Not Started
- [ ] Behavioral randomization
- [ ] RIoTPot integration
- [ ] AI/ML behavioral analysis
- [ ] Threat intelligence feeds
- [ ] Cloud scaling
- [ ] Advanced detection evasion

---

## Technical Debt & Blockers

| Issue | Severity | Status | Notes |
|-------|----------|--------|-------|
| Cowrie easily detectable on port 2222 | 🔴 Critical | 🟡 In Sprint 1 | Immediate quick win |
| No multi-protocol IoT coverage | 🔴 Critical | 🟡 In Sprint 3 | RIoTPot solves this |
| Static behavior patterns | 🟡 High | 🟡 In Sprint 2 | Randomization needed |
| No malware collection | 🟡 High | 🟡 In Sprint 3 | RIoTPot + Dionaea |
| Limited threat intelligence | 🟡 Medium | 🟡 In Sprint 5 | Feed integration pending |

---

## Resource Requirements

### Hardware
- **Current:** 1x Linux host (Docker)
- **Phase 8 (Cloud):** 3-4x regional VPS instances

### Software
- **Core:** Cowrie, RIoTPot, T-Pot (optional), Dionaea (optional)
- **Analysis:** Grafana, Elasticsearch, Kibana (optional)
- **CI/CD:** Make, systemd, Docker

### Team
- **Current:** 1x maintainer
- **Phase 8+:** 1x security engineer + 1x analyst

---

## Risk Assessment

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| RIoTPot fingerprinting | Medium | Medium | AI adaptation, multiple variants |
| High RAM usage (T-Pot) | Low | Low | Deploy as optional profile |
| Cloud cost overrun | Medium | Medium | Budget-conscious instance sizing |
| Attacker breakout | Low | High | Network segmentation, monitoring |
| Detection tool evolution | High | Medium | Continuous morphing updates |

---

## Success Metrics

### By Phase 1 (Quick Wins)
- [ ] cowrie_detect.py reports LOW confidence
- [ ] SSH connections > 5 minutes average
- [ ] 5+ new commands working
- [ ] 50% reduction in immediate detection

### By Phase 3 (IoT)
- [ ] MQTT, Modbus, CoAP traffic logged
- [ ] Multi-protocol attacks captured
- [ ] 10+ IoT-specific attack patterns identified

### By Phase 6 (AI Detection)
- [ ] Attack stages automatically classified
- [ ] Attacker sophistication scoring working
- [ ] 90%+ accuracy on multi-stage detection

### By Phase 9 (Full System)
- [ ] 99.9% uptime
- [ ] <3% false positive rate
- [ ] 47%+ faster incident response
- [ ] Monthly threat intelligence reports

---

## Timeline Summary

```
Nov 27, 2025 ─ Quick Wins (2 weeks)
              │
Dec 12, 2025 ─ Behavioral Randomization (2 weeks)
              │
Dec 27, 2025 ─ Advanced Cowrie (2 weeks)
              │
Jan 16, 2026 ─ IoT Protocols (2 weeks)
              │
Feb 1, 2026  ─ Threat Intelligence (4 weeks)
              │
Mar 1, 2026  ─ AI Detection (4 weeks)
              │
Apr 1, 2026  ─ Cloud Scaling (8 weeks)
              │
Jun 1, 2026  ─ PRODUCTION READY ✅
```

**Estimated Total Effort:** 120-150 hours over 6 months

---

## Review & Updates

- **Weekly:** Update sprint task status
- **Bi-weekly:** Review blockers & risks
- **Monthly:** Assess phase completion & next priorities
- **Quarterly:** Strategic planning for next quarter

**Last Review:** Nov 27, 2025
**Next Review:** Dec 4, 2025

---

## Reference Documents

- **QUICK_WINS.md** - Sprint 1 detailed implementation
- **IMPROVEMENT_ROADMAP.md** - Full 10-phase architecture
- **RESEARCH_SUMMARY_2025.md** - Industry research & trends
- **COWRIE_DETECTION_EVASION.md** - Detection techniques
- **INTEGRATION_CHECKLIST.md** - Component integration checklist
