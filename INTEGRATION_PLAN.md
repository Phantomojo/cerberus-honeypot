# CERBERUS-COWRIE INTEGRATION PLAN

## Current State Analysis

### What We Discovered:

**Cowrie's Architecture:**
- Has ~55 Python command files that generate output dynamically in code
- Commands like `ifconfig`, `free`, `netstat` are hardcoded in Python
- **BUT:** Commands that DON'T have Python handlers fall through to the filesystem
- `ps`, `top`, `docker`, `systemctl` etc. can work if they're in the honeyfs

**What's Working:**
- ✅ `ps aux` - Reads from honeyfs, shows your generated process list
- ✅ `git --version` - Has Python handler but reads external file
- ✅ Environment variables (hostname, kernel) - Used by Cowrie
- ✅ Files in `/proc/`, `/etc/` - Read by `cat` and filesystem commands

**The Gap:**
- Python-based commands (ifconfig, netstat, free, etc.) ignore your files
- No integration layer between your C outputs and Cowrie's Python handlers

---

## SOLUTION OPTIONS

### Option 1: **Filesystem-First Approach** ⭐ RECOMMENDED
**Difficulty:** Low  
**Timeline:** 1-2 days  
**Effectiveness:** 60-70%

**Strategy:** Focus on making commands that Cowrie reads from filesystem work perfectly

**What to do:**
1. **Identify which commands use filesystem** (not Python handlers)
   - These already work: `ps`, `top`, `uptime`, `dmesg`
   - These could work: `docker`, `systemctl`, `route`, custom scripts

2. **Make them executable and properly formatted**
   - Current issue: Some are text files, not executables
   - Fix: Make them proper shell scripts with `#!/bin/sh`
   
3. **Add more filesystem-based commands**
   - Create scripts for common recon tools attackers use
   - Examples: `nmap`, `wget`, `curl`, `nc`, `python`, `gcc`

4. **Leverage `/proc/` filesystem**
   - Your `/proc/cpuinfo` exists but Cowrie's `cat` command might have override
   - Test all `/proc/` files: cpuinfo, meminfo, version, net/tcp, etc.

5. **Focus on what attackers actually run**
   - After gaining access: `ps`, `netstat`, `ls`, `find`, `uname`
   - Those mostly work already or can easily

**Pros:**
- Minimal Cowrie modification
- Your C code keeps generating outputs
- Quick to implement

**Cons:**
- Won't fix hardcoded commands like `ifconfig`, `free`
- Limited to ~40-50% of attacker commands

---

### Option 2: **Cowrie Python Patching** 
**Difficulty:** Medium-High  
**Timeline:** 1-2 weeks  
**Effectiveness:** 90%+

**Strategy:** Modify Cowrie's Python commands to read from your files

**What to do:**
1. **Create a Cowrie modification layer**
   ```python
   # File: /cowrie/cowrie-git/src/cowrie/commands/cerberus_loader.py
   import os
   import json
   
   CERBERUS_DYNAMIC = "/data/cowrie-dynamic"
   
   def load_cerberus_output(command_name):
       """Check if Cerberus has generated output for this command"""
       path = f"{CERBERUS_DYNAMIC}/bin/{command_name}"
       if os.path.exists(path):
           with open(path, 'r') as f:
               return f.read()
       return None
   
   def load_network_config():
       """Load Cerberus network configuration"""
       config_path = f"{CERBERUS_DYNAMIC}/network-config.json"
       if os.path.exists(config_path):
           with open(config_path, 'r') as f:
               return json.load(f)
       return None
   ```

2. **Patch key commands one by one**
   - Start with high-value targets: `ifconfig`, `netstat`, `free`, `df`
   - Modify each Python file to check Cerberus first:
   
   ```python
   # In /cowrie/cowrie-git/src/cowrie/commands/ifconfig.py
   from cowrie.commands.cerberus_loader import load_cerberus_output, load_network_config
   
   def call(self):
       # Try Cerberus first
       cerberus_output = load_cerberus_output("ifconfig")
       if cerberus_output:
           self.write(cerberus_output)
           return
       
       # Or use Cerberus network config
       config = load_network_config()
       if config:
           # Generate ifconfig from your JSON
           self.write(self.format_from_cerberus(config))
           return
       
       # Fall back to original Cowrie behavior
       self.original_call()
   ```

3. **Priority commands to patch:**
   - `ifconfig` (network info)
   - `netstat` (connections)
   - `free` (memory)
   - `df` (disk usage)
   - `route` (routing table)
   - `arp` (ARP cache)

**Pros:**
- Nearly complete integration
- Your C code fully utilized
- Professional implementation

**Cons:**
- Requires Python knowledge
- Must maintain patches across Cowrie updates
- Medium complexity

---

### Option 3: **Cowrie Plugin System**
**Difficulty:** Medium  
**Timeline:** 3-5 days  
**Effectiveness:** 70-80%

**Strategy:** Create a Cowrie plugin that intercepts commands

**What to do:**
1. **Research Cowrie's plugin architecture**
   - Cowrie has an output plugin system
   - Can we create a command plugin?

2. **Create a command interceptor plugin**
   ```python
   # cerberus_plugin.py
   class CerberusCommandPlugin:
       def before_command(self, command, args):
           """Check if Cerberus has output for this command"""
           cerberus_file = f"/data/cowrie-dynamic/bin/{command}"
           if os.path.exists(cerberus_file):
               return open(cerberus_file).read()
           return None  # Let Cowrie handle it
   ```

3. **Register for high-value commands**
   - Intercept before Cowrie's Python handlers run
   - Return Cerberus output if available
   - Fall through to Cowrie otherwise

**Pros:**
- Cleaner than patching
- Easier to maintain
- Can toggle on/off

**Cons:**
- Depends on Cowrie plugin capabilities
- May not be able to intercept all commands
- Requires understanding Cowrie internals

---

### Option 4: **Hybrid Approach** ⭐⭐ BEST LONG-TERM
**Difficulty:** Medium  
**Timeline:** 1 week  
**Effectiveness:** 85%+

**Strategy:** Combine filesystem (easy wins) + selective patching (high value)

**Phase 1 - Quick Wins (Day 1-2):**
1. Fix filesystem-based commands (Option 1)
2. Make all scripts executable
3. Test and verify what works

**Phase 2 - High-Value Patches (Day 3-5):**
1. Create cerberus_loader.py helper
2. Patch just 5-10 critical commands:
   - `ifconfig`, `netstat`, `free`, `df`, `route`
3. Test integration

**Phase 3 - Polish (Day 6-7):**
1. Document which commands use Cerberus
2. Add fallback mechanisms
3. Test with real attacker scenarios

**Pros:**
- Quick initial results
- Focuses effort on high-impact items
- Maintainable
- Can expand over time

**Cons:**
- Still some commands won't integrate
- Requires some Python work

---

## RECOMMENDATION

**Start with Option 4 (Hybrid):**

### Week 1 Plan:

**Day 1: Filesystem Foundation**
- Make all your dynamic commands executable scripts
- Test: `ps`, `top`, `docker`, `systemctl`, `python`, `git`
- Verify they show in honeypot

**Day 2: Critical Patches**
- Create `cerberus_loader.py` helper module
- Patch `ifconfig.py` to read your network-config.json
- Test network command integration

**Day 3: Expand Coverage**
- Patch `netstat.py` to read your netstat output
- Patch `free.py` to read your memory info
- Patch `df.py` for disk usage

**Day 4: Process Integration**
- Ensure `ps` and `top` fully work with your generated lists
- Test process-related reconnaissance

**Day 5: Testing & Debugging**
- Run through common attacker workflows
- Fix any integration bugs
- Document what works

**Day 6-7: Documentation & Polish**
- Create integration test suite
- Document which commands use Cerberus
- Make morph → restart → test cycle smooth

---

## MEASUREMENTS OF SUCCESS

### Target Integration Levels:

**After Phase 1 (Filesystem):**
- 40-50% of attacker commands use Cerberus ✅

**After Phase 2 (Critical Patches):**
- 70-80% of attacker commands use Cerberus ✅

**After Phase 3 (Full Integration):**
- 85-90% of attacker commands use Cerberus ✅

### Key Metrics:

1. **Command Coverage**
   - How many of the top 20 attacker commands show Cerberus output?
   - Target: 15/20 (75%)

2. **Consistency**
   - Does `uname -a` match `/proc/version` match `cat /etc/os-release`?
   - Target: 100% consistent device profile

3. **Realism**
   - Does the output fool detection tools?
   - Can we pass fingerprinting tests?

---

## NEXT STEPS

**Right now, let's:**
1. Pick an approach (I recommend Option 4 - Hybrid)
2. Start with Day 1 tasks (filesystem commands)
3. Get quick wins to build momentum
4. Then tackle Python patching for high-value commands

**Which option sounds best to you?**

Or do you want to:
- A) Focus only on filesystem (safest, quickest)
- B) Go all-in on Python patching (most complete)
- C) Hybrid approach (balanced)
- D) Something completely different?

Let me know and we can start implementing immediately!
