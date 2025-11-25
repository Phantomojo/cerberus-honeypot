# Cowrie Customization Plan - Making Morphing Work

## Current Problem
✅ SSH connection works
✅ Configuration file has correct values (hostname, kernel_version, arch)
❌ Shell still shows default Debian system (svr04, x86_64, Debian 3.2.0)

## Root Cause Analysis

Cowrie uses:
1. **honeyfs** - Fake filesystem that attackers see
2. **command.py** - Command emulation handlers
3. **cowrie.cfg** - Configuration (may only affect logging)

The [honeypot] section in config may NOT automatically update the shell responses.

## Solutions to Implement

### Option 1: Mount Custom honeyfs Directory
- Create custom honeyfs with modified /etc/hostname
- Mount it in docker-compose.yml
- Modify /proc/version or create custom files

### Option 2: Modify Command Handlers
- Find command.py in Cowrie source
- Modify uname and hostname command handlers
- Use config values to return custom responses

### Option 3: Use Cowrie Plugins/Extensions
- Check if Cowrie supports plugins for custom responses
- Create custom plugin to override command outputs

### Option 4: Modify honeyfs Files Directly
- Access Cowrie container's honeyfs directory
- Modify /etc/hostname, /proc/version, etc.
- Restart Cowrie

## Next Steps

1. Check Cowrie container structure
2. Locate honeyfs directory
3. Modify files or mount custom directory
4. Test hostname and uname commands
5. Verify morphing works

## Files to Modify

- /etc/hostname → Should contain: Netgear_R7000
- /proc/version → Should show: Linux version 2.6.36.4brcmarm
- Command handlers → Should use config values

