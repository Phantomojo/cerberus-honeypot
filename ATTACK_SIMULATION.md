# 🧪 CERBERUS Attack Simulation Guide

To see how CERBERUS reacts, we will perform a simulated "Red Team" exercise from your local machine against the cloud instance.

---

## Phase 1: Reconnaissance (External Scan)
Attackers usually start by scanning for open ports.

**The Attack:**
```bash
# Run from your local machine
nmap -p 2222 cerberus-hunting.duckdns.org
```

**The Reaction (HUD):**
- You should see an **"Inbound Connection"** alert in the Real-Time Telemetry.
- A marker should appear on the map at your local IP's location.

---

## Phase 2: Brute Force Entry
Attackers try common credentials.

**The Attack:**
```bash
# Try to log in with common (wrong) credentials
ssh admin@cerberus-hunting.duckdns.org -p 2222
# Type 'password123' when prompted
```

**The Reaction (HUD):**
- The Intelligence Terminal will log the **Failed Password** attempt.
- The "Global Threat Flux" metric should tick up slightly.

---

## Phase 3: Successful Infiltration
Cerberus is designed to let them in to observe their behavior.

**The Attack:**
```bash
# Log in (Cowrie usually accepts root/root by default)
ssh root@cerberus-hunting.duckdns.org -p 2222
# Password: root
```

**The Reaction (HUD):**
- **Live Terminal Feed**: You will see exactly what the "attacker" types in the Real-Time Telemetry window on the dashboard.
- **AI Deception**: If you type `cat /etc/issue`, notice the custom "Secure Network Terminal" banner we configured.

---

## Phase 4: Probing the Environment
Attackers check the system info for vulnerabilities.

**The Attack (Inside the Honeypot SSH):**
```bash
uname -a
ls -la /etc
ps aux
```

**The Reaction (HUD):**
- CERBERUS will serve "realistic but fake" data for each command.
- The "Target Deep-Intel" card on the dashboard will start populating with "Attacker Profile" data.

---

## 🚀 Pro Tip: The "Attack-in-a-Box" Script
If you want to demo this for your team, I can write a script called `demo_attack.sh` that automates these steps so you can just run it and watch the HUD light up.
