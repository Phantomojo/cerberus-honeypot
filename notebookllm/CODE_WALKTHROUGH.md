# CERBERUS Code Walkthrough & Explanation

Complete guide to understanding the CERBERUS codebase, with focus on how each piece works.

---

## 🗺️ Code Navigation Map

### Entry Points

**Morphing Engine:**
```
./build/morph [config_file] [state_file]
  └─ Entry: src/morph/morph.c::main()
  └─ Loads: profiles.conf (6 device profiles)
  └─ Rotates: To next profile in sequence
  └─ Runs: 6-phase morphing cycle
  └─ Outputs: All generated configs, commands, state
  └─ Saves: Current profile index to build/morph-state.txt
```

**Quorum Engine:**
```
./build/quorum [config_file]
  └─ Entry: src/quorum/quorum.c::main()
  └─ Scans: All service log files
  └─ Analyzes: Attacker IP patterns
  └─ Detects: Coordinated attacks (2+ services)
  └─ Outputs: build/quorum-alerts.log
  └─ Signals: Emergency morph if needed
```

---

## 📝 Main Morphing Engine: morph.c

### Function: load_profiles()
```
Purpose: Load 6 device profiles from config file
Location: src/morph/morph.c::load_profiles()

What it does:
  1. Opens profiles.conf
  2. Parses INI-style sections: [ProfileName]
  3. Reads key=value pairs for each profile
  4. Creates device_profile_t structures
  5. Populates into global profiles[] array

Key variables:
  - profiles[MAX_PROFILES]    # Array of 6 devices
  - profile_count             # Number loaded
  - current_profile_index     # Currently active device

Example parsing:
  [TP-Link_Archer_C7]
    ssh_banner=SSH-2.0-dropbear_2017.75
    kernel_version=3.10.49
    arch=mips
    memory_mb=128
    cpu_mhz=720

Output: Populates device_profile_t structure with all values
```

### Function: morph_device()
```
Purpose: Rotate to next device and apply all 6 phases
Location: src/morph/morph.c::morph_device()

What it does:
  1. Calculate next profile index: (current + 1) % 6
  2. Get profile: profiles[next_index]
  3. Apply core morphing:
     - Update SSH banners in Cowrie config
     - Update router HTML theme
     - Update camera HTML theme
  4. Setup device filesystem
  5. Run 6-phase morphing cycle
  6. Generate session variations
  7. Save state: current_profile_index to morph-state.txt
  8. Log event to morph-events.log

Flow:
  load_profiles()
    → morph_device()
      → morph_cowrie_banners()
      → morph_router_html()
      → morph_camera_html()
      → morph_phase1_network()
      → morph_phase2_filesystem()
      → morph_phase3_processes()
      → morph_phase4_behavior()
      → morph_phase5_temporal()
      → morph_phase6_quorum()
      → generate_session_variations()
      → save_current_profile()

Variables:
  - current_profile_index     # Which device we're on
  - new_profile               # Next device to become
  - result                    # Error checking

Output: All files updated, state saved, event logged
```

### Function: morph_phase1_network()
```
Purpose: Generate realistic network configuration
Location: src/morph/morph.c::morph_phase1_network()

What it generates:
  1. build/cowrie-dynamic/sbin/ifconfig
     └─ Network interface output (eth0, eth1, etc.)
     └─ IP addresses based on device type
     └─ MAC addresses
     └─ MTU sizes
     └─ Packet counters

  2. build/cowrie-dynamic/bin/route
     └─ Routing table
     └─ Gateway information
     └─ Network masks

  3. build/cowrie-dynamic/sbin/arp
     └─ ARP cache entries
     └─ Neighboring device IPs
     └─ MAC-to-IP mappings

  4. build/cowrie-dynamic/bin/netstat
     └─ Network statistics
     └─ Connection information
     └─ Port information

Example ifconfig output:
```
eth0: flags=<UP,BROADCAST,RUNNING>  mtu 1500
    inet 192.168.1.1
    ether 14:cc:20:AB:CD:EF
    RX packets:12345 errors:0 dropped:0
    TX packets:54321 errors:0 dropped:0
```

Why this matters:
  - Real devices have specific network layouts
  - Attackers can fingerprint by checking ifconfig
  - CERBERUS generates realistic, device-specific network config
  - Changes every morph cycle
  - MAC address includes vendor prefix (real!)

Process:
  1. Get device profile
  2. Determine IP addresses for device type
  3. Create realistic interface names
  4. Generate realistic packet counters
  5. Create routing table matching device
  6. Generate ARP cache with plausible neighbors
  7. Write all outputs to files
  8. Cowrie mounts these as read-only
  9. When attacker runs command, sees these outputs
```

### Function: morph_phase2_filesystem()
```
Purpose: Generate device-specific filesystem
Location: src/morph/morph.c::morph_phase2_filesystem()

What it generates:
  1. services/cowrie/honeyfs/etc/
     ├─ passwd              # Device-specific users
     ├─ hostname            # Device name
     ├─ resolv.conf         # DNS config
     ├─ shadow              # Password file
     └─ config files        # Device-specific configs

  2. services/cowrie/honeyfs/var/
     ├─ log/syslog          # System logs (aged naturally)
     ├─ log/auth.log        # Authentication logs
     └─ lib/dhcp/           # DHCP information

  3. services/cowrie/honeyfs/proc/
     ├─ uptime              # System uptime
     ├─ meminfo             # Memory information
     └─ cpuinfo             # CPU information

Example /etc/passwd for router:
```
root:x:0:0:root:/root:/bin/bash
nobody:x:65534:65534:nobody:/nonexistent:/usr/sbin/nologin
www-data:x:33:33:www-data:/var/www:/usr/sbin/nologin
```

Example /etc/passwd for camera:
```
root:x:0:0:root:/root:/bin/sh
admin:x:1000:1000:admin:/home/admin:/bin/sh
nobody:x:65534:65534:nobody:/nonexistent:/bin/sh
```

Why device-specific:
  - Routers have www-data user (web server)
  - Cameras have admin user
  - Device-specific groups and UIDs
  - Users reveal device type

Process:
  1. Get device profile type (router/camera)
  2. Read device-specific user list
  3. Write /etc/passwd with device users
  4. Set file ownership and permissions
  5. Generate log files that look aged
  6. Create realistic device config files
  7. Set timestamps to look natural

When attacker runs:
  $ cat /etc/passwd
  → They see device-specific users
  → If they see "admin" user, might be camera
  → If they see "www-data" user, likely router
```

### Function: morph_phase3_processes()
```
Purpose: Generate device-specific process list
Location: src/morph/morph.c::morph_phase3_processes()

What it generates:
  1. build/cowrie-dynamic/bin/ps
     └─ Process listing output
     └─ PIDs, TTY, command names

  2. build/cowrie-dynamic/bin/ps aux
     └─ Extended process info
     └─ User, CPU%, memory, command line

  3. build/cowrie-dynamic/bin/top
     └─ Process statistics
     └─ CPU and memory usage

Router processes:
```
  - Init (PID 1)
  - Ubusd (system bus daemon)
  - Hostapd (WiFi access point)
  - Dnsmasq (DNS/DHCP server)
  - Dropbear (SSH server)
  - Ntpd (time synchronization)
  - Syslogd (system logging)
```

Camera processes:
```
  - Init (PID 1)
  - Syslogd (system logging)
  - Httpd (HTTP server for web UI)
  - Rtspd (RTSP streaming server)
  - Dropbear (SSH server)
  - Iniserver (camera config daemon)
  - V4l2rtspserver (video streaming)
```

Why this matters:
  - Attackers run "ps" to see what's running
  - Router process list is completely different from camera
  - Wrong processes = "This is not a real device!"
  - We generate device-specific process lists

Process:
  1. Determine device type from profile
  2. Create base processes (init, syslogd, etc.)
  3. Add device-specific services
  4. Randomize PIDs (different each morph)
  5. Allocate memory to processes
  6. Create realistic command lines
  7. Write to ps, ps aux, top outputs

Example "ps aux" for router:
```
USER       PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
root         1  0.1  2.3  19264  3892 ?        Ss   12:34   0:01 /sbin/init
root       128  0.0  1.1  10264  1856 ?        Ss   12:35   0:00 /usr/sbin/ubusd
root       145  0.3  3.4  25600  5632 ?        S    12:35   0:02 /usr/sbin/hostapd
root       256  0.1  2.1  18432  3456 ?        Ss   12:35   0:00 /usr/sbin/dnsmasq
root       287  0.0  1.8  15872  2976 ?        Ss   12:35   0:00 /usr/sbin/dropbear
```
```

### Function: morph_phase4_behavior()
```
Purpose: Configure behavioral delays and error handling
Location: src/morph/morph.c::morph_phase4_behavior()

What it configures:
  1. Command execution delays
     └─ 50-500ms based on device type
     └─ Slower devices get longer delays
     └─ Real devices are slow!

  2. Error messages
     └─ Device-specific error formats
     └─ Realistic permission denied messages
     └─ Device-specific error codes

  3. Session jitter
     └─ Random small delays in responses
     └─ Makes timing analysis harder
     └─ Looks like real network latency

  4. Timeout values
     └─ Session timeouts (10 minutes)
     └─ Command timeouts
     └─ Connection timeouts

Why this matters:
  Honeypots respond instantly. Real devices are slow.
  When attacker runs a command:
    - Honeypot: Response in 10ms (too fast!)
    - CERBERUS: Response in 150-300ms (realistic!)

Timing detection:
  Attackers can measure response times.
  Instant responses scream "honeypot!"
  CERBERUS adds realistic delays to everything.

Process:
  1. Get device CPU specs from profile
  2. Calculate base delay based on CPU speed
  3. Add random jitter (±20%)
  4. Write delays to config files
  5. Cowrie reads and applies these delays
  6. Every command response delayed appropriately
```

### Function: morph_phase5_temporal()
```
Purpose: Simulate device aging over time
Location: src/morph/morph.c::morph_phase5_temporal()

What it generates:
  1. Boot time (0-365 days ago)
     └─ Random, but consistent across session
     └─ Makes device appear aged

  2. Uptime
     └─ Time since boot
     └─ Shows on "uptime" command
     └─ Appears to increment naturally

  3. System logs
     └─ /var/log/syslog accumulates
     └─ Log entries span boot time
     └─ Looks like real activity history

  4. File timestamps
     └─ Files have realistic ages
     └─ Creation times before boot time
     └─ Access times recent

Why this matters:
  If "uptime" always shows "5 minutes", it's fake!
  Real devices have:
    - Old boot times
    - Accumulated logs
    - File history
  CERBERUS simulates this naturally.
  Between morphs, device appears to age (time passes).

Example timeline:
```
First morph (Monday):
  Boot time: 47 days ago
  Uptime: 47 days 3 hours
  Syslog: Has entries from 47 days of operation

Next morph (Tuesday, 24 hours later):
  Boot time: 48 days ago (aged by 24 hours!)
  Uptime: 48 days 3 hours (incremented!)
  Syslog: Added entries from today
  Device appears to have aged naturally!
```

Process:
  1. Generate random boot time (0-365 days)
  2. Calculate uptime from boot to now
  3. Write to /proc/uptime
  4. Generate syslog entries spanning boot time
  5. Set file timestamps to match timeline
  6. Create consistent historical context
  7. Next morph: time advances, device ages
```

### Function: morph_phase6_quorum()
```
Purpose: Integrate Quorum attack detection
Location: src/morph/morph.c::morph_phase6_quorum()

What it does:
  1. Signals quorum engine to monitor logs
  2. Enables coordinated attack detection
  3. Prepares for emergency morphing
  4. Sets up attack response signals

Why this phase:
  Phase 6 enables intelligent defense.
  When Quorum detects coordinated attack:
    → Generates alert
    → Emits emergency morph signal
    → Watchdog triggers immediate morph
    → Device changes within minutes
  Breaks exploit chains mid-attack.

Integration:
  Morph provides clean state.
  Quorum monitors for attacks.
  If serious attack detected:
    → Don't wait for scheduled morph
    → Change device IMMEDIATELY
    → Attacker's context is lost
  Then normal morphing resumes.
```

---

## 🔍 Quorum Detection Engine: quorum.c

### Function: scan_logs_for_ips()
```
Purpose: Parse all service logs to extract attacker IPs
Location: src/quorum/quorum.c::scan_logs_for_ips()

What it does:
  1. Scans service log files:
     ├─ services/cowrie/logs/cowrie.log       (SSH attempts)
     ├─ services/fake-router-web/logs/access.log  (HTTP requests)
     ├─ services/fake-camera-web/logs/access.log  (HTTP requests)
     └─ services/rtsp/logs/*.log              (RTSP probes)

  2. Extracts IP addresses from each line
  3. Adds to IP tracking array
  4. Associates IP with service

What happens:
  For each log line:
    1. extract_ip_from_line()
       └─ Finds pattern: digits.digits.digits.digits
       └─ Validates it's a real IP (0-255 range)
       └─ Returns IP address

    2. add_ip_to_tracking()
       └─ Checks if IP already tracked
       └─ If new: creates entry
       └─ Adds service to IP's service list
       └─ Increments hit count
       └─ Updates timestamp

Data structure after scanning:
```
ip_tracking[0]:
  ip: "203.0.113.42"
  services: ["cowrie", "router-web", "camera-web"]
  service_count: 3
  hit_count: 15
  first_seen: timestamp
  last_seen: timestamp
```

Process:
  1. Determine which logs to scan
  2. Open each log file
  3. Read line by line
  4. Extract IP from each line
  5. Track which service the line came from
  6. Group all activity by IP
  7. Generate summary of IPs and services

Output:
  - ip_tracking array (filled with data)
  - ip_count (number of unique IPs)
  - Each IP has list of services it hit
```

### Function: detect_coordinated_attacks()
```
Purpose: Find IPs that hit multiple services
Location: src/quorum/quorum.c::detect_coordinated_attacks()

What it does:
  1. Looks through all tracked IPs
  2. Counts services each IP hit
  3. If IP hit 2+ services → COORDINATED ATTACK
  4. Generates alert for each suspicious IP
  5. Emits emergency morph signal

Why 2+ services means attack:
  Single service probe:
    - Could be casual scanning
    - Could be service discovery
    - Not necessarily targeted

  Multiple services:
    - Attacker is doing reconnaissance
    - Probing all entry points
    - Serious threat
    - Plan to exploit multiple services

Example detection:
```
IP 203.0.113.42:
  ├─ Cowrie SSH: 8 events (login attempts)
  ├─ Router Web: 6 events (HTTP requests to /admin)
  └─ RTSP: 3 events (RTSP OPTIONS)

  Services hit: 3
  → COORDINATED ATTACK!

  Actions:
    1. Generate alert
    2. Log details: IP, services, timestamps
    3. Emit emergency morph signal
```

Process:
  1. For each IP in tracking:
     ```
     if (ip->service_count >= 2):
         This is a coordinated attack
         generate_alert(ip)
         emit_morph_signal(ip)
         alert_count++
     ```
  2. Log all alerts to build/quorum-alerts.log
  3. Emit signals to trigger immediate morph

Output:
  - Alerts in build/quorum-alerts.log
  - Emergency morph signal if needed
```

### Function: detect_lateral_movement()
```
Purpose: Find attempts at internal network probing
Location: src/quorum/quorum.c::detect_lateral_movement()

What it does:
  1. Looks for commands indicating reconnaissance
     └─ nmap, ping, arp, route, netstat, ssh, telnet

  2. Checks for probing of internal IP ranges
     └─ 172.17.x.x (Docker networks)
     └─ 192.168.x.x (Common private)
     └─ 10.0.x.x (Common private)

  3. Detects high activity on single service
     └─ Port sweeping behavior
     └─ Enumeration attacks

Why this matters:
  Lateral movement = attacker tries to jump to other systems.
  If attacker is probing internal networks:
    → They're doing reconnaissance
    → Planning multi-stage attack
    → Serious threat

Detection:
  ```
  For each IP:
    if (hit_count > 50 AND service_count == 1):
        This looks like port sweeping
        emit_alert("Potential enumeration")
  ```

This detects:
  - Port sweeps (single service, many hits)
  - Network discovery attempts
  - Internal network probing
```

### Function: emit_morph_signal()
```
Purpose: Create emergency morph signal file
Location: src/quorum/quorum.c::emit_morph_signal()

What it does:
  1. Creates signal directory: build/signals/
  2. Writes signal file: emergency_morph.signal
  3. Includes attack context:
     ```
     type=coordinated_attack
     attacker_ip=203.0.113.42
     services_hit=3
     timestamp=1705866900
     ```
  4. Watchdog monitors this directory
  5. On detection, triggers: ./build/morph
  6. Device changes IMMEDIATELY

Signal file format:
```
build/signals/emergency_morph.signal

Content:
  type=coordinated_attack
  attacker_ip=203.0.113.42
  services_hit=3
  timestamp=1705866900

Presence of this file = "morph NOW!"
```

Why emergency morph:
  When serious attack detected:
    1. Device morphs immediately
    2. All command outputs change
    3. Attacker's reconnaissance becomes useless
    4. If they had exploit for "Hikvision camera"
       → Now facing "TP-Link router"
       → Exploit fails completely
    5. Attack chain broken

Process:
  1. Quorum detects coordinated attack
  2. Calls: emit_morph_signal(attacker_ip, services)
  3. Creates: build/signals/ directory
  4. Writes signal file with attack details
  5. Watchdog sees signal
  6. Watchdog runs: ./build/morph
  7. Device changes
  8. Attacker sees different device
  9. Exploit fails
```

### Function: generate_alert()
```
Purpose: Create detailed alert report
Location: src/quorum/quorum.c::generate_alert()

What it generates:
  ```
  ALERT: Coordinated attack detected
    IP: 203.0.113.42
    Services hit: cowrie, router-web, camera-web (3 services)
    Total hits: 15 events
    First seen: 2025-01-20 14:22:15
    Last seen: 2025-01-20 15:47:33
  ---
  ```

Output location:
  build/quorum-alerts.log

What alerts include:
  1. Attack classification
  2. Attacker IP address
  3. All services hit
  4. Number of events
  5. Timing information
  6. Timeline of activity

Process:
  1. Format alert message
  2. Add IP and service details
  3. Calculate timestamps
  4. Write to quorum-alerts.log
  5. Also print to stdout
  6. Log for forensics

Forensics value:
  Analysts can review alerts to understand:
    - Which IPs were attacking
    - What they were probing
    - When they attacked
    - How long they persisted
    - Whether attack succeeded
```

---

## 🔄 Main Program Flow

### Morphing Engine Execution
```
./build/morph [config_file] [state_file]
    │
    ├─ main()
    │  └─ Seed random number generator (important!)
    │  └─ Call: init_morph_engine(config_file, state_file)
    │  │
    │  ├─ init_morph_engine()
    │  │  ├─ Create directories (build/, services/cowrie/etc/, etc.)
    │  │  ├─ Load profiles from config file
    │  │  └─ Load current profile state
    │  │
    │  └─ Call: morph_device()
    │     ├─ Determine next device: (current + 1) % 6
    │     ├─ Get device profile
    │     ├─ morph_cowrie_banners()
    │     ├─ morph_router_html()
    │     ├─ morph_camera_html()
    │     ├─ morph_phase1_network() ──→ build/cowrie-dynamic/sbin/ifconfig
    │     ├─ morph_phase2_filesystem() → services/cowrie/honeyfs/
    │     ├─ morph_phase3_processes() ─→ build/cowrie-dynamic/bin/ps
    │     ├─ morph_phase4_behavior() ──→ Command delays config
    │     ├─ morph_phase5_temporal() ──→ Boot time, uptime, logs
    │     ├─ morph_phase6_quorum() ────→ Enable attack detection
    │     ├─ generate_session_variations()
    │     └─ save_current_profile(state_file)
    │        └─ Save: current_profile=1 to build/morph-state.txt
    │
    └─ Exit with status (0=success, 1=failure)
       └─ Log: "Successfully morphed to profile: Netgear_R7000"
```

### Quorum Engine Execution
```
./build/quorum [config_file]
    │
    ├─ main()
    │  └─ Call: run_quorum_logic()
    │     │
    │     ├─ scan_logs_for_ips()
    │     │  ├─ Read: services/cowrie/logs/cowrie.log
    │     │  ├─ Read: services/fake-router-web/logs/access.log
    │     │  ├─ Read: services/fake-camera-web/logs/access.log
    │     │  ├─ Read: services/rtsp/logs/*
    │     │  └─ Extract all IPs, track by service
    │     │
    │     ├─ detect_coordinated_attacks()
    │     │  ├─ For each IP:
    │     │  │  └─ If service_count >= 2:
    │     │  │     ├─ generate_alert()
    │     │  │     ├─ emit_morph_signal() ──→ build/signals/emergency_morph.signal
    │     │  │     └─ alert_count++
    │     │  └─ Return alert count
    │     │
    │     ├─ detect_lateral_movement()
    │     │  ├─ Look for: nmap, ping, arp, route, netstat
    │     │  ├─ Check: Internal IP ranges (172.17, 192.168, 10.0)
    │     │  ├─ Detect: Port sweeping (high hits, single service)
    │     │  └─ Return: Lateral movement alert count
    │     │
    │     └─ Log results:
    │        ├─ Write: build/quorum-alerts.log
    │        └─ Print: Stdout summary
    │
    └─ Exit with status (0=no alerts, 1=alerts found)
```

---

## 💻 Docker Integration

### Volume Mounts (docker-compose.yml)

```yaml
services:
  cowrie:
    volumes:
      # Generated command outputs (all 6 phases)
      - ../build/cowrie-dynamic:/data/cowrie-dynamic:ro

      # Configuration from morphing engine
      - ../services/cowrie/etc/cowrie.cfg.local:/cowrie/cowrie-git/etc/cowrie.cfg.local:ro

      # Fake filesystem from phase 2
      - ../services/cowrie/honeyfs:/cowrie/cowrie-git/share/cowrie/honeyfs:rw

      # SSH user database
      - ../services/cowrie/etc/userdb.txt:/cowrie/cowrie-git/etc/userdb.txt:ro
```

### How It Works

When Cowrie SSH starts:
```
1. Reads: /cowrie/cowrie-git/etc/cowrie.cfg.local
   └─ Gets: SSH banner from current morph
   └─ Gets: Device hostname, port settings, etc.

2. Accesses: /data/cowrie-dynamic/
   └─ When attacker runs "ifconfig"
      → Cowrie executes: cat /data/cowrie-dynamic/sbin/ifconfig
      → Returns Phase 1 output

   └─ When attacker runs "ps"
      → Cowrie executes: cat /data/cowrie-dynamic/bin/ps
      → Returns Phase 3 output

3. Uses: /cowrie/cowrie-git/share/cowrie/honeyfs/
   └─ Fake filesystem from Phase 2
   └─ /etc/passwd, /var/log/, etc. are read from here

4. Logs: Activity to /var/log/cowrie/
   └─ Scanned by Quorum engine
   └─ IPs and patterns extracted
```

---

## 🧮 Important Code Patterns

### Random Number Generation
```c
// IMPORTANT: Called ONCE at start of main()
// If called multiple times, resets sequence!
srand((unsigned int)(time(NULL) ^ getpid()));

// Then use rand() anywhere:
int random_value = rand() % 256;  // 0-255
int random_delay = 50 + (rand() % 450);  // 50-500ms
```

### String Handling
```c
// Safe string copy (prevent overflow):
strncpy(dest, src, MAX_SIZE - 1);
dest[MAX_SIZE - 1] = '\0';  // Ensure null termination

// Safe printf-style formatting:
snprintf(buffer, sizeof(buffer), "format: %s", value);
```

### File Operations
```c
// Write file safely:
FILE *f = fopen(filepath, "w");
if (!f) {
    log_event_level(LOG_ERROR, "Failed to open");
    return -1;
}
fprintf(f, content);
fclose(f);

// Check if file exists:
if (file_exists(filepath)) {
    // exists
}
```

### Logging
```c
// Log at different levels:
log_event_level(LOG_ERROR, "Error message");
log_event_level(LOG_WARN, "Warning message");
log_event_level(LOG_INFO, "Info message");
log_event_level(LOG_DEBUG, "Debug message");

// Or log to file:
log_event_file(LOG_WARN, "build/quorum-alerts.log", message);
```

---

## 🔧 Debugging & Development

### Enable Debug Logging
```c
// In any .c file:
log_event_level(LOG_DEBUG, "Variable value: %d", some_var);

// In header:
#define DEBUG 1  // Enable debug output
```

### Add New Device Profile
```c
// Edit: profiles.conf (or default_profiles in morph.c)

[My_New_Device]
ssh_banner=SSH-2.0-MyDeviceSSH_1.0
telnet_banner=My Device Banner
kernel_version=4.19.0
arch=armv7l
mac_prefix=aa:bb:cc
memory_mb=256
cpu_mhz=1200
```

### Add New Phase
```c
// In src/morph/morph.c:

int morph_phase7_newfeature(const char* device_name) {
    log_event_level(LOG_INFO, "Running Phase 7: New Feature");

    // Implementation here

    return 0;  // Success
}

// Then call in morph_device():
result += morph_phase7_newfeature(new_profile->name);
```

---

## 📚 Topics for NotebookLM Audio

Ask these for deep audio explanations:

1. "Walk me through the entire morphing process step by step"
2. "Explain how Phase 1 network generation works technically"
3. "How does the Quorum engine detect coordinated attacks?"
4. "Explain the data flow from morph engine to Cowrie service"
5. "How does emergency morphing break exploit chains?"
6. "Explain the device profile structure and why it's important"
7. "How does the temporal evolution simulation work?"
8. "Explain process simulation and why it matters"
9. "How does behavioral delay make CERBERUS look like a real device?"
10. "Walk through an attack scenario: what does Quorum see?"

---

**This documentation links technical code to practical explanation, perfect for NotebookLM audio content generation!**
