#!/bin/bash
# =============================================================================
# setup_honeyfs.sh - RUBIK'S CUBE EDITION
# =============================================================================
#
# THE RUBIK'S CUBE VISION:
# ========================
# Every time this script runs, it creates a DIFFERENT but REALISTIC filesystem.
# Like a Rubik's cube - the pieces are always the same (files that should exist),
# but the arrangement is different (contents, timestamps, sizes, what's present).
#
# RANDOMNESS WITHIN RANDOMNESS:
# - Random number of temp files (3-15)
# - Random log entries with random IPs, PIDs, timestamps
# - Random file sizes (within realistic ranges)
# - Random timestamps (consistent with uptime)
# - Sometimes files exist, sometimes they don't
# - Random users logged in
# - Random network connections
# - Random process counts
#
# This makes fingerprinting nearly impossible because:
# - Session 1: 5 temp files, uptime 45 days, 3 users in wtmp
# - Session 2: 12 temp files, uptime 180 days, 7 users in wtmp
# - Session 3: 8 temp files, uptime 12 days, 1 user in wtmp
#
# Same honeypot, completely different "feel" each time!
#
# =============================================================================

set -e

# -----------------------------------------------------------------------------
# CONFIGURATION
# -----------------------------------------------------------------------------
HONEYFS_DIR="${1:-services/cowrie/honeyfs}"
PROFILE_TYPE="${2:-router}"  # "router" or "camera"
DEVICE_NAME="${3:-Generic_Device}"

# Colors (because pretty output helps debugging)
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# -----------------------------------------------------------------------------
# RANDOM HELPER FUNCTIONS
# -----------------------------------------------------------------------------
# These are the building blocks of our randomness

# Random number between min and max (inclusive)
rand_between() {
    local min=$1
    local max=$2
    echo $(( (RANDOM % (max - min + 1)) + min ))
}

# Random element from array
rand_element() {
    local arr=("$@")
    echo "${arr[RANDOM % ${#arr[@]}]}"
}

# Random boolean (returns 0 for true, 1 for false) with given probability
# Usage: rand_bool 70 means 70% chance of true
rand_bool() {
    local probability=${1:-50}
    [ $(rand_between 1 100) -le $probability ]
}

# Random IP address (looks realistic)
rand_ip() {
    local type=${1:-"local"}
    case $type in
        local)
            echo "192.168.$(rand_between 0 10).$(rand_between 1 254)"
            ;;
        wan)
            echo "$(rand_between 1 223).$(rand_between 0 255).$(rand_between 0 255).$(rand_between 1 254)"
            ;;
        attacker)
            # Common attacker IP ranges (for realism in logs)
            local ranges=("185.220" "45.33" "194.26" "89.248" "178.128" "104.248" "167.99" "206.189")
            local prefix=$(rand_element "${ranges[@]}")
            echo "${prefix}.$(rand_between 0 255).$(rand_between 1 254)"
            ;;
    esac
}

# Random MAC address with optional vendor prefix
rand_mac() {
    local prefix=${1:-""}
    if [ -z "$prefix" ]; then
        # Random vendor prefixes from real IoT devices
        local prefixes=("00:1A:2B" "44:19:B6" "A0:63:91" "14:CC:20" "00:1B:11" "D8:EB:97")
        prefix=$(rand_element "${prefixes[@]}")
    fi
    echo "${prefix}:$(printf '%02X' $((RANDOM%256))):$(printf '%02X' $((RANDOM%256))):$(printf '%02X' $((RANDOM%256)))"
}

# Random timestamp within last N days
rand_timestamp() {
    local max_days=${1:-30}
    local seconds_ago=$(rand_between 0 $((max_days * 86400)))
    date -d "@$(($(date +%s) - seconds_ago))" '+%b %d %H:%M:%S' 2>/dev/null || date '+%b %d %H:%M:%S'
}

# Random PID (realistic range for IoT)
rand_pid() {
    echo $(rand_between 100 32000)
}

# Random username from common IoT users
rand_user() {
    local users=("root" "admin" "user" "guest" "support" "service" "default")
    rand_element "${users[@]}"
}

# Random port number
rand_port() {
    local type=${1:-"high"}
    case $type in
        low) echo $(rand_between 1 1024) ;;
        high) echo $(rand_between 1025 65535) ;;
        common)
            local ports=(22 23 80 443 554 8080 8443 8000 8888)
            rand_element "${ports[@]}"
            ;;
    esac
}

# Generate random but realistic file size
rand_filesize() {
    local type=${1:-"small"}
    case $type in
        tiny) echo $(rand_between 10 500) ;;
        small) echo $(rand_between 500 5000) ;;
        medium) echo $(rand_between 5000 50000) ;;
        large) echo $(rand_between 50000 500000) ;;
        log) echo $(rand_between 1000 100000) ;;
    esac
}

# -----------------------------------------------------------------------------
# SYSTEM STATE VARIABLES (Randomized once per run for consistency)
# -----------------------------------------------------------------------------
# These ensure internal consistency - if uptime is 45 days, logs should match

# Random uptime between 1 and 365 days (in seconds)
UPTIME_DAYS=$(rand_between 1 365)
UPTIME_SECONDS=$((UPTIME_DAYS * 86400 + RANDOM % 86400))
BOOT_TIME=$(($(date +%s) - UPTIME_SECONDS))

# Random memory (32MB to 256MB for IoT)
TOTAL_MEM_KB=$(($(rand_between 32 256) * 1024))
USED_MEM_KB=$((TOTAL_MEM_KB * $(rand_between 30 80) / 100))
FREE_MEM_KB=$((TOTAL_MEM_KB - USED_MEM_KB))

# Random number of processes (10-60 for IoT)
NUM_PROCESSES=$(rand_between 10 60)

# Random number of users who have logged in
NUM_LOGIN_HISTORY=$(rand_between 3 20)

# Random load average
LOAD_1=$(printf "0.%02d" $(rand_between 1 99))
LOAD_5=$(printf "0.%02d" $(rand_between 1 50))
LOAD_15=$(printf "0.%02d" $(rand_between 1 30))

echo -e "${BLUE}=== Honeyfs Setup (Rubik's Cube Edition) ===${NC}"
echo -e "Device: ${DEVICE_NAME} (${PROFILE_TYPE})"
echo -e "Randomized State:"
echo -e "  Uptime: ${UPTIME_DAYS} days"
echo -e "  Memory: $((TOTAL_MEM_KB/1024))MB total, $((USED_MEM_KB/1024))MB used"
echo -e "  Processes: ${NUM_PROCESSES}"
echo -e "  Login history entries: ${NUM_LOGIN_HISTORY}"
echo ""

# -----------------------------------------------------------------------------
# STEP 1: CREATE DIRECTORY STRUCTURE
# -----------------------------------------------------------------------------
echo -e "${YELLOW}[1/10] Creating directory structure...${NC}"

# Clear existing honeyfs (fresh start each time - part of the randomness!)
rm -rf "${HONEYFS_DIR:?}"/*

# Standard directories (always exist)
mkdir -p "${HONEYFS_DIR}"/{bin,sbin,usr/bin,usr/sbin,usr/lib}
mkdir -p "${HONEYFS_DIR}"/etc/{config,init.d,network,ssl/certs}
mkdir -p "${HONEYFS_DIR}"/var/{log,run,tmp,cache,lib/misc}
mkdir -p "${HONEYFS_DIR}"/{proc,sys,dev,tmp,root,home,mnt,opt}
mkdir -p "${HONEYFS_DIR}"/lib/{modules,firmware}

# Random: Sometimes these exist, sometimes not (50% chance each)
rand_bool 50 && mkdir -p "${HONEYFS_DIR}/media"
rand_bool 50 && mkdir -p "${HONEYFS_DIR}/srv"
rand_bool 30 && mkdir -p "${HONEYFS_DIR}/boot"
rand_bool 40 && mkdir -p "${HONEYFS_DIR}/usr/local/bin"

echo -e "${GREEN}✓ Directory structure created${NC}"

# -----------------------------------------------------------------------------
# STEP 2: CREATE USER FILES (with randomization)
# -----------------------------------------------------------------------------
echo -e "${YELLOW}[2/10] Creating user files...${NC}"

# /etc/passwd - Always has these core users, but UIDs vary slightly
ROOT_UID=0
ADMIN_UID=$(rand_between 1000 1010)
NOBODY_UID=$(rand_between 65530 65535)

cat > "${HONEYFS_DIR}/etc/passwd" << EOF
root:x:${ROOT_UID}:0:root:/root:/bin/sh
daemon:x:1:1:daemon:/usr/sbin:/bin/false
bin:x:2:2:bin:/bin:/bin/false
sys:x:3:3:sys:/dev:/bin/false
nobody:x:${NOBODY_UID}:${NOBODY_UID}:nobody:/nonexistent:/bin/false
admin:x:${ADMIN_UID}:${ADMIN_UID}:Administrator:/home/admin:/bin/sh
EOF

# Random: Add extra users based on device type (30-70% chance each)
if [ "$PROFILE_TYPE" = "camera" ]; then
    rand_bool 70 && echo "onvif:x:$(rand_between 1001 1005):$(rand_between 1001 1005):ONVIF Service:/var/run/onvif:/bin/false" >> "${HONEYFS_DIR}/etc/passwd"
    rand_bool 60 && echo "rtsp:x:$(rand_between 1006 1010):$(rand_between 1006 1010):RTSP Service:/var/run/rtsp:/bin/false" >> "${HONEYFS_DIR}/etc/passwd"
    rand_bool 40 && echo "stream:x:$(rand_between 1011 1015):$(rand_between 1011 1015):Stream User:/tmp:/bin/false" >> "${HONEYFS_DIR}/etc/passwd"
else
    rand_bool 60 && echo "dnsmasq:x:$(rand_between 1001 1005):$(rand_between 1001 1005):dnsmasq daemon:/var/lib/misc:/bin/false" >> "${HONEYFS_DIR}/etc/passwd"
    rand_bool 50 && echo "www-data:x:33:33:www-data:/var/www:/bin/false" >> "${HONEYFS_DIR}/etc/passwd"
    rand_bool 40 && echo "ftp:x:$(rand_between 1006 1010):$(rand_between 1006 1010):FTP User:/var/ftp:/bin/false" >> "${HONEYFS_DIR}/etc/passwd"
    rand_bool 30 && echo "ntp:x:$(rand_between 1011 1015):$(rand_between 1011 1015):NTP daemon:/var/lib/ntp:/bin/false" >> "${HONEYFS_DIR}/etc/passwd"
fi

# /etc/group
cat > "${HONEYFS_DIR}/etc/group" << EOF
root:x:0:
daemon:x:1:
bin:x:2:
sys:x:3:
adm:x:4:
tty:x:5:
disk:x:6:
wheel:x:10:root,admin
nobody:x:${NOBODY_UID}:
admin:x:${ADMIN_UID}:
EOF

# /etc/shadow - Hashes look random but are actually static (can't login anyway)
SHADOW_HASH='$6$rounds=5000$'$(head -c 8 /dev/urandom | base64 | tr -d '/+=' | head -c 8)'$'$(head -c 32 /dev/urandom | base64 | tr -d '/+=' | head -c 43)
cat > "${HONEYFS_DIR}/etc/shadow" << EOF
root:${SHADOW_HASH}:$(rand_between 18000 19000):0:99999:7:::
daemon:*:$(rand_between 18000 19000):0:99999:7:::
bin:*:$(rand_between 18000 19000):0:99999:7:::
nobody:*:$(rand_between 18000 19000):0:99999:7:::
admin:${SHADOW_HASH}:$(rand_between 18000 19000):0:99999:7:::
EOF

echo -e "${GREEN}✓ User files created (with $(grep -c ':' ${HONEYFS_DIR}/etc/passwd) users)${NC}"

# -----------------------------------------------------------------------------
# STEP 3: SYSTEM CONFIGURATION FILES
# -----------------------------------------------------------------------------
echo -e "${YELLOW}[3/10] Creating system configuration...${NC}"

# /etc/hostname
echo "${DEVICE_NAME}" > "${HONEYFS_DIR}/etc/hostname"

# /etc/hosts - Random local network IPs
LOCAL_IP=$(rand_ip local)
cat > "${HONEYFS_DIR}/etc/hosts" << EOF
127.0.0.1       localhost
127.0.1.1       ${DEVICE_NAME}
${LOCAL_IP}     ${DEVICE_NAME}
::1             localhost ip6-localhost ip6-loopback
EOF

# Random: Sometimes there are extra hosts entries (like printers, NAS, etc.)
if rand_bool 40; then
    echo "$(rand_ip local)    nas" >> "${HONEYFS_DIR}/etc/hosts"
fi
if rand_bool 30; then
    echo "$(rand_ip local)    printer" >> "${HONEYFS_DIR}/etc/hosts"
fi

# /etc/resolv.conf - Random DNS servers
DNS_OPTIONS=("8.8.8.8 8.8.4.4" "1.1.1.1 1.0.0.1" "208.67.222.222 208.67.220.220" "9.9.9.9 149.112.112.112")
CHOSEN_DNS=$(rand_element "${DNS_OPTIONS[@]}")
cat > "${HONEYFS_DIR}/etc/resolv.conf" << EOF
nameserver ${CHOSEN_DNS%% *}
nameserver ${CHOSEN_DNS##* }
EOF

# /etc/os-release - Varies by device type
if [ "$PROFILE_TYPE" = "camera" ]; then
    OS_NAMES=("Embedded Linux" "Camera OS" "DVR Linux" "IP-Cam Linux")
    OS_VERSIONS=("2.0" "2.5" "3.0" "3.1" "3.2")
else
    OS_NAMES=("OpenWrt" "DD-WRT" "Tomato" "LEDE" "RouterOS")
    OS_VERSIONS=("15.05" "17.01" "18.06" "19.07" "21.02")
fi
OS_NAME=$(rand_element "${OS_NAMES[@]}")
OS_VER=$(rand_element "${OS_VERSIONS[@]}")

cat > "${HONEYFS_DIR}/etc/os-release" << EOF
NAME="${OS_NAME}"
VERSION="${OS_VER}"
ID=$(echo "${OS_NAME}" | tr '[:upper:]' '[:lower:]' | tr -d ' -')
VERSION_ID=${OS_VER}
PRETTY_NAME="${OS_NAME} ${OS_VER}"
EOF

# /etc/issue and /etc/issue.net - Login banners
cat > "${HONEYFS_DIR}/etc/issue" << EOF

${DEVICE_NAME}
${OS_NAME} ${OS_VER}
Kernel \r on \m

EOF
cp "${HONEYFS_DIR}/etc/issue" "${HONEYFS_DIR}/etc/issue.net"

# /etc/motd - Message of the day (sometimes exists, sometimes doesn't)
if rand_bool 60; then
    MOTD_MSGS=(
        "Welcome to ${DEVICE_NAME}"
        "Authorized users only."
        "${OS_NAME} - ${DEVICE_NAME}"
        "Type 'help' for available commands."
        ""
    )
    echo "$(rand_element "${MOTD_MSGS[@]}")" > "${HONEYFS_DIR}/etc/motd"
fi

# /etc/fstab
cat > "${HONEYFS_DIR}/etc/fstab" << 'EOF'
/dev/root       /               jffs2   rw,noatime      0 1
proc            /proc           proc    defaults        0 0
sysfs           /sys            sysfs   defaults        0 0
tmpfs           /tmp            tmpfs   defaults,noatime 0 0
tmpfs           /var            tmpfs   defaults,noatime 0 0
EOF

# Random: Sometimes there's a USB mount
if rand_bool 20; then
    echo "/dev/sda1       /mnt/usb        vfat    defaults,noauto 0 0" >> "${HONEYFS_DIR}/etc/fstab"
fi

echo -e "${GREEN}✓ System configuration created${NC}"

# -----------------------------------------------------------------------------
# STEP 4: NETWORK CONFIGURATION
# -----------------------------------------------------------------------------
echo -e "${YELLOW}[4/10] Creating network configuration...${NC}"

# Random network parameters
LAN_IP="192.168.$(rand_between 0 10).1"
LAN_NETMASK="255.255.255.0"
WAN_PROTO=$(rand_element "dhcp" "dhcp" "dhcp" "pppoe" "static")  # DHCP most common

# /etc/network/interfaces
cat > "${HONEYFS_DIR}/etc/network/interfaces" << EOF
auto lo
iface lo inet loopback

auto eth0
iface eth0 inet ${WAN_PROTO}
EOF

# Router-specific configs
if [ "$PROFILE_TYPE" = "router" ]; then
    mkdir -p "${HONEYFS_DIR}/etc/config"

    # OpenWrt-style network config
    cat > "${HONEYFS_DIR}/etc/config/network" << EOF
config interface 'loopback'
    option ifname 'lo'
    option proto 'static'
    option ipaddr '127.0.0.1'
    option netmask '255.0.0.0'

config interface 'lan'
    option type 'bridge'
    option ifname 'eth0'
    option proto 'static'
    option ipaddr '${LAN_IP}'
    option netmask '${LAN_NETMASK}'

config interface 'wan'
    option ifname 'eth1'
    option proto '${WAN_PROTO}'
EOF

    # Firewall config (sometimes more rules, sometimes fewer)
    NUM_FW_RULES=$(rand_between 3 8)
    cat > "${HONEYFS_DIR}/etc/config/firewall" << EOF
config defaults
    option syn_flood '1'
    option input 'ACCEPT'
    option output 'ACCEPT'
    option forward 'REJECT'

config zone
    option name 'lan'
    option input 'ACCEPT'
    option output 'ACCEPT'
    option forward 'ACCEPT'
    option network 'lan'

config zone
    option name 'wan'
    option input 'REJECT'
    option output 'ACCEPT'
    option forward 'REJECT'
    option masq '1'
EOF

    # Random: Add some port forwards (30% chance each)
    if rand_bool 30; then
        cat >> "${HONEYFS_DIR}/etc/config/firewall" << EOF

config redirect
    option name 'SSH'
    option src 'wan'
    option dest 'lan'
    option proto 'tcp'
    option src_dport '$(rand_between 2200 2299)'
    option dest_ip '192.168.$(rand_between 0 10).$(rand_between 100 200)'
    option dest_port '22'
EOF
    fi

    # DHCP config
    DHCP_START=$(rand_between 100 150)
    DHCP_LIMIT=$(rand_between 50 100)
    cat > "${HONEYFS_DIR}/etc/config/dhcp" << EOF
config dnsmasq
    option domainneeded '1'
    option localise_queries '1'
    option local '/lan/'
    option domain 'lan'
    option authoritative '1'
    option leasefile '/tmp/dhcp.leases'

config dhcp 'lan'
    option interface 'lan'
    option start '${DHCP_START}'
    option limit '${DHCP_LIMIT}'
    option leasetime '12h'
EOF

    # Wireless config (random SSID)
    SSID_WORDS=("Home" "NET" "WIFI" "Network" "Wireless" "MyNet" "Router" "Guest")
    SSID="${SSID_WORDS[RANDOM % ${#SSID_WORDS[@]}]}$(rand_between 100 999)"
    CHANNEL=$(rand_between 1 11)

    cat > "${HONEYFS_DIR}/etc/config/wireless" << EOF
config wifi-device 'radio0'
    option type 'mac80211'
    option channel '${CHANNEL}'
    option hwmode '11g'
    option disabled '0'

config wifi-iface 'default_radio0'
    option device 'radio0'
    option network 'lan'
    option mode 'ap'
    option ssid '${SSID}'
    option encryption 'psk2'
    option key '********'
EOF

    # DHCP leases file (random active leases)
    NUM_LEASES=$(rand_between 0 15)
    : > "${HONEYFS_DIR}/var/lib/misc/dhcp.leases"
    for ((i=0; i<NUM_LEASES; i++)); do
        LEASE_TIME=$(($(date +%s) + RANDOM % 43200))
        LEASE_MAC=$(rand_mac)
        LEASE_IP="192.168.$(rand_between 0 10).$(rand_between 100 250)"
        LEASE_HOST=$(rand_element "android" "iPhone" "laptop" "PC" "tablet" "SmartTV" "printer" "*")
        echo "${LEASE_TIME} ${LEASE_MAC} ${LEASE_IP} ${LEASE_HOST} *" >> "${HONEYFS_DIR}/var/lib/misc/dhcp.leases"
    done
fi

# Camera-specific configs
if [ "$PROFILE_TYPE" = "camera" ]; then
    mkdir -p "${HONEYFS_DIR}/etc/camera"

    RESOLUTIONS=("1920x1080" "1280x720" "2560x1440" "3840x2160" "640x480")
    FRAMERATES=(15 20 25 30)
    BITRATES=(2048 4096 6144 8192)

    cat > "${HONEYFS_DIR}/etc/camera/config.ini" << EOF
[Network]
IPAddress=${LOCAL_IP}
SubnetMask=255.255.255.0
Gateway=192.168.$(rand_between 0 10).1
DHCP=$(rand_between 0 1)
HTTPPort=$(rand_element 80 8080 8000)
RTSPPort=554

[Video]
Resolution=$(rand_element "${RESOLUTIONS[@]}")
Framerate=$(rand_element "${FRAMERATES[@]}")
Bitrate=$(rand_element "${BITRATES[@]}")
Codec=$(rand_element "H264" "H265" "MJPEG")
Quality=$(rand_between 1 10)

[Recording]
Enable=$(rand_between 0 1)
StoragePath=/mnt/sd
PreRecord=$(rand_between 3 10)
PostRecord=$(rand_between 5 30)

[Motion]
Enable=1
Sensitivity=$(rand_between 1 100)
EOF
fi

echo -e "${GREEN}✓ Network configuration created${NC}"

# -----------------------------------------------------------------------------
# STEP 5: /proc FILESYSTEM (Fake Kernel Info)
# -----------------------------------------------------------------------------
echo -e "${YELLOW}[5/10] Creating /proc filesystem...${NC}"

# /proc/version - Kernel version (randomized build date)
KERNEL_VERSIONS=("3.10.49" "3.4.35" "3.0.8" "2.6.36" "4.4.60" "4.9.37" "3.18.20")
KERNEL_VER=$(rand_element "${KERNEL_VERSIONS[@]}")
BUILD_DATE=$(date -d "@${BOOT_TIME}" '+%a %b %d %H:%M:%S UTC %Y' 2>/dev/null || date '+%a %b %d %H:%M:%S UTC %Y')
GCC_VERS=("4.6.3" "4.8.3" "5.4.0" "6.3.0")
GCC_VER=$(rand_element "${GCC_VERS[@]}")

cat > "${HONEYFS_DIR}/proc/version" << EOF
Linux version ${KERNEL_VER} (buildroot@builder) (gcc version ${GCC_VER}) #1 SMP PREEMPT ${BUILD_DATE}
EOF

# /proc/cpuinfo - Varies by device type
if [ "$PROFILE_TYPE" = "camera" ]; then
    CPU_MODELS=("Hi3516CV300" "Hi3516EV100" "Hi3518EV200" "GM8136S" "SSC335")
    CPU_MODEL=$(rand_element "${CPU_MODELS[@]}")
    BOGOMIPS=$(printf "%.2f" $(echo "scale=2; $(rand_between 500 1500) / 100" | bc 2>/dev/null || echo "6.00"))

    cat > "${HONEYFS_DIR}/proc/cpuinfo" << EOF
processor       : 0
model name      : ARMv7 Processor rev $(rand_between 1 10) (v7l)
BogoMIPS        : ${BOGOMIPS}
Features        : half thumb fastmult vfp edsp neon vfpv3 tls vfpv4
CPU implementer : 0x41
CPU architecture: 7
CPU variant     : 0x$(printf '%x' $(rand_between 0 3))
CPU part        : 0xc0$(rand_between 5 9)
CPU revision    : $(rand_between 1 10)
Hardware        : ${CPU_MODEL}
Revision        : 000$(rand_between 0 9)
Serial          : $(printf '%016x' $RANDOM$RANDOM$RANDOM$RANDOM)
EOF
else
    CPU_MODELS=("MT7621" "MT7620" "AR9344" "QCA9558" "BCM4708" "IPQ4019")
    CPU_MODEL=$(rand_element "${CPU_MODELS[@]}")

    cat > "${HONEYFS_DIR}/proc/cpuinfo" << EOF
system type     : MediaTek ${CPU_MODEL} SoC
machine         : Router Board
processor       : 0
cpu model       : MIPS 1004Kc V$(rand_between 1 3).$(rand_between 10 20)
BogoMIPS        : $(rand_between 400 900).$(rand_between 10 99)
wait instruction: yes
microsecond timers: yes
tlb_entries     : $(rand_between 16 64)
extra interrupt vector: yes
hardware watchpoint: yes, count: $(rand_between 2 8)
isa             : mips1 mips2 mips32r1 mips32r2
ASEs implemented: mips16 dsp mt
EOF
fi

# /proc/meminfo - Uses our randomized memory values
BUFFERS=$((FREE_MEM_KB * $(rand_between 5 15) / 100))
CACHED=$((FREE_MEM_KB * $(rand_between 10 30) / 100))
cat > "${HONEYFS_DIR}/proc/meminfo" << EOF
MemTotal:        ${TOTAL_MEM_KB} kB
MemFree:         ${FREE_MEM_KB} kB
MemAvailable:    $((FREE_MEM_KB + BUFFERS + CACHED)) kB
Buffers:         ${BUFFERS} kB
Cached:          ${CACHED} kB
SwapCached:      0 kB
Active:          $((USED_MEM_KB * 60 / 100)) kB
Inactive:        $((USED_MEM_KB * 40 / 100)) kB
SwapTotal:       0 kB
SwapFree:        0 kB
Dirty:           $(rand_between 0 100) kB
Writeback:       0 kB
AnonPages:       $((USED_MEM_KB * 30 / 100)) kB
Mapped:          $((USED_MEM_KB * 10 / 100)) kB
Slab:            $(rand_between 1000 5000) kB
EOF

# /proc/uptime - Uses our randomized uptime
IDLE_SECONDS=$((UPTIME_SECONDS * $(rand_between 70 95) / 100))
echo "${UPTIME_SECONDS}.$((RANDOM % 100)) ${IDLE_SECONDS}.$((RANDOM % 100))" > "${HONEYFS_DIR}/proc/uptime"

# /proc/loadavg
echo "${LOAD_1} ${LOAD_5} ${LOAD_15} 1/${NUM_PROCESSES} $(rand_pid)" > "${HONEYFS_DIR}/proc/loadavg"

# /proc/mounts
cat > "${HONEYFS_DIR}/proc/mounts" << EOF
rootfs / rootfs rw 0 0
/dev/root / jffs2 rw,noatime 0 0
proc /proc proc rw,nosuid,nodev,noexec,relatime 0 0
sysfs /sys sysfs rw,nosuid,nodev,noexec,relatime 0 0
tmpfs /tmp tmpfs rw,nosuid,nodev,noatime 0 0
devpts /dev/pts devpts rw,nosuid,noexec,relatime,mode=600 0 0
EOF

# Random: Sometimes USB or SD card is mounted
rand_bool 15 && echo "/dev/sda1 /mnt/usb vfat rw,noatime 0 0" >> "${HONEYFS_DIR}/proc/mounts"
rand_bool 20 && echo "/dev/mmcblk0p1 /mnt/sd vfat rw,noatime 0 0" >> "${HONEYFS_DIR}/proc/mounts"

# /proc/net/dev - Network stats
cat > "${HONEYFS_DIR}/proc/net/dev" << EOF
Inter-|   Receive                                                |  Transmit
 face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
    lo:$(rand_between 10000 99999)    $(rand_between 100 999)    0    0    0     0          0         0 $(rand_between 10000 99999)    $(rand_between 100 999)    0    0    0     0       0          0
  eth0:$(rand_between 1000000 99999999) $(rand_between 10000 99999)    0    0    0     0          0         0 $(rand_between 100000 9999999) $(rand_between 1000 99999)    0    0    0     0
EOF
