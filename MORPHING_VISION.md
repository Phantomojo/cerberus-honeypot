# CERBERUS Vision: Bio-Adaptive Self-Morphing Honeypot

## Core Philosophy

CERBERUS is designed as a **never-ending Rubik's cube** - a constantly shifting, self-morphing honeypot where no two attackers see the same system, and even the same attacker encounters variations between sessions. The goal is to keep attackers in a state of limbo, observing them while remaining undetectable.

## The "Limbo State" Concept

### What is Limbo?
In CERBERUS, "limbo" refers to a state where:
- Attackers cannot establish a consistent fingerprint of the system
- Each reconnaissance attempt yields slightly different results
- Comparing notes with other attackers creates confusion rather than clarity
- The honeypot adapts faster than the attacker can analyze

### How We Achieve Limbo
1. **Profile-Level Morphing**: Device type changes (router → camera → different router)
2. **Session-Level Variations**: Each connection sees unique system details
3. **Time-Based Drift**: System details shift gradually over time
4. **Behavioral Randomization**: Response times, errors, and outputs vary

## Current Implementation

### ✅ Implemented Features

#### 1. Device Profile Morphing
- **6 realistic device profiles** (TP-Link, D-Link, Netgear routers; Hikvision, Dahua cameras)
- **Automatic rotation** through profiles on schedule
- **Complete fingerprint changes**:
  - SSH/Telnet banners
  - Kernel versions (2.6.30 → 3.10.49 → etc.)
  - System architecture (MIPS → ARM)
  - Hostname
  - Web UI themes

#### 2. System Information Variation
Each profile includes:
- **Kernel version**: Device-specific Linux kernels
- **Architecture**: mips, armv7l (matches real IoT devices)
- **MAC address prefix**: Vendor-specific OUI
- **Memory size**: 32MB to 256MB (device-appropriate)
- **CPU speed**: 400MHz to 1000MHz

#### 3. Session Variations (NEW)
Each morphing cycle now generates:
- **Random MAC addresses** using vendor prefix
- **Randomized uptime** (1-365 days)
- **Memory variations** (±10% from base)

These make each session unique even with the same profile.

### What Attackers See

#### Before CERBERUS Enhancement:
```bash
# Attacker A connects:
$ hostname
honeypot

$ uname -a
Linux honeypot 3.2.0-generic x86_64 GNU/Linux
```

#### After CERBERUS Enhancement:
```bash
# Attacker A connects (Session 1):
$ hostname  
Netgear_R7000

$ uname -a
Linux Netgear_R7000 2.6.36.4brcmarm #1 SMP PREEMPT armv7l GNU/Linux

$ ifconfig eth0
eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 192.168.1.1  netmask 255.255.255.0
        ether a0:63:91:3f:a2:b1  txqueuelen 1000

$ cat /proc/meminfo
MemTotal:        262144 kB

# Attacker B connects (Same day, different session):
$ hostname
Netgear_R7000

$ uname -a
Linux Netgear_R7000 2.6.36.4brcmarm #1 SMP PREEMPT armv7l GNU/Linux

$ ifconfig eth0
eth0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 192.168.1.1  netmask 255.255.255.0
        ether a0:63:91:8d:12:4f  txqueuelen 1000  # DIFFERENT MAC!

$ cat /proc/meminfo
MemTotal:        245760 kB  # DIFFERENT MEMORY!

# Next day, morphing engine runs:
$ hostname
Hikvision_DS-2CD2  # COMPLETELY DIFFERENT DEVICE!

$ uname -a
Linux Hikvision_DS-2CD2 3.0.8 #1 SMP PREEMPT armv7l GNU/Linux
```

## Future Enhancements (Roadmap)

### Phase 1: Network Layer Variation (NEXT)
- [ ] Dynamic IP address changes
- [ ] Varying network interface names (eth0 vs wlan0)
- [ ] Different routing tables per session
- [ ] Randomized ARP cache entries

### Phase 2: Filesystem Dynamics
- [ ] Varying directory structures
- [ ] Random file timestamps
- [ ] Different available commands per device profile
- [ ] Session-specific log files

### Phase 3: Process Simulation
- [ ] Dynamic process lists
- [ ] Randomized PIDs
- [ ] Different running services per profile
- [ ] Varying resource usage patterns

### Phase 4: Behavioral Adaptation
- [ ] Random command execution delays
- [ ] Varying error messages
- [ ] Different command output formats
- [ ] Session timeout variations

### Phase 5: Temporal Evolution
- [ ] Gradual system "aging" (increasing uptime)
- [ ] Log file accumulation
- [ ] Configuration file modifications over time
- [ ] Service restart simulation

### Phase 6: Quorum-Based Adaptation
- [ ] Detect coordinated attacks
- [ ] Increase morphing frequency when under attack
- [ ] Deploy countermeasures (delays, fake errors)
- [ ] Share attack patterns across instances

## Design Principles

### 1. Every Detail Matters
"Every detail must be checked to the letter" - Nothing should reveal the honeypot nature:
- MAC addresses match vendor OUIs
- Memory sizes appropriate for device type
- Kernel versions authentic to real devices
- Process lists match actual IoT firmware
- File timestamps realistic for uptime

### 2. Uniqueness Per Session
No two connections should see identical systems:
- Random MAC addresses (vendor prefix preserved)
- Varying uptimes
- Different memory amounts (within realistic bounds)
- Unique process IDs
- Session-specific file modifications

### 3. Coordinated Morphing
All system elements must stay consistent:
- ARM device → ARM processes, ARM binaries
- Router profile → router services running
- Camera profile → camera-specific commands available
- Uptime → file timestamps consistent with it

### 4. Imperceptible to Attackers
Observers remain undetected:
- No obvious honeypot indicators
- Realistic response times
- Authentic error messages
- Real IoT device quirks replicated

## Technical Implementation

### Morphing Engine
```c
// Profile rotation (scheduled via cron)
./build/morph  → Rotates to next device profile

// Session variations (per-session)
generate_random_mac()     → Unique MAC each session
generate_session_variations() → Uptime, memory variations
```

### Quorum Engine
```c
// Attack detection (monitors all logs)
./build/quorum → Detects coordinated attacks

// Future: Trigger rapid morphing when under attack
if (coordinated_attack_detected()) {
    trigger_immediate_morph();
}
```

### Integration with Cowrie
Cowrie configuration files are dynamically updated:
- `cowrie.cfg.local` - Profile-specific settings
- Future: `txtcmds/` - Custom command outputs per session
- Future: `fs.pickle` - Dynamic filesystem per profile

## Success Metrics

### Imperceptibility
- [ ] Passes automated honeypot detection tools
- [ ] Realistic response times (matches real IoT devices)
- [ ] Authentic fingerprints (Shodan, Censys match real devices)

### Variability
- [ ] Different sessions produce different system info
- [ ] Attackers comparing notes see inconsistencies
- [ ] Morphing undetectable to individual attackers

### Intelligence Gathering
- [ ] Captures complete attack chains
- [ ] Correlates attacks across profiles
- [ ] Identifies new IoT exploits
- [ ] Tracks attacker adaptation strategies

## Why This Matters

### Traditional Honeypot Problems:
1. **Static fingerprints** - Easy to identify and blacklist
2. **Predictable behavior** - Attackers learn and avoid
3. **Single perspective** - Only see one device type
4. **No adaptation** - Can't respond to new attack patterns

### CERBERUS Advantages:
1. **Dynamic fingerprints** - Never the same system twice
2. **Unpredictable behavior** - Keeps attackers guessing
3. **Multiple perspectives** - See attacks against various IoT devices
4. **Adaptive response** - Morphs faster when under attack

## Analogy: The Never-Ending Rubik's Cube

Imagine a Rubik's cube that:
- Changes colors while you're solving it
- Adds new faces randomly
- Shifts patterns between attempts
- Never lets you see the same configuration twice

That's CERBERUS. Every time an attacker thinks they understand the system, it shifts. They're in perpetual limbo, never quite certain what they're looking at, while we observe every move they make.

---

**Status**: Phase 1 Complete (Profile Morphing + Session Variations)  
**Next**: Network Layer Variation Implementation  
**Vision**: Complete self-morphing honeypot ecosystem
