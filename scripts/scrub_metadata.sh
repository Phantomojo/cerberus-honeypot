#!/bin/bash
# =============================================================================
# scrub_metadata.sh - DEEP DECEPTION MODE
# =============================================================================
# Purpose: Overwrites system files in the honeyfs to match the active profile's
# metadata (Arch, Kernel, CPU). This defeats APT-level fingerprinting.

HONEYFS_DIR="${1:-services/cowrie/honeyfs}"
PROFILE_NAME="$2"

if [ -z "$PROFILE_NAME" ]; then
    echo "Usage: $0 <honeyfs_dir> <profile_name>"
    exit 1
fi

# Load variables from profiles.conf for this profile
# Note: Simple parser, assumes standard ini-like format
parse_conf() {
    local section="$1"
    local key="$2"
    grep -A 20 "\[$section\]" profiles.conf | grep "^$key=" | cut -d'=' -f2-
}

KERNEL_VER=$(parse_conf "$PROFILE_NAME" "kernel_version")
ARCH=$(parse_conf "$PROFILE_NAME" "arch")
MAC_PREFIX=$(parse_conf "$PROFILE_NAME" "mac_prefix")
MEM_MB=$(parse_conf "$PROFILE_NAME" "memory_mb")
CPU_MHZ=$(parse_conf "$PROFILE_NAME" "cpu_mhz")

echo "[*] Scrubbing metadata for profile: $PROFILE_NAME"
echo "[*] Targeted Arch: $ARCH, Kernel: $KERNEL_VER"

# 1. SCRUB /proc/version
echo "Linux version $KERNEL_VER (gcc version 7.5.0) #1 SMP PREEMPT $(date '+%a %b %d %H:%M:%S UTC %Y')" > "$HONEYFS_DIR/proc/version"

# 2. SCRUB /proc/cpuinfo (Architecture Specific)
case $ARCH in
    armv7l|armv8l|aarch64)
        cat > "$HONEYFS_DIR/proc/cpuinfo" << EOF
processor       : 0
model name      : ARMv8 Processor rev 1 (v8l)
BogoMIPS        : $((CPU_MHZ / 2)).00
Features        : half thumb fastmult vfp edsp neon vfpv3 tls vfpv4
CPU implementer : 0x41
CPU architecture: 8
CPU variant     : 0x0
CPU part        : 0xd03
CPU revision    : 1
Hardware        : $PROFILE_NAME SoC
Revision        : 0000
Serial          : $(printf '%016x' $RANDOM$RANDOM)
EOF
        ;;
    mips)
        cat > "$HONEYFS_DIR/proc/cpuinfo" << EOF
system type     : $PROFILE_NAME SoC
machine         : Cerberus Node
processor       : 0
cpu model       : MIPS 24Kc V$(rand_between 1 9).0
BogoMIPS        : $((CPU_MHZ / 2)).05
wait instruction: yes
microsecond timers: yes
tlb_entries     : 32
extra interrupt vector: yes
hardware watchpoint: yes, count: 4
isa             : mips1 mips2 mips32r1 mips32r2
ASEs implemented: mips16
EOF
        ;;
    x86_64)
        cat > "$HONEYFS_DIR/proc/cpuinfo" << EOF
processor       : 0
vendor_id       : GenuineIntel
cpu family      : 6
model           : 142
model name      : Intel(R) Core(TM) i5-8250U CPU @ $(echo "scale=2; $CPU_MHZ/1000" | bc)GHz
stepping        : 10
microcode       : 0xca
cpu MHz         : $CPU_MHZ.000
cache size      : 6144 KB
physical id     : 0
siblings        : 1
core id         : 0
cpu cores       : 1
apicid          : 0
initial apicid  : 0
fpu             : yes
fpu_exception   : yes
cpuid level     : 22
wp              : yes
flags           : fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx pdpe1gb rdtscp lm constant_tsc art arch_perfmon pebs bts rep_good nopl xtopology nonstop_tsc cpuid aperfmperf pni pclmulqdq dtes64 monitor ds_cpl vmx est tm2 ssse3 sdbg fma cx16 xtpr pdcm pcid sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes xsave avx f16c rdrand lahf_lm abm 3dnowprefetch cpuid_fault epb invpcid_single pti ssbd ibrs ibpb stibp tpr_shadow vnmi flexpriority ept vpid ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid mpx rdseed adx smap clflushopt intel_pt xsaveopt xsavec xgetbv1 xsaves dtherm ida arat pln pts hwp hwp_notify hwp_act_window hwp_epp flush_l1d
bugs            : cpu_meltdown spectre_v1 spectre_v2 spec_store_bypass l1tf mds swapgs itlb_multihit srbds
bogomips        : $((CPU_MHZ * 2)).00
clflush size    : 64
cache_alignment : 64
address sizes   : 39 bits physical, 48 bits virtual
power management:
EOF
        ;;
esac

# 3. SCRUB /proc/meminfo
MEM_KB=$((MEM_MB*1024))
cat > "$HONEYFS_DIR/proc/meminfo" << EOF
MemTotal:        ${MEM_KB} kB
MemFree:         $((MEM_KB / 4)) kB
MemAvailable:    $((MEM_KB / 2)) kB
Buffers:         $((MEM_KB / 20)) kB
Cached:          $((MEM_KB / 10)) kB
SwapCached:      0 kB
EOF

# 4. SCRUB Host Leakage in /sys (Simulate NIC)
mkdir -p "$HONEYFS_DIR/sys/class/net/eth0"
echo "0x$(printf '%x' $RANDOM)" > "$HONEYFS_DIR/sys/class/net/eth0/ifindex"
echo "$MAC_PREFIX:$(printf '%02x:%02x:%02x' $((RANDOM%256)) $((RANDOM%256)) $((RANDOM%256)))" > "$HONEYFS_DIR/sys/class/net/eth0/address"

echo "[✓] Metadata scrubbing complete for $ARCH / $KERNEL_VER"
chmod +x "$HONEYFS_DIR/proc/version" "$HONEYFS_DIR/proc/cpuinfo" "$HONEYFS_DIR/proc/meminfo" 2>/dev/null || true
