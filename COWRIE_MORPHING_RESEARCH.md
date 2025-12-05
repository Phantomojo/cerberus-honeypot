# Cowrie Morphing Research & Implementation Plan

## Problem Statement

✅ **Working:**
- SSH connection successful
- Configuration file (`cowrie.cfg.local`) has correct morphing values:
  - `hostname = Netgear_R7000`
  - `kernel_version = 2.6.36.4brcmarm`
  - `arch = armv7l`

❌ **Not Working:**
- Shell still shows default Debian system:
  - `hostname` → `svr04` (should be `Netgear_R7000`)
  - `uname -a` → `Linux svr04 3.2.0-4-amd64 #1 SMP Debian 3.2.68-1+deb7u1 x86_64` (should show Netgear profile)

## Root Cause Analysis

Based on research, Cowrie uses three main components:

1. **honeyfs (Fake Filesystem)**
   - Location: `/cowrie/cowrie-git/share/cowrie/honeyfs/`
   - Contains files like `/etc/hostname`, `/proc/version`, `/etc/issue`, etc.
   - These files are what attackers see when they run commands

2. **command.py (Command Emulation)**
   - Commands like `uname`, `hostname`, `cat /proc/version` are emulated
   - Handlers return hardcoded or filesystem-based responses
   - May not automatically read from `[honeypot]` config section

3. **cowrie.cfg Configuration**
   - The `[honeypot]` section may only affect **logging**, not shell responses
   - Configuration values might not be automatically applied to command outputs

## Research Findings

### Key Insights:
- Cowrie is a **medium-interaction honeypot** - it emulates commands, not a full OS
- The fake filesystem (honeyfs) is static by default
- Command responses are hardcoded in Python handlers
- Configuration `[honeypot]` section may be for logging metadata only

### Documentation References:
- Official docs: https://docs.cowrie.org
- GitHub: https://github.com/cowrie/cowrie
- honeyfs location: `/cowrie/cowrie-git/share/cowrie/honeyfs/`

## Solution Options

### Option 1: Mount Custom honeyfs Directory (Recommended)
**Pros:** Clean, maintainable, works with morphing
**Cons:** Requires creating custom honeyfs structure

**Steps:**
1. Create custom honeyfs directory structure
2. Modify `/etc/hostname` file
3. Modify `/proc/version` or create custom version file
4. Mount in docker-compose.yml
5. Update morphing engine to modify these files

### Option 2: Modify Command Handlers
**Pros:** Dynamic responses based on config
**Cons:** Requires modifying Cowrie source code

**Steps:**
1. Locate `command.py` in Cowrie source
2. Modify `uname` and `hostname` handlers
3. Read values from `[honeypot]` config section
4. Return custom responses based on current profile

### Option 3: Use Cowrie's Built-in Customization
**Pros:** Uses official Cowrie features
**Cons:** May have limitations

**Steps:**
1. Check if Cowrie supports dynamic honeyfs
2. Use environment variables or config hooks
3. Modify files at runtime

### Option 4: Direct honeyfs Modification
**Pros:** Quick fix
**Cons:** Not persistent, requires container access

**Steps:**
1. Access Cowrie container
2. Modify honeyfs files directly
3. Restart Cowrie
4. (Not ideal for morphing - files would need constant updates)

## Recommended Implementation: Option 1 + Option 2 Hybrid

### Phase 1: Custom honeyfs Mount
1. Create `services/cowrie/honeyfs/` directory
2. Structure it like real filesystem:
   ```
   services/cowrie/honeyfs/
   ├── etc/
   │   ├── hostname
   │   ├── issue
   │   └── issue.net
   └── proc/
       └── version
   ```
3. Mount in docker-compose.yml:
   ```yaml
   volumes:
     - ../services/cowrie/honeyfs:/cowrie/cowrie-git/share/cowrie/honeyfs:ro
   ```

### Phase 2: Update Morphing Engine
Modify `morph_cowrie_banners()` in `src/morph/morph.c` to:
1. Update `services/cowrie/honeyfs/etc/hostname` with profile name
2. Update `services/cowrie/honeyfs/proc/version` with kernel version
3. Update any other relevant files

### Phase 3: Test and Verify
1. Run morphing engine
2. Restart Cowrie
3. Connect via SSH
4. Verify `hostname` and `uname -a` show correct values

## Files to Create/Modify

### New Files:
- `services/cowrie/honeyfs/etc/hostname`
- `services/cowrie/honeyfs/proc/version`
- `services/cowrie/honeyfs/etc/issue` (optional)
- `services/cowrie/honeyfs/etc/issue.net` (optional)

### Files to Modify:
- `docker/docker-compose.yml` - Add honeyfs mount
- `src/morph/morph.c` - Update `morph_cowrie_banners()` function
- `services/cowrie/etc/cowrie.cfg.local` - Already correct

## Testing Checklist

- [ ] Custom honeyfs directory created
- [ ] Files mounted in docker-compose
- [ ] Morphing engine updates honeyfs files
- [ ] Cowrie restarts after morph
- [ ] SSH connection shows correct hostname
- [ ] `uname -a` shows correct kernel/arch
- [ ] Morphing between profiles works
- [ ] All 6 profiles tested

## Next Steps

1. **Immediate:** Create custom honeyfs structure
2. **Short-term:** Update morphing engine to modify honeyfs files
3. **Testing:** Verify morphing works end-to-end
4. **Documentation:** Update README with morphing details

## References

- Cowrie Documentation: https://docs.cowrie.org/en/stable/
- Cowrie GitHub: https://github.com/cowrie/cowrie
- Honeypot Customization: Various blog posts and tutorials
- Docker Cowrie Image: https://hub.docker.com/r/cowrie/cowrie

---

**Status:** Research complete. Ready for implementation.
