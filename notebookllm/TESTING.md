# CERBERUS Honeypot - Testing Guide

This document provides comprehensive instructions on **where** and **how** to test the CERBERUS Bio-Adaptive IoT Honeynet.

---

## Table of Contents

1. [Testing Overview](#testing-overview)
2. [Test Environments](#test-environments)
3. [Component Testing](#component-testing)
4. [Integration Testing](#integration-testing)
5. [Security Testing](#security-testing)
6. [Automated Testing](#automated-testing)
7. [Manual Testing Scenarios](#manual-testing-scenarios)

---

## Testing Overview

CERBERUS honeypot testing is divided into several layers:

- **Unit Tests**: Test individual C modules (morph, quorum, utils)
- **Component Tests**: Test each service independently
- **Integration Tests**: Test the full system working together
- **Security Tests**: Validate honeypot behavior and isolation
- **Performance Tests**: Ensure morphing and detection work under load

---

## Test Environments

### 1. Local Development Environment (Recommended for Development)

**Best for**: Developing and testing C modules without full deployment

**Requirements**:
- Linux system (Ubuntu/Debian/Arch)
- GCC compiler
- No Docker required
- Fast iteration cycles

**Setup**:
```bash
# Build C modules
make clean && make

# Run tests
make test
```

**What you can test**:
- ✅ Morphing engine logic
- ✅ Quorum detection logic
- ✅ File I/O operations
- ✅ Configuration parsing
- ❌ Full honeypot interaction (requires Docker)

---

### 2. Docker Local Environment (Recommended for Full Testing)

**Best for**: Testing complete honeypot system with all services

**Requirements**:
- Docker Engine (v20.10+)
- Docker Compose (v2.0+)
- 2GB RAM minimum
- 10GB disk space

**Setup**:
```bash
# 1. Run setup
./setup.sh

# 2. Configure services
./services/setup-services.sh

# 3. Build C modules
make

# 4. Start Docker services
cd docker
docker compose up -d

# 5. Run C engines
cd ..
./build/morph
./build/quorum
```

**What you can test**:
- ✅ Complete honeypot system
- ✅ SSH/Telnet emulation (Cowrie)
- ✅ Web UI emulation (Router/Camera)
- ✅ RTSP video streaming
- ✅ Device morphing
- ✅ Attack detection
- ✅ Log correlation

---

### 3. Isolated VM/Cloud Environment (Recommended for Production Testing)

**Best for**: Testing with real attackers in a safe environment

**Requirements**:
- Virtual Machine or Cloud Instance (AWS EC2, Azure VM, etc.)
- Isolated network (separate VLAN or VPC)
- Firewall rules configured
- Monitoring tools

**Setup**:
```bash
# Same as Docker environment, but deployed on isolated VM
# See SETUP_PLAN.md for deployment details
```

**What you can test**:
- ✅ Everything from Docker environment
- ✅ Real attack traffic
- ✅ Network isolation
- ✅ Long-term stability
- ✅ Log analysis with real data

**⚠️ SECURITY WARNING**: Never expose honeypots on production networks!

---

## Component Testing

### 1. Testing the Morphing Engine

The morphing engine rotates device profiles and updates configurations.

#### Manual Testing

```bash
# Test profile rotation
./build/morph

# Check generated files
cat build/morph-state.txt          # Current profile
cat build/morph-events.log         # Morphing history
cat services/cowrie/etc/cowrie.cfg.local  # Updated Cowrie config
cat services/fake-router-web/html/index.html  # Updated router HTML
```

#### Expected Output

```
Bio-Adaptive IoT Honeynet Morphing Engine
Morph event: Rotating device profile at [timestamp]
[INFO] Morphing to profile: [ProfileName]
[INFO] Cowrie banners updated for profile: [ProfileName]
[INFO] Router HTML theme updated
[INFO] Camera HTML theme updated
[INFO] Successfully morphed to profile: [ProfileName]
```

#### Verification Checklist

- [ ] Profile rotates to next in sequence
- [ ] `morph-state.txt` shows current profile
- [ ] `morph-events.log` records morphing event
- [ ] Cowrie config file updated with correct banners
- [ ] Router HTML contains correct theme
- [ ] Camera HTML contains correct theme
- [ ] No errors in output

#### Automated Testing

```bash
make test-morph
```

---

### 2. Testing the Quorum Detection Engine

The quorum engine correlates attack events across services.

#### Manual Testing

```bash
# Generate some test log entries
mkdir -p services/cowrie/logs
echo "[2024-11-24 10:00:00] Connection from 192.168.1.100" > services/cowrie/logs/cowrie.log
echo "[2024-11-24 10:01:00] Failed login from 192.168.1.100" >> services/cowrie/logs/cowrie.log

# Run quorum engine
./build/quorum
```

#### Expected Output

```
Bio-Adaptive IoT Honeynet Quorum Engine
Quorum check: Checking logs at [timestamp]
[INFO] Scanning logs for IP addresses
[INFO] Found IP: 192.168.1.100 (Count: 2)
[INFO] Total unique IPs tracked: 1
[INFO] Detecting coordinated attacks
[INFO] No coordinated attacks detected (threshold not met)
```

#### Verification Checklist

- [ ] Engine scans all log directories
- [ ] IP addresses extracted correctly
- [ ] Duplicate IPs counted
- [ ] Coordinated attacks detected when threshold met
- [ ] Alerts generated for suspicious activity
- [ ] No false positives with legitimate traffic

#### Automated Testing

```bash
make test-quorum
```

---

### 3. Testing Cowrie (SSH/Telnet Honeypot)

#### Prerequisites

```bash
# Start Cowrie service
cd docker
docker compose up -d cowrie
```

#### Testing SSH

```bash
# Connect to SSH honeypot
ssh -p 2222 root@localhost

# Try some commands (should be logged)
ls
pwd
whoami
exit
```

#### Testing Telnet

```bash
# Connect to Telnet honeypot
telnet localhost 2323

# Login with any credentials (should be logged)
# Username: root
# Password: password
```

#### Verification Checklist

- [ ] SSH accepts connections on port 2222
- [ ] Telnet accepts connections on port 2323
- [ ] Login attempts are logged
- [ ] Commands are logged
- [ ] Fake filesystem responds realistically
- [ ] Banners match current device profile

#### Check Logs

```bash
docker compose logs cowrie
cat ../services/cowrie/logs/cowrie.log
```

---

### 4. Testing Web UI Emulators

#### Testing Router Web Interface

```bash
# Access router UI
curl http://localhost:80

# Or open in browser
xdg-open http://localhost:80
```

#### Testing Camera Web Interface

```bash
# Access camera UI
curl http://localhost:8080

# Or open in browser
xdg-open http://localhost:8080
```

#### Verification Checklist

- [ ] Router UI loads correctly
- [ ] Camera UI loads correctly
- [ ] HTML contains honeypot warning
- [ ] Theme matches current device profile
- [ ] Pages look realistic
- [ ] No broken links or resources

---

### 5. Testing RTSP Server

#### Testing RTSP Stream

```bash
# Test RTSP connection (requires ffmpeg or VLC)
ffmpeg -rtsp_transport tcp -i rtsp://localhost:554/stream -t 5 -f null -

# Or use VLC
vlc rtsp://localhost:554/stream
```

#### Verification Checklist

- [ ] RTSP server responds on port 554
- [ ] Stream can be accessed
- [ ] Connection attempts are logged
- [ ] No errors in MediaMTX logs

---

## Integration Testing

### Full System Test

This tests all components working together.

#### Setup

```bash
# 1. Start all services
cd docker
docker compose up -d

# 2. Verify all services running
docker compose ps

# Expected output:
# NAME                    STATUS
# cerberus-cowrie        running
# cerberus-router-web    running
# cerberus-camera-web    running
# cerberus-rtsp          running
```

#### Test Sequence

```bash
# 3. Run morphing engine
cd ..
./build/morph

# 4. Simulate attack from same IP on multiple services
ssh -p 2222 test@localhost  # Attempt 1
curl http://localhost:80    # Attempt 2
telnet localhost 2323       # Attempt 3

# 5. Run quorum detection
./build/quorum

# Expected: Should detect coordinated attack from localhost
```

#### Verification Checklist

- [ ] All Docker services start successfully
- [ ] Services respond to requests
- [ ] Morphing updates all configurations
- [ ] Logs are generated in correct locations
- [ ] Quorum engine detects cross-service attacks
- [ ] No service crashes or errors

---

## Security Testing

### Testing Honeypot Isolation

**Important**: These tests verify the honeypot is properly isolated.

#### Network Isolation Test

```bash
# 1. Check that honeypot cannot access internal network
docker compose exec cowrie ping -c 1 192.168.1.1
# Expected: Should fail or be blocked

# 2. Check that honeypot can only access intended services
docker compose exec cowrie ping -c 1 router-web
# Expected: Should work (same Docker network)
```

#### Data Leakage Test

```bash
# 1. Check that no sensitive data in logs
grep -r "password" logs/
grep -r "secret" logs/
grep -r "api_key" logs/

# Expected: No real credentials should appear
```

#### Container Escape Test

```bash
# 1. Try to break out of container
docker compose exec cowrie bash -c "whoami"
# Expected: Should be cowrie user, not root

# 2. Check for privileged containers
docker compose config | grep privileged
# Expected: Should be "false" or not present
```

---

## Automated Testing

### Running All Tests

```bash
# Run complete test suite
make test

# This will:
# 1. Test C module compilation
# 2. Run unit tests for morph engine
# 3. Run unit tests for quorum engine
# 4. Test configuration generation
# 5. Validate profile rotation
# 6. Test log parsing
```

### Continuous Integration

The test suite can be integrated into CI/CD pipelines:

```bash
# Example: GitHub Actions, GitLab CI, Jenkins
make clean
make
make test
```

---

## Manual Testing Scenarios

### Scenario 1: Device Morphing Over Time

**Goal**: Verify device profiles rotate correctly over time

**Steps**:
1. Run morphing engine: `./build/morph`
2. Note current profile from output
3. Wait for next morph interval (or run again)
4. Verify profile changes
5. Check that services pick up new configurations

**Success Criteria**:
- Profiles rotate in sequence
- Each service reflects the new profile
- No duplicate profiles in succession
- Logs record all morph events

---

### Scenario 2: Coordinated Attack Detection

**Goal**: Verify quorum engine detects attacks from same source

**Steps**:
1. Start all services
2. From external machine, attempt connections to:
   - SSH (port 2222)
   - Router web (port 80)
   - Camera web (port 8080)
3. Run quorum engine: `./build/quorum`
4. Check for coordinated attack alert

**Success Criteria**:
- Quorum engine identifies the IP
- Count shows attempts across multiple services
- Alert generated when threshold exceeded
- No false positives from legitimate traffic

---

### Scenario 3: Long-Running Stability Test

**Goal**: Verify honeypot runs stably for extended periods

**Steps**:
1. Start all services
2. Run morphing engine on cron schedule (every hour)
3. Run quorum engine on cron schedule (every 15 minutes)
4. Let system run for 24-48 hours
5. Check logs for errors or crashes

**Success Criteria**:
- No service crashes
- No memory leaks
- Logs rotate correctly
- All morphing events successful
- No missed attack detections

---

### Scenario 4: Performance Under Load

**Goal**: Test system handles multiple simultaneous attacks

**Steps**:
1. Start all services
2. Use tool like `hping3` or custom script to generate traffic
3. Monitor system resources (CPU, memory, disk)
4. Verify all attacks are logged
5. Check quorum detection still works

**Success Criteria**:
- System remains responsive
- No log entries dropped
- Quorum engine processes all IPs
- Resource usage stays within acceptable limits

---

## Testing Best Practices

### Before Testing

- [ ] Read all documentation (README, SETUP_PLAN, DEPENDENCIES)
- [ ] Ensure all dependencies installed
- [ ] Build project successfully (`make`)
- [ ] Verify Docker services start correctly

### During Testing

- [ ] Test one component at a time
- [ ] Document any unexpected behavior
- [ ] Save logs for analysis
- [ ] Take screenshots of web UIs
- [ ] Note any performance issues

### After Testing

- [ ] Stop all Docker services: `docker compose down`
- [ ] Clean up logs if needed: `rm -rf logs/*`
- [ ] Review test results
- [ ] File issues for any bugs found
- [ ] Update documentation if needed

---

## Troubleshooting

### Common Issues

#### Build Fails

```bash
# Solution: Install build dependencies
sudo apt-get install build-essential gcc make

# Rebuild
make clean && make
```

#### Docker Services Won't Start

```bash
# Solution: Check Docker is running
sudo systemctl status docker

# Check for port conflicts
sudo netstat -tulpn | grep -E '2222|2323|80|8080|554'

# View Docker logs
docker compose logs
```

#### Morphing Engine Fails

```bash
# Solution: Check file permissions
ls -la services/cowrie/etc/
chmod 755 services/cowrie/etc/

# Verify profile config exists
ls -la profiles.conf
```

#### Quorum Engine Finds No IPs

```bash
# Solution: Generate test logs first
echo "[$(date)] Connection from 192.168.1.100" >> services/cowrie/logs/cowrie.log

# Verify log directory exists and is readable
ls -la services/cowrie/logs/
```

---

## Testing Checklist

Use this checklist to verify complete testing:

### Component Tests
- [ ] Morphing engine compiles and runs
- [ ] Quorum engine compiles and runs
- [ ] Cowrie accepts SSH connections
- [ ] Cowrie accepts Telnet connections
- [ ] Router web UI loads
- [ ] Camera web UI loads
- [ ] RTSP server responds

### Integration Tests
- [ ] All services start together
- [ ] Morphing updates all services
- [ ] Quorum detects cross-service attacks
- [ ] Logs are generated correctly

### Security Tests
- [ ] Honeypot is network isolated
- [ ] No sensitive data leakage
- [ ] Containers not privileged
- [ ] Access controls working

### Performance Tests
- [ ] Handles multiple connections
- [ ] Morphing completes quickly
- [ ] Quorum processes logs efficiently
- [ ] No memory leaks

---

## Additional Resources

- **Documentation**: See `docs/` directory for design documents
- **Examples**: See `REPOSITORY_STATUS.md` for test outputs
- **Issues**: Report bugs on GitHub issue tracker
- **Support**: Educational project for learning purposes

---

## Contributing Tests

When adding new tests:

1. Create test scripts in `tests/` directory
2. Add test target to `Makefile`
3. Update this documentation
4. Ensure tests are reproducible
5. Document expected outputs

Example test script structure:
```bash
#!/bin/bash
# tests/test_morph.sh

echo "Testing morphing engine..."
./build/morph > /tmp/morph_output.txt
if grep -q "Successfully morphed" /tmp/morph_output.txt; then
    echo "✅ PASS: Morphing engine works"
    exit 0
else
    echo "❌ FAIL: Morphing engine failed"
    exit 1
fi
```

---

**For questions or issues, see README.md or file a GitHub issue.**

**License**: Educational purposes only
