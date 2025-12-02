/**
 * state_engine.c - The Brain of CERBERUS Honeypot
 * ============================================================================
 * 
 * THE RUBIK'S CUBE VISION:
 * ------------------------
 * This is the "single source of truth" for the entire fake system.
 * Instead of generating random values independently (which don't correlate),
 * we maintain ONE coherent system state, and ALL outputs derive from it.
 * 
 * KEY PRINCIPLES:
 * 1. Single Source of Truth - All data comes from system_state_t
 * 2. Correlation - Derived values are CALCULATED, not random
 * 3. Reproducibility - Same seed = same state (for debugging)
 * 4. Reactivity - State updates when attacker does something
 * 
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>

#include "state_engine.h"
#include "utils.h"

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static system_state_t g_state;
static bool g_state_initialized = false;

/* ============================================================================
 * PRNG - Seeded Random Number Generator
 * ============================================================================
 * 
 * WHY WE NEED THIS:
 * Using a seeded PRNG means we can reproduce EXACT states.
 * Same seed = same "random" numbers = same fake system.
 * This is crucial for debugging and for the Rubik's Cube effect.
 */

static uint32_t prng_state = 0;

static void prng_seed(uint32_t seed) {
    prng_state = seed ? seed : (uint32_t)time(NULL);
}

/* xorshift32 - Fast, decent quality PRNG */
static uint32_t prng_next(void) {
    uint32_t x = prng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    prng_state = x;
    return x;
}

uint32_t state_rand(system_state_t* state) {
    (void)state; /* Could use per-state PRNG in future */
    return prng_next();
}

uint32_t state_rand_between(system_state_t* state, uint32_t min, uint32_t max) {
    if (min >= max) return min;
    return min + (state_rand(state) % (max - min + 1));
}

/* Generate random IP address */
void state_rand_ip(system_state_t* state, char* buf, size_t size, const char* type) {
    if (!buf || size < 16) return;
    
    if (!type || strcmp(type, "local") == 0 || strcmp(type, "lan") == 0) {
        snprintf(buf, size, "192.168.%u.%u",
                 state_rand_between(state, 0, 10),
                 state_rand_between(state, 1, 254));
    } else if (strcmp(type, "wan") == 0 || strcmp(type, "public") == 0) {
        snprintf(buf, size, "%u.%u.%u.%u",
                 state_rand_between(state, 1, 223),
                 state_rand_between(state, 0, 255),
                 state_rand_between(state, 0, 255),
                 state_rand_between(state, 1, 254));
    } else if (strcmp(type, "attacker") == 0) {
        /* Common attacker IP ranges for realistic logs */
        const char* prefixes[] = {"185.220", "45.33", "194.26", "89.248", "178.128"};
        int idx = state_rand_between(state, 0, 4);
        snprintf(buf, size, "%s.%u.%u",
                 prefixes[idx],
                 state_rand_between(state, 0, 255),
                 state_rand_between(state, 1, 254));
    } else {
        snprintf(buf, size, "192.168.1.%u", state_rand_between(state, 1, 254));
    }
}

/* Generate random MAC address */
void state_rand_mac(system_state_t* state, char* buf, size_t size, const char* prefix) {
    if (!buf || size < 18) return;
    
    if (prefix && strlen(prefix) >= 8) {
        snprintf(buf, size, "%s:%02X:%02X:%02X",
                 prefix,
                 state_rand_between(state, 0, 255),
                 state_rand_between(state, 0, 255),
                 state_rand_between(state, 0, 255));
    } else {
        /* Generate fully random MAC (locally administered) */
        snprintf(buf, size, "%02X:%02X:%02X:%02X:%02X:%02X",
                 (state_rand_between(state, 0, 255) & 0xFE) | 0x02, /* Locally administered */
                 state_rand_between(state, 0, 255),
                 state_rand_between(state, 0, 255),
                 state_rand_between(state, 0, 255),
                 state_rand_between(state, 0, 255),
                 state_rand_between(state, 0, 255));
    }
}

/* ============================================================================
 * BUILT-IN DEVICE PROFILES
 * ============================================================================
 * 
 * These are based on REAL devices. The values come from actual IoT devices
 * captured in the wild. Using realistic values is crucial for evasion.
 */

static const device_profile_t builtin_profiles[] = {
    {
        .name = "TP-Link_Archer_C7",
        .vendor = "TP-Link",
        .model = "Archer C7 v4",
        .type = DEVICE_TYPE_ROUTER,
        .architecture = ARCH_MIPS,
        .cpu_model = "MIPS 74Kc V5.0",
        .cpu_mhz = 720,
        .cpu_cores = 1,
        .total_ram_kb = 128 * 1024,
        .total_flash_kb = 16 * 1024,
        .bogomips = 36168,
        .kernel_version = "3.10.49",
        .os_name = "OpenWrt",
        .os_version = "18.06.4",
        .busybox_version = "1.24.1",
        .ssh_banner = "SSH-2.0-dropbear_2017.75",
        .telnet_banner = "TP-Link Archer C7 v4\r\nLogin: ",
        .mac_prefix = "14:CC:20"
    },
    {
        .name = "Hikvision_DS-2CD2",
        .vendor = "Hikvision",
        .model = "DS-2CD2032-I",
        .type = DEVICE_TYPE_CAMERA,
        .architecture = ARCH_ARMV7,
        .cpu_model = "ARMv7 Processor rev 5 (v7l)",
        .cpu_mhz = 600,
        .cpu_cores = 1,
        .total_ram_kb = 64 * 1024,
        .total_flash_kb = 8 * 1024,
        .bogomips = 600,
        .kernel_version = "3.0.8",
        .os_name = "Embedded Linux",
        .os_version = "2.3",
        .busybox_version = "1.20.2",
        .ssh_banner = "SSH-2.0-OpenSSH_5.8p1",
        .telnet_banner = "Hikvision Digital Technology\r\nLogin: ",
        .mac_prefix = "44:19:B6"
    },
    {
        .name = "Netgear_R7000",
        .vendor = "NETGEAR",
        .model = "R7000 Nighthawk",
        .type = DEVICE_TYPE_ROUTER,
        .architecture = ARCH_ARMV7,
        .cpu_model = "ARMv7 Processor rev 0 (v7l)",
        .cpu_mhz = 1000,
        .cpu_cores = 2,
        .total_ram_kb = 256 * 1024,
        .total_flash_kb = 128 * 1024,
        .bogomips = 1998,
        .kernel_version = "2.6.36.4brcmarm",
        .os_name = "DD-WRT",
        .os_version = "v3.0",
        .busybox_version = "1.24.1",
        .ssh_banner = "SSH-2.0-dropbear_2015.71",
        .telnet_banner = "NETGEAR R7000\r\nLogin: ",
        .mac_prefix = "A0:63:91"
    },
    {
        .name = "Dahua_IPC-HDW",
        .vendor = "Dahua",
        .model = "IPC-HDW4631C-A",
        .type = DEVICE_TYPE_CAMERA,
        .architecture = ARCH_ARMV7,
        .cpu_model = "ARMv7 Processor rev 4 (v7l)",
        .cpu_mhz = 800,
        .cpu_cores = 1,
        .total_ram_kb = 128 * 1024,
        .total_flash_kb = 16 * 1024,
        .bogomips = 792,
        .kernel_version = "3.4.35",
        .os_name = "Embedded Linux",
        .os_version = "2.600",
        .busybox_version = "1.22.1",
        .ssh_banner = "SSH-2.0-OpenSSH_6.0p1",
        .telnet_banner = "Dahua Technology Co., Ltd.\r\nLogin: ",
        .mac_prefix = "3C:EF:8C"
    },
    {
        .name = "D-Link_DIR-615",
        .vendor = "D-Link",
        .model = "DIR-615",
        .type = DEVICE_TYPE_ROUTER,
        .architecture = ARCH_MIPS,
        .cpu_model = "MIPS 24Kc V7.4",
        .cpu_mhz = 400,
        .cpu_cores = 1,
        .total_ram_kb = 32 * 1024,
        .total_flash_kb = 4 * 1024,
        .bogomips = 26640,
        .kernel_version = "2.6.30",
        .os_name = "Linux",
        .os_version = "2.6.30",
        .busybox_version = "1.12.1",
        .ssh_banner = "SSH-2.0-dropbear_2014.63",
        .telnet_banner = "D-Link DIR-615\r\nPassword: ",
        .mac_prefix = "00:1B:11"
    },
    {
        .name = "Generic_IoT",
        .vendor = "Generic",
        .model = "IoT Device",
        .type = DEVICE_TYPE_GENERIC_IOT,
        .architecture = ARCH_ARMV7,
        .cpu_model = "ARMv7 Processor",
        .cpu_mhz = 500,
        .cpu_cores = 1,
        .total_ram_kb = 64 * 1024,
        .total_flash_kb = 8 * 1024,
        .bogomips = 500,
        .kernel_version = "3.4.0",
        .os_name = "Embedded Linux",
        .os_version = "1.0",
        .busybox_version = "1.24.1",
        .ssh_banner = "SSH-2.0-dropbear_2016.74",
        .telnet_banner = "Login: ",
        .mac_prefix = "00:11:22"
    }
};

static const int builtin_profile_count = sizeof(builtin_profiles) / sizeof(builtin_profiles[0]);

/* ============================================================================
 * PROFILE MANAGEMENT
 * ============================================================================ */

int state_get_builtin_profile(device_profile_t* profile, const char* name) {
    if (!profile) return -1;
    
    /* If name specified, search for it */
    if (name) {
        for (int i = 0; i < builtin_profile_count; i++) {
            if (strcasecmp(builtin_profiles[i].name, name) == 0) {
                memcpy(profile, &builtin_profiles[i], sizeof(device_profile_t));
                return 0;
            }
        }
    }
    
    /* Not found or no name - return first profile */
    memcpy(profile, &builtin_profiles[0], sizeof(device_profile_t));
    return name ? -1 : 0;
}

const char** state_list_builtin_profiles(int* count) {
    static const char* names[16];
    
    for (int i = 0; i < builtin_profile_count && i < 16; i++) {
        names[i] = builtin_profiles[i].name;
    }
    
    if (count) *count = builtin_profile_count;
    return names;
}

/* ============================================================================
 * USER INITIALIZATION
 * ============================================================================
 * 
 * Creates fake users that would exist on this type of device.
 * Different devices have different user sets!
 */

static void init_users(system_state_t* state) {
    state->user_count = 0;
    
    /* Root - ALWAYS present on Linux */
    state_user_t* root = &state->users[state->user_count++];
    strncpy(root->username, "root", MAX_NAME_LENGTH - 1);
    snprintf(root->password_hash, sizeof(root->password_hash),
             "$6$%08X$%032X", state_rand(state), state_rand(state));
    root->uid = 0;
    root->gid = 0;
    strncpy(root->home_dir, "/root", MAX_PATH_LENGTH - 1);
    strncpy(root->shell, "/bin/sh", MAX_PATH_LENGTH - 1);
    strncpy(root->gecos, "root", MAX_NAME_LENGTH - 1);
    root->is_system_user = false;
    root->can_login = true;
    
    /* Admin - very common on IoT */
    state_user_t* admin = &state->users[state->user_count++];
    strncpy(admin->username, "admin", MAX_NAME_LENGTH - 1);
    snprintf(admin->password_hash, sizeof(admin->password_hash),
             "$6$%08X$%032X", state_rand(state), state_rand(state));
    admin->uid = state_rand_between(state, 1000, 1010);
    admin->gid = admin->uid;
    strncpy(admin->home_dir, "/home/admin", MAX_PATH_LENGTH - 1);
    strncpy(admin->shell, "/bin/sh", MAX_PATH_LENGTH - 1);
    strncpy(admin->gecos, "Administrator", MAX_NAME_LENGTH - 1);
    admin->is_system_user = false;
    admin->can_login = true;
    
    /* System users - always present */
    struct { const char* name; uid_t uid; } sys_users[] = {
        {"daemon", 1}, {"bin", 2}, {"sys", 3}, {"nobody", 65534}
    };
    
    for (int i = 0; i < 4 && state->user_count < MAX_STATE_USERS; i++) {
        state_user_t* user = &state->users[state->user_count++];
        strncpy(user->username, sys_users[i].name, MAX_NAME_LENGTH - 1);
        strncpy(user->password_hash, "*", sizeof(user->password_hash) - 1);
        user->uid = sys_users[i].uid;
        user->gid = sys_users[i].uid;
        strncpy(user->home_dir, "/", MAX_PATH_LENGTH - 1);
        strncpy(user->shell, "/bin/false", MAX_PATH_LENGTH - 1);
        strncpy(user->gecos, sys_users[i].name, MAX_NAME_LENGTH - 1);
        user->is_system_user = true;
        user->can_login = false;
    }
    
    /* Device-specific users */
    if (state->profile.type == DEVICE_TYPE_CAMERA) {
        /* Camera might have RTSP user */
        if (state_rand_between(state, 0, 100) < 70) {
            state_user_t* user = &state->users[state->user_count++];
            strncpy(user->username, "rtsp", MAX_NAME_LENGTH - 1);
            strncpy(user->password_hash, "*", sizeof(user->password_hash) - 1);
            user->uid = state_rand_between(state, 1001, 1010);
            user->gid = user->uid;
            strncpy(user->home_dir, "/var/run/rtsp", MAX_PATH_LENGTH - 1);
            strncpy(user->shell, "/bin/false", MAX_PATH_LENGTH - 1);
            user->is_system_user = true;
            user->can_login = false;
        }
    } else if (state->profile.type == DEVICE_TYPE_ROUTER) {
        /* Router might have dnsmasq user */
        if (state_rand_between(state, 0, 100) < 60) {
            state_user_t* user = &state->users[state->user_count++];
            strncpy(user->username, "dnsmasq", MAX_NAME_LENGTH - 1);
            strncpy(user->password_hash, "*", sizeof(user->password_hash) - 1);
            user->uid = state_rand_between(state, 1001, 1010);
            user->gid = user->uid;
            strncpy(user->home_dir, "/var/lib/misc", MAX_PATH_LENGTH - 1);
            strncpy(user->shell, "/bin/false", MAX_PATH_LENGTH - 1);
            user->is_system_user = true;
            user->can_login = false;
        }
    }
}

/* ============================================================================
 * PROCESS INITIALIZATION
 * ============================================================================
 * 
 * Creates fake processes appropriate for the device type.
 * CRUCIAL: The process list affects memory usage, load average, etc.
 * These must all CORRELATE!
 */

static void init_processes(system_state_t* state) {
    state->process_count = 0;
    state->next_pid = state_rand_between(state, 1, 500);
    
    /* Kernel threads - always first, low PIDs */
    const char* kernel_threads[] = {
        "init", "kthreadd", "ksoftirqd/0", "kworker/0:0", "kswapd0",
        "watchdog/0", "kdevtmpfs"
    };
    
    int num_kernel = state_rand_between(state, 5, 7);
    for (int i = 0; i < num_kernel && state->process_count < MAX_STATE_PROCESSES; i++) {
        state_process_t* proc = &state->processes[state->process_count++];
        proc->pid = (i == 0) ? 1 : state_rand_between(state, 2, 50);
        proc->ppid = (i <= 1) ? 0 : 2;
        proc->uid = 0;
        proc->gid = 0;
        strncpy(proc->name, kernel_threads[i], MAX_NAME_LENGTH - 1);
        snprintf(proc->cmdline, MAX_CMDLINE_LENGTH, "[%s]", kernel_threads[i]);
        proc->state = PROC_STATE_SLEEPING;
        proc->memory_kb = state_rand_between(state, 0, 64); /* Kernel threads use little mem */
        proc->virtual_kb = 0;
        proc->cpu_percent = state_rand_between(state, 0, 3);
        proc->start_time_offset = i; /* Started right after boot */
        proc->is_kernel_thread = true;
        proc->is_service = true;
        proc->visible_in_ps = true;
        strncpy(proc->tty, "?", sizeof(proc->tty) - 1);
    }
    
    /* Services depend on device type */
    typedef struct {
        const char* name;
        const char* cmd;
        int min_mem;
        int max_mem;
    } service_def_t;
    
    static const service_def_t router_services[] = {
        {"dropbear", "/usr/sbin/dropbear -F -R", 500, 2000},
        {"dnsmasq", "/usr/sbin/dnsmasq -C /etc/dnsmasq.conf", 1000, 4000},
        {"httpd", "/usr/sbin/httpd -p 80 -h /www", 500, 2000},
        {"telnetd", "/usr/sbin/telnetd -F", 300, 1000},
        {"syslogd", "/sbin/syslogd -C16", 200, 800},
        {"crond", "/usr/sbin/crond -f", 200, 500}
    };
    
    static const service_def_t camera_services[] = {
        {"dropbear", "/usr/sbin/dropbear -F", 500, 2000},
        {"rtsp_srv", "/usr/bin/rtsp_server", 2000, 8000},
        {"encoder", "/usr/bin/video_encoder", 5000, 15000},
        {"httpd", "/usr/sbin/httpd -p 80", 500, 2000},
        {"telnetd", "/usr/sbin/telnetd", 300, 1000},
        {"onvif", "/usr/bin/onvif_srvd", 1000, 3000}
    };
    
    const service_def_t* services;
    int num_services;
    
    if (state->profile.type == DEVICE_TYPE_CAMERA) {
        services = camera_services;
        num_services = sizeof(camera_services) / sizeof(camera_services[0]);
    } else {
        services = router_services;
        num_services = sizeof(router_services) / sizeof(router_services[0]);
    }
    
    /* Add random subset of services */
    int services_to_add = state_rand_between(state, 3, num_services);
    for (int i = 0; i < services_to_add && state->process_count < MAX_STATE_PROCESSES; i++) {
        state_process_t* proc = &state->processes[state->process_count++];
        proc->pid = state->next_pid;
        state->next_pid += state_rand_between(state, 1, 100);
        if (state->next_pid > 32000) state->next_pid = state_rand_between(state, 1000, 2000);
        
        proc->ppid = 1; /* Child of init */
        proc->uid = (strcmp(services[i].name, "dropbear") == 0) ? 0 : 
                    state_rand_between(state, 0, 1) * state_rand_between(state, 1000, 1010);
        proc->gid = proc->uid;
        strncpy(proc->name, services[i].name, MAX_NAME_LENGTH - 1);
        strncpy(proc->cmdline, services[i].cmd, MAX_CMDLINE_LENGTH - 1);
        proc->state = PROC_STATE_SLEEPING;
        proc->memory_kb = state_rand_between(state, services[i].min_mem, services[i].max_mem);
        proc->virtual_kb = proc->memory_kb * state_rand_between(state, 2, 4);
        proc->cpu_percent = state_rand_between(state, 0, 30);
        proc->start_time_offset = state_rand_between(state, 5, 120);
        proc->is_kernel_thread = false;
        proc->is_service = true;
        proc->visible_in_ps = true;
        strncpy(proc->tty, "?", sizeof(proc->tty) - 1);
    }
}

/* ============================================================================
 * NETWORK INITIALIZATION
 * ============================================================================ */

static void init_network(system_state_t* state) {
    state->interface_count = 0;
    state->connection_count = 0;
    
    /* Loopback - ALWAYS present */
    state_interface_t* lo = &state->interfaces[state->interface_count++];
    strncpy(lo->name, "lo", MAX_NAME_LENGTH - 1);
    strncpy(lo->ip_address, "127.0.0.1", MAX_IP_LENGTH - 1);
    strncpy(lo->netmask, "255.0.0.0", MAX_IP_LENGTH - 1);
    strncpy(lo->broadcast, "127.255.255.255", MAX_IP_LENGTH - 1);
    strncpy(lo->mac_address, "00:00:00:00:00:00", MAX_MAC_LENGTH - 1);
    lo->mtu = 65536;
    lo->rx_bytes = state_rand_between(state, 10000, 100000);
    lo->tx_bytes = lo->rx_bytes;
    lo->rx_packets = state_rand_between(state, 100, 1000);
    lo->tx_packets = lo->rx_packets;
    lo->is_up = true;
    lo->is_loopback = true;
    lo->is_wireless = false;
    
    /* Primary ethernet */
    const char* eth_names[] = {"eth0", "eth0", "enp0s3"};
    state_interface_t* eth = &state->interfaces[state->interface_count++];
    strncpy(eth->name, eth_names[state_rand_between(state, 0, 2)], MAX_NAME_LENGTH - 1);
    state_rand_ip(state, eth->ip_address, MAX_IP_LENGTH, "local");
    strncpy(eth->netmask, "255.255.255.0", MAX_IP_LENGTH - 1);
    
    /* Calculate broadcast from IP */
    int ip[4];
    if (sscanf(eth->ip_address, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]) == 4) {
        snprintf(eth->broadcast, MAX_IP_LENGTH, "%d.%d.%d.255", ip[0], ip[1], ip[2]);
        snprintf(eth->gateway, MAX_IP_LENGTH, "%d.%d.%d.1", ip[0], ip[1], ip[2]);
    }
    
    state_rand_mac(state, eth->mac_address, MAX_MAC_LENGTH, state->profile.mac_prefix);
    eth->mtu = 1500;
    
    /* Traffic stats based on uptime - THIS IS CORRELATION! */
    /* Roughly 1-10 MB per day for IoT device */
    uint32_t days_up = state->uptime_seconds / 86400;
    eth->rx_bytes = days_up * state_rand_between(state, 1000000, 10000000);
    eth->tx_bytes = days_up * state_rand_between(state, 500000, 5000000);
    eth->rx_packets = eth->rx_bytes / state_rand_between(state, 500, 1500);
    eth->tx_packets = eth->tx_bytes / state_rand_between(state, 500, 1500);
    eth->is_up = true;
    eth->is_loopback = false;
    eth->is_wireless = false;
    
    /* Router might have WAN interface */
    if (state->profile.type == DEVICE_TYPE_ROUTER && state_rand_between(state, 0, 100) < 70) {
        state_interface_t* wan = &state->interfaces[state->interface_count++];
        strncpy(wan->name, "eth1", MAX_NAME_LENGTH - 1);
        state_rand_ip(state, wan->ip_address, MAX_IP_LENGTH, "wan");
        strncpy(wan->netmask, "255.255.255.0", MAX_IP_LENGTH - 1);
        state_rand_mac(state, wan->mac_address, MAX_MAC_LENGTH, state->profile.mac_prefix);
        wan->mtu = 1500;
        wan->rx_bytes = days_up * state_rand_between(state, 10000000, 100000000);
        wan->tx_bytes = days_up * state_rand_between(state, 5000000, 50000000);
        wan->rx_packets = wan->rx_bytes / 1000;
        wan->tx_packets = wan->tx_bytes / 1000;
        wan->is_up = true;
        wan->is_wireless = false;
    }
    
    /* Add listening connections based on services */
    struct { const char* proto; uint16_t port; } listeners[] = {
        {"tcp", 22},   /* SSH */
        {"tcp", 23},   /* Telnet */
        {"tcp", 80},   /* HTTP */
    };
    
    for (int i = 0; i < 3 && state->connection_count < MAX_STATE_CONNECTIONS; i++) {
        if (i == 1 && state_rand_between(state, 0, 100) < 30) continue; /* Sometimes no telnet */
        
        state_connection_t* conn = &state->connections[state->connection_count++];
        strncpy(conn->protocol, listeners[i].proto, sizeof(conn->protocol) - 1);
        strncpy(conn->local_ip, "0.0.0.0", MAX_IP_LENGTH - 1);
        conn->local_port = listeners[i].port;
        strncpy(conn->remote_ip, "0.0.0.0", MAX_IP_LENGTH - 1);
        conn->remote_port = 0;
        conn->state = CONN_STATE_LISTEN;
        conn->owner_pid = (state->process_count > 0) ? 
                          state->processes[state_rand_between(state, 0, state->process_count - 1)].pid : 1;
    }
    
    /* Camera: RTSP on 554 */
    if (state->profile.type == DEVICE_TYPE_CAMERA && state->connection_count < MAX_STATE_CONNECTIONS) {
        state_connection_t* rtsp = &state->connections[state->connection_count++];
        strncpy(rtsp->protocol, "tcp", sizeof(rtsp->protocol) - 1);
        strncpy(rtsp->local_ip, "0.0.0.0", MAX_IP_LENGTH - 1);
        rtsp->local_port = 554;
        strncpy(rtsp->remote_ip, "0.0.0.0", MAX_IP_LENGTH - 1);
        rtsp->remote_port = 0;
        rtsp->state = CONN_STATE_LISTEN;
    }

    /* Router: DNS on 53 */
    if (state->profile.type == DEVICE_TYPE_ROUTER && state->connection_count < MAX_STATE_CONNECTIONS) {
        state_connection_t* dns = &state->connections[state->connection_count++];
        strncpy(dns->protocol, "udp", sizeof(dns->protocol) - 1);
        strncpy(dns->local_ip, "0.0.0.0", MAX_IP_LENGTH - 1);
        dns->local_port = 53;
        strncpy(dns->remote_ip, "0.0.0.0", MAX_IP_LENGTH - 1);
        dns->remote_port = 0;
        dns->state = CONN_STATE_LISTEN;
    }
}

/* ============================================================================
 * FILESYSTEM INITIALIZATION  
 * ============================================================================ */

static void init_filesystem(system_state_t* state) {
    state->file_count = 0;
    state->mount_count = 0;
    
    /* Root mount - always present */
    state_mount_t* root_mount = &state->mounts[state->mount_count++];
    strncpy(root_mount->device, "/dev/root", MAX_PATH_LENGTH - 1);
    strncpy(root_mount->mount_point, "/", MAX_PATH_LENGTH - 1);
    strncpy(root_mount->fs_type, "squashfs", sizeof(root_mount->fs_type) - 1);
    strncpy(root_mount->options, "ro,relatime", sizeof(root_mount->options) - 1);
    root_mount->total_kb = state->profile.total_flash_kb;
    root_mount->used_kb = state_rand_between(state, root_mount->total_kb * 60 / 100, 
                                              root_mount->total_kb * 80 / 100);
    root_mount->available_kb = root_mount->total_kb - root_mount->used_kb;
    
    /* tmpfs on /tmp */
    state_mount_t* tmp_mount = &state->mounts[state->mount_count++];
    strncpy(tmp_mount->device, "tmpfs", MAX_PATH_LENGTH - 1);
    strncpy(tmp_mount->mount_point, "/tmp", MAX_PATH_LENGTH - 1);
    strncpy(tmp_mount->fs_type, "tmpfs", sizeof(tmp_mount->fs_type) - 1);
    strncpy(tmp_mount->options, "rw,nosuid,nodev", sizeof(tmp_mount->options) - 1);
    tmp_mount->total_kb = state->profile.total_ram_kb / 2;
    tmp_mount->used_kb = state_rand_between(state, 100, tmp_mount->total_kb / 10);
    tmp_mount->available_kb = tmp_mount->total_kb - tmp_mount->used_kb;
    
    /* proc - virtual */
    state_mount_t* proc_mount = &state->mounts[state->mount_count++];
    strncpy(proc_mount->device, "proc", MAX_PATH_LENGTH - 1);
    strncpy(proc_mount->mount_point, "/proc", MAX_PATH_LENGTH - 1);
    strncpy(proc_mount->fs_type, "proc", sizeof(proc_mount->fs_type) - 1);
    strncpy(proc_mount->options, "rw,nosuid,nodev,noexec,relatime", sizeof(proc_mount->options) - 1);
    proc_mount->total_kb = 0;
    proc_mount->used_kb = 0;
    proc_mount->available_kb = 0;
    
    /* sysfs - virtual */
    state_mount_t* sys_mount = &state->mounts[state->mount_count++];
    strncpy(sys_mount->device, "sysfs", MAX_PATH_LENGTH - 1);
    strncpy(sys_mount->mount_point, "/sys", MAX_PATH_LENGTH - 1);
    strncpy(sys_mount->fs_type, "sysfs", sizeof(sys_mount->fs_type) - 1);
    strncpy(sys_mount->options, "rw,nosuid,nodev,noexec,relatime", sizeof(sys_mount->options) - 1);
    sys_mount->total_kb = 0;
    sys_mount->used_kb = 0;
    sys_mount->available_kb = 0;
}

/* ============================================================================
 * DERIVED VALUE CALCULATIONS - THE CORRELATION MAGIC
 * ============================================================================
 * 
 * THIS IS WHERE THE RUBIK'S CUBE HAPPENS.
 * All these values are CALCULATED from the state, not random!
 */

static void calculate_memory_usage(system_state_t* state) {
    /* Memory usage = sum of all process memory + kernel overhead */
    uint32_t process_memory = 0;
    for (int i = 0; i < state->process_count; i++) {
        process_memory += state->processes[i].memory_kb;
    }
    
    /* Add kernel overhead (typically 10-20% on embedded) */
    uint32_t kernel_overhead = state->profile.total_ram_kb * 
                               state_rand_between(state, 10, 20) / 100;
    
    state->total_memory_kb = state->profile.total_ram_kb;
    state->used_memory_kb = process_memory + kernel_overhead;
    
    /* Clamp to not exceed total */
    if (state->used_memory_kb > state->total_memory_kb * 95 / 100) {
        state->used_memory_kb = state->total_memory_kb * 95 / 100;
    }
    
    /* Cache and buffers (typically 10-30% of free memory) */
    uint32_t free_mem = state->total_memory_kb - state->used_memory_kb;
    state->cached_memory_kb = free_mem * state_rand_between(state, 15, 30) / 100;
    state->buffer_memory_kb = free_mem * state_rand_between(state, 5, 15) / 100;
}

static void calculate_load_average(system_state_t* state) {
    /* Load average = running processes / CPU cores (roughly) */
    int running = 0;
    for (int i = 0; i < state->process_count; i++) {
        if (state->processes[i].state == PROC_STATE_RUNNING) {
            running++;
        }
    }
    
    /* Load is usually low on IoT devices */
    float base_load = (float)running / (float)state->profile.cpu_cores;
    
    /* Add some variation, stored as fixed point (x100) */
    state->load_avg_1 = (uint16_t)((base_load + 0.01f * state_rand_between(state, 0, 50)) * 100);
    state->load_avg_5 = (uint16_t)((base_load + 0.01f * state_rand_between(state, 0, 30)) * 100);
    state->load_avg_15 = (uint16_t)((base_load + 0.01f * state_rand_between(state, 0, 20)) * 100);
    
    /* Clamp to reasonable values for IoT */
    if (state->load_avg_1 > 500) state->load_avg_1 = state_rand_between(state, 10, 100);
    if (state->load_avg_5 > 400) state->load_avg_5 = state_rand_between(state, 10, 80);
    if (state->load_avg_15 > 300) state->load_avg_15 = state_rand_between(state, 5, 60);
}

static void calculate_cpu_usage(system_state_t* state) {
    /* CPU usage = sum of process CPU percentages (capped) */
    uint32_t total_cpu = 0;
    for (int i = 0; i < state->process_count; i++) {
        total_cpu += state->processes[i].cpu_percent;
    }
    
    /* Normalize to 0-100 range */
    state->cpu_usage_percent = (total_cpu > 100) ? state_rand_between(state, 5, 30) : total_cpu;
}

/* ============================================================================
 * CORE STATE ENGINE API
 * ============================================================================ */

/**
 * Initialize the state engine with a device profile
 */
int state_engine_init(system_state_t* state, const device_profile_t* profile) {
    if (!state) return -1;
    
    /* Clear everything */
    memset(state, 0, sizeof(system_state_t));
    
    /* Copy profile */
    if (profile) {
        memcpy(&state->profile, profile, sizeof(device_profile_t));
    } else {
        /* Use default profile */
        state_get_builtin_profile(&state->profile, NULL);
    }
    
    /* Generate initial seed */
    state->state_seed = (uint32_t)time(NULL) ^ (uint32_t)getpid();
    prng_seed(state->state_seed);
    
    /* Set boot time (1-90 days ago for IoT device) */
    int days_ago = state_rand_between(state, 1, 90);
    int hours_offset = state_rand_between(state, 0, 23);
    int mins_offset = state_rand_between(state, 0, 59);
    state->boot_time = time(NULL) - (days_ago * 86400) - (hours_offset * 3600) - (mins_offset * 60);
    state->uptime_seconds = time(NULL) - state->boot_time;
    state->last_morph_time = time(NULL);
    
    /* Generate hostname */
    const char* prefixes[] = {"router", "cam", "dvr", "device", "iot"};
    int prefix_idx = 0;
    switch (state->profile.type) {
        case DEVICE_TYPE_ROUTER: prefix_idx = 0; break;
        case DEVICE_TYPE_CAMERA: prefix_idx = 1; break;
        case DEVICE_TYPE_DVR: prefix_idx = 2; break;
        default: prefix_idx = state_rand_between(state, 3, 4); break;
    }
    snprintf(state->hostname, sizeof(state->hostname), "%s-%04X", 
             prefixes[prefix_idx], state_rand_between(state, 0, 0xFFFF));
    
    /* Initialize all subsystems */
    init_users(state);
    init_processes(state);
    init_network(state);
    init_filesystem(state);
    
    /* Calculate derived values - THE CORRELATION */
    calculate_memory_usage(state);
    calculate_load_average(state);
    calculate_cpu_usage(state);
    
    state->is_initialized = true;
    state->needs_recalculation = false;
    
    return 0;
}

/**
 * Destroy and clean up state
 */
void state_engine_destroy(system_state_t* state) {
    if (!state) return;
    
    /* Clear sensitive data */
    memset(state, 0, sizeof(system_state_t));
    state->is_initialized = false;
}

/**
 * Update uptime based on current time
 */
void state_engine_update_time(system_state_t* state) {
    if (!state || !state->is_initialized) return;
    
    state->uptime_seconds = time(NULL) - state->boot_time;
}

/**
 * Recalculate all derived values
 * Call this after modifying processes, files, etc.
 */
void state_engine_recalculate(system_state_t* state) {
    if (!state || !state->is_initialized) return;
    
    state_engine_update_time(state);
    calculate_memory_usage(state);
    calculate_load_average(state);
    calculate_cpu_usage(state);
    
    state->needs_recalculation = false;
}

/**
 * MORPH - Generate completely new state with same profile
 * This is the RUBIK'S CUBE TWIST
 */
int state_engine_morph(system_state_t* state, uint32_t seed) {
    if (!state || !state->is_initialized) return -1;
    
    /* Save profile - identity stays the same */
    device_profile_t saved_profile;
    memcpy(&saved_profile, &state->profile, sizeof(device_profile_t));
    
    /* Re-initialize with new seed */
    state->state_seed = seed ? seed : ((uint32_t)time(NULL) ^ state_rand(state));
    prng_seed(state->state_seed);
    
    /* Clear counts but keep profile */
    state->user_count = 0;
    state->process_count = 0;
    state->file_count = 0;
    state->interface_count = 0;
    state->connection_count = 0;
    state->mount_count = 0;
    state->log_count = 0;
    
    /* New boot time (different uptime) */
    int days_ago = state_rand_between(state, 1, 90);
    state->boot_time = time(NULL) - (days_ago * 86400) - state_rand_between(state, 0, 86399);
    state->uptime_seconds = time(NULL) - state->boot_time;
    state->last_morph_time = time(NULL);
    
    /* New hostname */
    const char* prefixes[] = {"router", "cam", "dvr", "device", "iot"};
    int prefix_idx = state_rand_between(state, 0, 4);
    if (state->profile.type == DEVICE_TYPE_ROUTER) prefix_idx = 0;
    else if (state->profile.type == DEVICE_TYPE_CAMERA) prefix_idx = 1;
    snprintf(state->hostname, sizeof(state->hostname), "%s-%04X", 
             prefixes[prefix_idx], state_rand_between(state, 0, 0xFFFF));
    
    /* Regenerate everything */
    init_users(state);
    init_processes(state);
    init_network(state);
    init_filesystem(state);
    
    /* Recalculate correlations */
    calculate_memory_usage(state);
    calculate_load_average(state);
    calculate_cpu_usage(state);
    
    return 0;
}

/**
 * MORPH with new profile - completely change device identity
 */
int state_engine_morph_to_profile(system_state_t* state, const device_profile_t* new_profile) {
    if (!state || !new_profile) return -1;
    
    /* Copy new profile */
    memcpy(&state->profile, new_profile, sizeof(device_profile_t));
    
    /* Morph with new seed */
    return state_engine_morph(state, 0);
}

/* ============================================================================
 * OUTPUT GENERATORS - Generate File Contents From State
 * ============================================================================
 * 
 * These functions generate realistic file contents based on the current state.
 * They DON'T use random values - they DERIVE everything from state!
 */

/**
 * Generate /proc/uptime content
 */
int state_generate_proc_uptime(system_state_t* state, char* buf, size_t size) {
    if (!state || !buf || size < 32) return -1;
    
    state_engine_update_time(state);
    
    /* Format: uptime_seconds idle_seconds */
    /* idle_seconds is typically 90-99% of uptime for IoT */
    double uptime = (double)state->uptime_seconds;
    double idle = uptime * (90 + (state_rand(state) % 10)) / 100.0;
    
    return snprintf(buf, size, "%.2f %.2f\n", uptime, idle);
}

/**
 * Generate /proc/meminfo content
 * THIS IS WHERE CORRELATION SHINES - values derived from process memory!
 */
int state_generate_proc_meminfo(system_state_t* state, char* buf, size_t size) {
    if (!state || !buf || size < 512) return -1;
    
    uint32_t total = state->total_memory_kb;
    uint32_t free = total - state->used_memory_kb;
    uint32_t buffers = state->buffer_memory_kb;
    uint32_t cached = state->cached_memory_kb;
    uint32_t available = free + buffers + cached;
    
    /* Swap is usually 0 on IoT */
    uint32_t swap_total = 0;
    uint32_t swap_free = 0;
    
    return snprintf(buf, size,
        "MemTotal:       %8u kB\n"
        "MemFree:        %8u kB\n"
        "MemAvailable:   %8u kB\n"
        "Buffers:        %8u kB\n"
        "Cached:         %8u kB\n"
        "SwapCached:     %8u kB\n"
        "Active:         %8u kB\n"
        "Inactive:       %8u kB\n"
        "SwapTotal:      %8u kB\n"
        "SwapFree:       %8u kB\n"
        "Dirty:          %8u kB\n"
        "Writeback:      %8u kB\n"
        "AnonPages:      %8u kB\n"
        "Mapped:         %8u kB\n"
        "Shmem:          %8u kB\n"
        "Slab:           %8u kB\n",
        total,
        free,
        available,
        buffers,
        cached,
        0,  /* SwapCached */
        state->used_memory_kb * 60 / 100,  /* Active ~60% of used */
        state->used_memory_kb * 40 / 100,  /* Inactive ~40% of used */
        swap_total,
        swap_free,
        state_rand_between(state, 0, 100),  /* Dirty */
        0,  /* Writeback */
        state->used_memory_kb * 50 / 100,  /* AnonPages */
        state_rand_between(state, 1000, 5000),  /* Mapped */
        state_rand_between(state, 100, 500),    /* Shmem */
        state_rand_between(state, 1000, 3000)   /* Slab */
    );
}

/**
 * Generate /proc/loadavg content
 * Derived from running process count!
 */
int state_generate_proc_loadavg(system_state_t* state, char* buf, size_t size) {
    if (!state || !buf || size < 64) return -1;
    
    /* Count running processes for the fraction */
    int running = 0;
    for (int i = 0; i < state->process_count; i++) {
        if (state->processes[i].state == PROC_STATE_RUNNING) {
            running++;
        }
    }
    if (running == 0) running = 1;
    
    /* Format: load1 load5 load15 running/total last_pid */
    return snprintf(buf, size, "%.2f %.2f %.2f %d/%d %d\n",
        state->load_avg_1 / 100.0,
        state->load_avg_5 / 100.0,
        state->load_avg_15 / 100.0,
        running,
        state->process_count,
        state->next_pid - 1
    );
}

/**
 * Generate /etc/passwd content
 * Derived from user list!
 */
int state_generate_passwd(system_state_t* state, char* buf, size_t size) {
    if (!state || !buf || size < 256) return -1;
    
    int written = 0;
    for (int i = 0; i < state->user_count && written < (int)size - 100; i++) {
        state_user_t* u = &state->users[i];
        int len = snprintf(buf + written, size - written,
            "%s:x:%d:%d:%s:%s:%s\n",
            u->username,
            u->uid,
            u->gid,
            u->gecos,
            u->home_dir,
            u->shell
        );
        if (len > 0) written += len;
    }
    
    return written;
}

/**
 * Generate /etc/shadow content (fake hashes)
 */
int state_generate_shadow(system_state_t* state, char* buf, size_t size) {
    if (!state || !buf || size < 256) return -1;
    
    int written = 0;
    for (int i = 0; i < state->user_count && written < (int)size - 150; i++) {
        state_user_t* u = &state->users[i];
        const char* hash = u->can_login ? u->password_hash : "*";
        int len = snprintf(buf + written, size - written,
            "%s:%s:18000:0:99999:7:::\n",
            u->username,
            hash
        );
        if (len > 0) written += len;
    }
    
    return written;
}

/**
 * Generate ps aux output
 * Directly from process list - perfect correlation!
 */
int state_generate_ps_output(system_state_t* state, char* buf, size_t size, bool aux_format) {
    if (!state || !buf || size < 256) return -1;
    
    int written = 0;
    
    /* Header */
    if (aux_format) {
        written = snprintf(buf, size, 
            "USER       PID %%CPU %%MEM    VSZ   RSS TTY      STAT START   TIME COMMAND\n");
    } else {
        written = snprintf(buf, size, "  PID TTY          TIME CMD\n");
    }
    
    for (int i = 0; i < state->process_count && written < (int)size - 200; i++) {
        state_process_t* p = &state->processes[i];
        if (!p->visible_in_ps) continue;
        
        /* Get username for UID */
        const char* username = "root";
        for (int j = 0; j < state->user_count; j++) {
            if (state->users[j].uid == p->uid) {
                username = state->users[j].username;
                break;
            }
        }
        
        /* State character */
        char stat_char = 'S';
        switch (p->state) {
            case PROC_STATE_RUNNING: stat_char = 'R'; break;
            case PROC_STATE_SLEEPING: stat_char = 'S'; break;
            case PROC_STATE_DISK_WAIT: stat_char = 'D'; break;
            case PROC_STATE_ZOMBIE: stat_char = 'Z'; break;
            case PROC_STATE_STOPPED: stat_char = 'T'; break;
        }
        
        int len;
        if (aux_format) {
            len = snprintf(buf + written, size - written,
                "%-8s %5d  %3d  %3d %6u %5u %-8s %c    %02d:%02d   0:%02d %s\n",
                username,
                p->pid,
                p->cpu_percent / 10,
                p->mem_percent,
                p->virtual_kb,
                p->memory_kb,
                p->tty,
                stat_char,
                (p->start_time_offset / 3600) % 24,
                (p->start_time_offset / 60) % 60,
                p->cpu_time_ms / 60000,
                p->cmdline[0] ? p->cmdline : p->name
            );
        } else {
            len = snprintf(buf + written, size - written,
                "%5d %-8s 00:00:%02d %s\n",
                p->pid,
                p->tty,
                p->cpu_time_ms / 60000,
                p->name
            );
        }
        if (len > 0) written += len;
    }
    
    return written;
}

/**
 * Generate uptime command output
 */
int state_generate_uptime_output(system_state_t* state, char* buf, size_t size) {
    if (!state || !buf || size < 128) return -1;
    
    state_engine_update_time(state);
    
    uint32_t uptime = state->uptime_seconds;
    int days = uptime / 86400;
    int hours = (uptime % 86400) / 3600;
    int mins = (uptime % 3600) / 60;
    
    /* Count users (always 1 for honeypot) */
    int users = 1;
    
    /* Current time */
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[16];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
    
    if (days > 0) {
        return snprintf(buf, size, 
            " %s up %d days, %2d:%02d,  %d user,  load average: %.2f, %.2f, %.2f\n",
            time_str, days, hours, mins, users,
            state->load_avg_1 / 100.0,
            state->load_avg_5 / 100.0,
            state->load_avg_15 / 100.0
        );
    } else {
        return snprintf(buf, size,
            " %s up %2d:%02d,  %d user,  load average: %.2f, %.2f, %.2f\n",
            time_str, hours, mins, users,
            state->load_avg_1 / 100.0,
            state->load_avg_5 / 100.0,
            state->load_avg_15 / 100.0
        );
    }
}

/**
 * Generate free command output
 */
int state_generate_free_output(system_state_t* state, char* buf, size_t size) {
    if (!state || !buf || size < 256) return -1;
    
    uint32_t total = state->total_memory_kb;
    uint32_t used = state->used_memory_kb;
    uint32_t free = total - used;
    uint32_t shared = state_rand_between(state, 100, 500);
    uint32_t buffers = state->buffer_memory_kb;
    uint32_t cached = state->cached_memory_kb;
    uint32_t available = free + buffers + cached;
    
    return snprintf(buf, size,
        "              total        used        free      shared  buff/cache   available\n"
        "Mem:       %8u    %8u    %8u    %8u    %8u    %8u\n"
        "Swap:      %8u    %8u    %8u\n",
        total, used, free, shared, buffers + cached, available,
        0, 0, 0  /* No swap on IoT */
    );
}

/**
 * Generate ifconfig output
 */
int state_generate_ifconfig_output(system_state_t* state, char* buf, size_t size) {
    if (!state || !buf || size < 512) return -1;
    
    int written = 0;
    
    for (int i = 0; i < state->interface_count && written < (int)size - 400; i++) {
        state_interface_t* iface = &state->interfaces[i];
        
        int len = snprintf(buf + written, size - written,
            "%s      Link encap:%s  HWaddr %s\n"
            "          inet addr:%s  Bcast:%s  Mask:%s\n"
            "          %s  MTU:%u  Metric:1\n"
            "          RX packets:%lu errors:%u dropped:0 overruns:0 frame:0\n"
            "          TX packets:%lu errors:%u dropped:0 overruns:0 carrier:0\n"
            "          collisions:0 txqueuelen:1000\n"
            "          RX bytes:%lu  TX bytes:%lu\n\n",
            iface->name,
            iface->is_loopback ? "Local Loopback" : "Ethernet",
            iface->mac_address,
            iface->ip_address,
            iface->broadcast,
            iface->netmask,
            iface->is_up ? "UP BROADCAST RUNNING MULTICAST" : "DOWN",
            iface->mtu,
            (unsigned long)iface->rx_packets,
            iface->rx_errors,
            (unsigned long)iface->tx_packets,
            iface->tx_errors,
            (unsigned long)iface->rx_bytes,
            (unsigned long)iface->tx_bytes
        );
        if (len > 0) written += len;
    }
    
    return written;
}

/**
 * Generate /proc/cpuinfo content
 */
int state_generate_proc_cpuinfo(system_state_t* state, char* buf, size_t size) {
    if (!state || !buf || size < 512) return -1;
    
    int written = 0;
    
    for (uint32_t i = 0; i < state->profile.cpu_cores && written < (int)size - 300; i++) {
        int len;
        
        if (state->profile.architecture == ARCH_MIPS || 
            state->profile.architecture == ARCH_MIPSEL) {
            /* MIPS format */
            len = snprintf(buf + written, size - written,
                "system type\t\t: %s\n"
                "machine\t\t\t: %s %s\n"
                "processor\t\t: %d\n"
                "cpu model\t\t: %s\n"
                "BogoMIPS\t\t: %u.%02u\n"
                "wait instruction\t: yes\n"
                "microsecond timers\t: yes\n"
                "tlb_entries\t\t: 32\n"
                "extra interrupt vector\t: yes\n"
                "hardware watchpoint\t: yes\n"
                "isa\t\t\t: mips1 mips2 mips32r1 mips32r2\n"
                "ASEs implemented\t: mips16 dsp dsp2\n\n",
                state->profile.cpu_model,
                state->profile.vendor,
                state->profile.model,
                i,
                state->profile.cpu_model,
                state->profile.bogomips / 100,
                state->profile.bogomips % 100
            );
        } else {
            /* ARM format */
            len = snprintf(buf + written, size - written,
                "processor\t: %d\n"
                "model name\t: %s\n"
                "BogoMIPS\t: %u.%02u\n"
                "Features\t: %s\n"
                "CPU implementer\t: 0x41\n"
                "CPU architecture: %d\n"
                "CPU variant\t: 0x0\n"
                "CPU part\t: 0xc07\n"
                "CPU revision\t: 5\n\n",
                i,
                state->profile.cpu_model,
                state->profile.bogomips / 100,
                state->profile.bogomips % 100,
                (state->profile.architecture == ARCH_ARMV7) ? 
                    "half thumb fastmult vfp edsp neon vfpv3 tls vfpv4" :
                    "half thumb fastmult vfp edsp",
                (state->profile.architecture == ARCH_AARCH64) ? 8 : 7
            );
        }
        if (len > 0) written += len;
    }
    
    return written;
}

/**
 * Generate /proc/version content
 */
int state_generate_proc_version(system_state_t* state, char* buf, size_t size) {
    if (!state || !buf || size < 256) return -1;
    
    return snprintf(buf, size,
        "Linux version %s (%s@%s) (gcc version 5.4.0) #1 SMP %s\n",
        state->profile.kernel_version,
        "root",
        state->hostname,
        "Mon Jan 1 00:00:00 UTC 2024"
    );
}

/**
 * Generate /proc/mounts content
 */
int state_generate_proc_mounts(system_state_t* state, char* buf, size_t size) {
    if (!state || !buf || size < 512) return -1;
    
    int written = 0;
    
    for (int i = 0; i < state->mount_count && written < (int)size - 150; i++) {
        state_mount_t* m = &state->mounts[i];
        int len = snprintf(buf + written, size - written,
            "%s %s %s %s 0 0\n",
            m->device,
            m->mount_point,
            m->fs_type,
            m->options
        );
        if (len > 0) written += len;
    }
    
    return written;
}

/**
 * Generate df command output
 */
int state_generate_df_output(system_state_t* state, char* buf, size_t size) {
    if (!state || !buf || size < 512) return -1;
    
    int written = snprintf(buf, size,
        "Filesystem           1K-blocks      Used Available Use%% Mounted on\n");
    
    for (int i = 0; i < state->mount_count && written < (int)size - 100; i++) {
        state_mount_t* m = &state->mounts[i];
        if (m->total_kb == 0) continue; /* Skip virtual filesystems */
        
        int use_percent = (m->total_kb > 0) ? (m->used_kb * 100 / m->total_kb) : 0;
        int len = snprintf(buf + written, size - written,
            "%-20s %10lu %10lu %10lu %3d%% %s\n",
            m->device,
            (unsigned long)m->total_kb,
            (unsigned long)m->used_kb,
            (unsigned long)m->available_kb,
            use_percent,
            m->mount_point
        );
        if (len > 0) written += len;
    }
    
    return written;
}

/**
 * Generate netstat output
 */
int state_generate_netstat_output(system_state_t* state, char* buf, size_t size) {
    if (!state || !buf || size < 512) return -1;
    
    int written = snprintf(buf, size,
        "Active Internet connections (servers and established)\n"
        "Proto Recv-Q Send-Q Local Address           Foreign Address         State\n");
    
    for (int i = 0; i < state->connection_count && written < (int)size - 100; i++) {
        state_connection_t* c = &state->connections[i];
        
        const char* state_str = "UNKNOWN";
        switch (c->state) {
            case CONN_STATE_LISTEN: state_str = "LISTEN"; break;
            case CONN_STATE_ESTABLISHED: state_str = "ESTABLISHED"; break;
            case CONN_STATE_TIME_WAIT: state_str = "TIME_WAIT"; break;
            case CONN_STATE_CLOSE_WAIT: state_str = "CLOSE_WAIT"; break;
            case CONN_STATE_SYN_SENT: state_str = "SYN_SENT"; break;
            case CONN_STATE_SYN_RECV: state_str = "SYN_RECV"; break;
        }
        
        char local[32], remote[32];
        snprintf(local, sizeof(local), "%s:%d", c->local_ip, c->local_port);
        snprintf(remote, sizeof(remote), "%s:%d", 
                 c->state == CONN_STATE_LISTEN ? "0.0.0.0" : c->remote_ip, 
                 c->remote_port);
        
        int len = snprintf(buf + written, size - written,
            "%-5s %6d %6d %-23s %-23s %s\n",
            c->protocol,
            0, 0,  /* Recv-Q, Send-Q */
            local,
            remote,
            state_str
        );
        if (len > 0) written += len;
    }
    
    return written;
}

/**
 * Generate uname output
 */
int state_generate_uname_output(system_state_t* state, char* buf, size_t size, const char* flags) {
    if (!state || !buf || size < 256) return -1;
    
    const char* arch_str = "unknown";
    switch (state->profile.architecture) {
        case ARCH_MIPS: arch_str = "mips"; break;
        case ARCH_MIPSEL: arch_str = "mipsel"; break;
        case ARCH_ARM: arch_str = "armv6l"; break;
        case ARCH_ARMV7: arch_str = "armv7l"; break;
        case ARCH_AARCH64: arch_str = "aarch64"; break;
        case ARCH_X86: arch_str = "i686"; break;
        case ARCH_X86_64: arch_str = "x86_64"; break;
    }
    
    if (!flags || strcmp(flags, "-a") == 0) {
        return snprintf(buf, size, "Linux %s %s #1 SMP %s %s GNU/Linux\n",
            state->hostname,
            state->profile.kernel_version,
            "Mon Jan 1 00:00:00 UTC 2024",
            arch_str
        );
    } else if (strcmp(flags, "-r") == 0) {
        return snprintf(buf, size, "%s\n", state->profile.kernel_version);
    } else if (strcmp(flags, "-m") == 0) {
        return snprintf(buf, size, "%s\n", arch_str);
    } else if (strcmp(flags, "-n") == 0) {
        return snprintf(buf, size, "%s\n", state->hostname);
    } else if (strcmp(flags, "-s") == 0) {
        return snprintf(buf, size, "Linux\n");
    }
    
    return snprintf(buf, size, "Linux\n");
}

/**
 * Route content from path to appropriate generator
 */
int state_generate_file_content(system_state_t* state, const char* path,
                                char* buffer, size_t buffer_size) {
    if (!state || !path || !buffer) return -1;
    
    /* Route to appropriate generator based on path */
    if (strcmp(path, "/proc/uptime") == 0) {
        return state_generate_proc_uptime(state, buffer, buffer_size);
    } else if (strcmp(path, "/proc/meminfo") == 0) {
        return state_generate_proc_meminfo(state, buffer, buffer_size);
    } else if (strcmp(path, "/proc/loadavg") == 0) {
        return state_generate_proc_loadavg(state, buffer, buffer_size);
    } else if (strcmp(path, "/proc/cpuinfo") == 0) {
        return state_generate_proc_cpuinfo(state, buffer, buffer_size);
    } else if (strcmp(path, "/proc/version") == 0) {
        return state_generate_proc_version(state, buffer, buffer_size);
    } else if (strcmp(path, "/proc/mounts") == 0) {
        return state_generate_proc_mounts(state, buffer, buffer_size);
    } else if (strcmp(path, "/etc/passwd") == 0) {
        return state_generate_passwd(state, buffer, buffer_size);
    } else if (strcmp(path, "/etc/shadow") == 0) {
        return state_generate_shadow(state, buffer, buffer_size);
    }
    
    return -1; /* Unknown path */
}

/* ============================================================================
 * GLOBAL STATE MANAGEMENT
 * ============================================================================ */

system_state_t* state_get_global(void) {
    if (!g_state_initialized) return NULL;
    return &g_state;
}

int state_init_global(const device_profile_t* profile) {
    if (g_state_initialized) {
        state_engine_destroy(&g_state);
    }
    
    int result = state_engine_init(&g_state, profile);
    if (result == 0) {
        g_state_initialized = true;
    }
    return result;
}

void state_destroy_global(void) {
    if (g_state_initialized) {
        state_engine_destroy(&g_state);
        g_state_initialized = false;
    }
}