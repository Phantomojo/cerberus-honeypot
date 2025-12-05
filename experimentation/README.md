# CERBERUS Honeypot - Experimentation & Reverse Engineering

**⚠️ WARNING: This directory contains experimental code and reverse engineering artifacts. Not for production use.**

---

## Overview

This directory contains all experimental work, reverse engineering analysis, and research related to building CERBERUS's custom SSH/Telnet honeypot by studying and adapting Cowrie.

## Directory Structure

```
experimentation/
├── README.md                          # This file
├── reverse_engineer_cowrie.sh         # Automated analysis script
├── analysis/                          # Analysis results
│   ├── 01_directory_structure.txt     # Cowrie code structure
│   ├── 02_code_stats.txt              # Code statistics
│   ├── 03_dependencies.txt            # Python dependencies
│   ├── 04_classes_functions.txt       # Classes and functions
│   ├── 05_protocol_analysis.txt       # SSH/Telnet protocols
│   ├── 06_authentication.txt          # Auth mechanisms
│   ├── 07_commands.txt                # Command implementations
│   ├── 08_filesystem.txt              # Filesystem emulation
│   ├── 09_sessions.txt                # Session management
│   ├── 10_logging.txt                 # Logging mechanisms
│   ├── 11_configs.txt                 # Configuration files
│   ├── 12_network.txt                 # Network code
│   └── 13_security.txt                # Security features
├── cowrie/                            # Cowrie source code (cloned)
├── prototypes/                        # Prototype implementations
├── notes/                             # Research notes
└── custom-ssh/                        # Our custom SSH honeypot

```

---

## Reverse Engineering Goals

### Primary Objectives

1. **Understand Cowrie Architecture**
   - How SSH/Telnet protocols are implemented
   - Session management and authentication flow
   - Command execution and filesystem emulation
   - Logging and data capture mechanisms

2. **Extract Key Components**
   - Reusable code patterns
   - Protocol handling strategies
   - Command simulation techniques
   - Filesystem virtualization methods

3. **Design CERBERUS-Native Honeypot**
   - Integrate with morph engine
   - Use our path security module
   - Support dynamic command generation
   - Match our C-based architecture

4. **Improve Upon Cowrie**
   - Better IoT device emulation
   - Lighter weight (C vs Python)
   - Native integration with CERBERUS
   - Real-time morphing capabilities

---

## Cowrie Analysis Summary

### Architecture Overview

**Technology Stack:**
- Language: Python (34,663 lines of code)
- Framework: Twisted (async networking)
- Total Files: 212 Python files
- Dependencies: 13 core packages

**Key Components:**

1. **Protocol Handlers**
   - `src/cowrie/ssh/` - SSH protocol implementation
   - `src/cowrie/telnet/` - Telnet protocol implementation
   - Uses Twisted Conch for SSH
   - Custom Telnet state machine

2. **Shell Emulation**
   - `src/cowrie/shell/honeypot.py` - Main shell logic (21,318 lines!)
   - `src/cowrie/shell/fs.py` - Virtual filesystem (7,795 lines)
   - `src/cowrie/shell/protocol.py` - Command protocol (13,505 lines)

3. **Command System**
   - 100+ command implementations
   - Each command is a separate module
   - Commands return realistic output
   - Some commands are interactive (e.g., apt, wget)

4. **Authentication**
   - Pluggable auth backends
   - Supports username/password databases
   - Can accept any credentials (honeypot mode)
   - Logs all auth attempts

5. **Logging/Output**
   - JSON log output
   - Text log output
   - Multiple output plugins (MySQL, Splunk, etc.)
   - Session recording

---

## Key Insights

### What Cowrie Does Well

✅ **Comprehensive Command Set**
   - 100+ Linux commands implemented
   - Realistic command outputs
   - Interactive commands (wget, curl, apt)

✅ **Robust Protocol Implementation**
   - Full SSH protocol support
   - Telnet with proper negotiation
   - Session management

✅ **Flexible Architecture**
   - Plugin-based output system
   - Configurable filesystem
   - Extensible command system

✅ **Rich Logging**
   - Detailed session logs
   - Command history
   - File download capture

### What We Can Improve

⚠️ **Performance**
   - Python overhead vs C
   - Twisted async complexity
   - Heavy memory footprint

⚠️ **Integration**
   - Doesn't integrate with our morph engine
   - Static configuration
   - No real-time morphing

⚠️ **IoT Focus**
   - Generic Linux, not IoT-specific
   - Doesn't emulate router/camera behavior
   - Missing IoT-specific quirks

⚠️ **Deployment**
   - Complex Docker setup
   - Many dependencies
   - Hard to customize on-the-fly

---

## CERBERUS Custom Honeypot Design

### Approach: Hybrid Architecture

Instead of rewriting Cowrie entirely, we'll:

1. **Keep Cowrie for Complex Features**
   - Use it for SSH protocol (battle-tested)
   - Leverage existing command implementations
   - Utilize logging infrastructure

2. **Integrate with CERBERUS**
   - Pre-configure Cowrie from morph engine
   - Generate dynamic configs
   - Inject morphed filesystem
   - Override commands with our outputs

3. **Build C-Based Lightweight Alternative** (Future)
   - Simple SSH/Telnet server in C
   - Integration with our codebase
   - Minimal dependencies
   - Real-time morphing

---

## Implementation Strategy

### Phase 1: Enhanced Cowrie Integration ✅ (Current)

**Goal:** Make Cowrie work seamlessly with CERBERUS

**Tasks:**
- [x] Analyze Cowrie architecture
- [x] Identify configuration points
- [x] Design integration strategy
- [ ] Fix Docker startup issues
- [ ] Auto-generate Cowrie configs from morph
- [ ] Inject dynamic commands
- [ ] Synchronize device profiles

**Files to Modify:**
```
services/cowrie/etc/cowrie.cfg     # Main config
services/cowrie/honeyfs/           # Virtual filesystem
build/cowrie-dynamic/              # Dynamic commands
docker/docker-compose.yml          # Container setup
```

### Phase 2: Cowrie Customization 🔄 (In Progress)

**Goal:** Customize Cowrie to emulate IoT devices better

**Tasks:**
- [ ] Create IoT-specific command outputs
- [ ] Add router-specific commands (nvram, uci)
- [ ] Add camera-specific commands (v4l2-ctl)
- [ ] Implement device-specific errors
- [ ] Add realistic delays and jitter

**New Commands Needed:**
```
nvram get/set      # Router NVRAM
uci show           # OpenWrt config
v4l2-ctl           # Video4Linux
iw/iwconfig        # Wireless tools
dmesg (IoT ver)    # IoT kernel messages
```

### Phase 3: Lightweight C Alternative 📋 (Planned)

**Goal:** Build native C SSH/Telnet honeypot

**Technology:**
- libssh for SSH protocol
- Raw sockets for Telnet
- Integration with our morph engine
- Use existing path security module

**Advantages:**
- 10x faster startup
- 100x lower memory usage
- Native C integration
- Real-time morphing
- No Python dependencies

**Architecture:**
```
src/honeypot/
├── ssh_server.c          # SSH protocol handler
├── telnet_server.c       # Telnet protocol handler
├── shell_emulator.c      # Command execution
├── command_handler.c     # Command dispatch
├── session_manager.c     # Session tracking
└── virtual_fs.c          # Filesystem emulation
```

---

## Reverse Engineering Methods Used

### 1. Static Code Analysis
- Directory structure mapping
- Dependency extraction
- Class/function inventory
- Import graph analysis

### 2. Dynamic Analysis
- Docker container inspection
- Runtime behavior observation
- Log analysis
- Network traffic capture

### 3. Documentation Review
- README files
- Configuration files
- Code comments
- API documentation

### 4. Pattern Extraction
- Authentication flows
- Command execution paths
- Session lifecycle
- Data flow diagrams

### 5. Comparative Analysis
- Compare with other honeypots
- Identify unique features
- Find improvement opportunities

---

## Tools Used

### Analysis Tools
- `grep` - Pattern searching
- `find` - File discovery
- `tree` - Directory visualization
- `wc` - Code metrics
- `git` - Source control

### Debugging Tools
- `docker inspect` - Container analysis
- `docker logs` - Runtime logs
- `strace` - System call tracing (future)
- `tcpdump` - Network capture (future)

### Development Tools
- `gcc` - C compilation
- `gdb` - Debugging (future)
- `valgrind` - Memory analysis (future)
- `perf` - Performance profiling (future)

---

## Research Notes

### Cowrie Dependencies

**Critical:**
- `twisted` - Async networking framework (25.5.0)
- `cryptography` - Crypto operations (46.0.3)
- `pyasn1_modules` - ASN.1 parsing (0.4.2)

**Important:**
- `bcrypt` - Password hashing (5.0.0)
- `requests` - HTTP client (2.32.5)
- `tftpy` - TFTP support (0.8.6)

**Optional:**
- Various output plugins (MySQL, Splunk, etc.)

### Command Implementation Pattern

Each command follows this pattern:
```python
from cowrie.shell.command import HoneyPotCommand

class Command_ls(HoneyPotCommand):
    def call(self):
        # Parse arguments
        # Generate output
        # Write to terminal
        return
```

### Filesystem Structure

Cowrie uses a JSON-based filesystem:
- `share/cowrie/fs.pickle` - Serialized filesystem
- Directory tree stored in memory
- Files can be real or fake
- Supports symbolic links

### Session Flow

1. Connection received (SSH/Telnet)
2. Authentication (any password accepted)
3. Shell prompt displayed
4. Commands executed in loop
5. Output generated and logged
6. Session timeout or disconnect

---

## Next Steps

### Immediate (This Week)

1. **Fix Cowrie Docker Issues**
   - Resolve entrypoint problem
   - Test SSH connectivity
   - Verify Telnet works
   - Validate logging

2. **Dynamic Configuration**
   - Generate cowrie.cfg from morph
   - Inject device profile data
   - Sync network settings
   - Update filesystem

3. **Command Integration**
   - Copy dynamic commands to honeyfs
   - Test command execution
   - Verify output format
   - Check logging

### Short-term (This Month)

1. **IoT Command Development**
   - Implement nvram command
   - Add uci command
   - Create v4l2-ctl mock
   - Add wireless tools

2. **Behavioral Tuning**
   - Add IoT-specific delays
   - Implement device quirks
   - Customize error messages
   - Add resource limits (slow CPU)

3. **Testing & Validation**
   - Test with real attack tools
   - Verify deception quality
   - Measure performance
   - Check logs completeness

### Long-term (Next Quarter)

1. **C-Based Honeypot Development**
   - Design architecture
   - Choose SSH library
   - Implement core features
   - Performance testing

2. **Advanced Features**
   - Machine learning for responses
   - Automated profile generation
   - Multi-honeypot coordination
   - Advanced evasion detection

3. **Integration & Deployment**
   - CI/CD pipeline
   - Automated testing
   - Production deployment
   - Monitoring & alerting

---

## Lessons Learned

### Technical Insights

1. **Complexity vs Capability**
   - Cowrie is powerful but complex
   - 34K lines for full feature set
   - Trade-off: features vs maintainability

2. **Async is Hard**
   - Twisted adds significant complexity
   - Callback hell in places
   - Hard to debug

3. **Protocol Handling**
   - SSH protocol is complex (RFC 4251-4256)
   - Telnet is simpler but has quirks
   - Both need proper state machines

4. **Filesystem Emulation**
   - In-memory tree is fast
   - JSON serialization works well
   - Need realistic file contents

### Strategic Insights

1. **Don't Reinvent Everything**
   - Use Cowrie for SSH heavy lifting
   - Focus on IoT-specific customization
   - C rewrite only if necessary

2. **Integration > Replacement**
   - Make Cowrie play nice with CERBERUS
   - Use morph engine for configuration
   - Share security modules

3. **Incremental Improvement**
   - Fix immediate issues first
   - Add features gradually
   - Test thoroughly at each step

---

## References

### Cowrie Resources
- GitHub: https://github.com/cowrie/cowrie
- Documentation: https://cowrie.readthedocs.io/
- Community: https://www.cowrie.org/

### Honeypot Research
- The Honeynet Project: https://www.honeynet.org/
- SANS Internet Storm Center: https://isc.sans.edu/
- Shodan IoT Search: https://www.shodan.io/

### SSH/Telnet Specs
- RFC 4251: SSH Protocol Architecture
- RFC 4252: SSH Authentication Protocol
- RFC 4253: SSH Transport Layer Protocol
- RFC 854: Telnet Protocol Specification

### Security Research
- OWASP IoT Top 10
- Mirai Botnet Analysis
- Common IoT Vulnerabilities

---

## Contributing

This is experimental work. If you're working in this directory:

1. **Document Everything**
   - Add notes to `notes/`
   - Update this README
   - Comment your code

2. **Keep It Organized**
   - Use subdirectories
   - Name files clearly
   - Clean up when done

3. **Test Safely**
   - Use isolated environments
   - Don't affect production
   - Validate before integrating

4. **Share Insights**
   - Document findings
   - Share patterns discovered
   - Report issues found

---

## Status

**Last Updated:** December 3, 2025  
**Status:** Active Development  
**Phase:** Phase 1 - Enhanced Cowrie Integration  
**Progress:** 40% Complete

**Recent Achievements:**
- ✅ Cowrie source code cloned
- ✅ Comprehensive reverse engineering analysis
- ✅ 13 analysis reports generated
- ✅ Architecture understood
- ✅ Integration strategy designed

**Current Blockers:**
- ⚠️ Docker entrypoint configuration
- ⚠️ Cowrie startup script issues

**Next Milestone:**
- 🎯 Get Cowrie SSH honeypot fully operational
- 🎯 Integrate with morph engine
- 🎯 Test end-to-end SSH session

---

## Contact

For questions about this experimental work:
- Check `analysis/` reports first
- Review Cowrie documentation
- Consult CERBERUS main docs
- Ask the team

**Remember:** This is experimentation. Break things, learn, iterate! 🚀
