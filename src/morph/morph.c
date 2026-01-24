#include "morph.h"

#include "behavior.h"
#include "filesystem.h"
#include "network.h"
#include "processes.h"
#include "quorum_adapt.h"
#include "temporal.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Global state
static device_profile_t profiles[MAX_PROFILES];
static int profile_count = 0;
static int current_profile_index = -1;
static char state_file_path[MAX_PATH_SIZE] = "build/morph-state.txt";

// Forward declaration
static int create_default_profiles(void);

// External function declaration for Phase 1 (Network)
extern int morph_network_config(const char* base_ip, const char* profile_type);

// Profile management
int load_profiles(const char* config_file) {
    if (!file_exists(config_file)) {
        log_event_level(LOG_WARN, "Profile config file not found, using defaults");
        return create_default_profiles();
    }

    FILE* f = fopen(config_file, "r");
    if (!f) {
        log_event_level(LOG_ERROR, "Failed to open profile config file");
        return -1;
    }

    profile_count = 0;
    char line[1024];
    device_profile_t* current = NULL;

    while (fgets(line, sizeof(line), f) && profile_count < MAX_PROFILES) {
        // Remove newline
        line[strcspn(line, "\n")] = '\0';
        trim_string(line);

        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\0') {
            continue;
        }

        // Profile header: [ProfileName]
        if (line[0] == '[' && line[strlen(line) - 1] == ']') {
            if (profile_count < MAX_PROFILES) {
                current = &profiles[profile_count];
                // Remove [ and ] brackets
                size_t name_len = strlen(line) - 2; // Exclude [ and ]
                if (name_len > MAX_PROFILE_NAME - 1) {
                    name_len = MAX_PROFILE_NAME - 1;
                }
                strncpy(current->name, line + 1, name_len);
                current->name[name_len] = '\0';

                // Set defaults
                snprintf(current->ssh_banner, MAX_BANNER_SIZE, "SSH-2.0-OpenSSH_7.4");
                snprintf(current->telnet_banner, MAX_BANNER_SIZE, "Welcome to device");
                snprintf(current->shell_prompt, MAX_PROFILE_NAME, "router# ");
                snprintf(current->router_html_path,
                         MAX_PATH_SIZE,
                         "services/fake-router-web/html/index.html");
                snprintf(current->camera_html_path,
                         MAX_PATH_SIZE,
                         "services/fake-camera-web/html/index.html");
                snprintf(current->kernel_version, MAX_KERNEL_VERSION, "3.2.0");
                snprintf(current->arch, MAX_PROFILE_NAME, "armv7l");
                snprintf(current->mac_address, MAX_MAC_ADDR, "00:11:22");
                current->memory_mb = 64;
                current->cpu_mhz = 600;

                profile_count++;
            }
            continue;
        }

        // Parse key=value pairs
        if (current) {
            char* eq = strchr(line, '=');
            if (eq) {
                *eq = '\0';
                char* key = line;
                char* value = eq + 1;
                trim_string(key);
                trim_string(value);

                if (strcmp(key, "ssh_banner") == 0) {
                    strncpy(current->ssh_banner, value, MAX_BANNER_SIZE - 1);
                } else if (strcmp(key, "telnet_banner") == 0) {
                    strncpy(current->telnet_banner, value, MAX_BANNER_SIZE - 1);
                } else if (strcmp(key, "router_html") == 0) {
                    strncpy(current->router_html_path, value, MAX_PATH_SIZE - 1);
                } else if (strcmp(key, "camera_html") == 0) {
                    strncpy(current->camera_html_path, value, MAX_PATH_SIZE - 1);
                } else if (strcmp(key, "kernel_version") == 0) {
                    strncpy(current->kernel_version, value, MAX_KERNEL_VERSION - 1);
                } else if (strcmp(key, "arch") == 0) {
                    strncpy(current->arch, value, MAX_PROFILE_NAME - 1);
                } else if (strcmp(key, "mac_prefix") == 0) {
                    strncpy(current->mac_address, value, MAX_MAC_ADDR - 1);
                } else if (strcmp(key, "memory_mb") == 0) {
                    current->memory_mb = atoi(value);
                } else if (strcmp(key, "cpu_mhz") == 0) {
                    current->cpu_mhz = atoi(value);
                } else if (strcmp(key, "shell_prompt") == 0) {
                    strncpy(current->shell_prompt, value, MAX_PROFILE_NAME - 1);
                } else if (strcmp(key, "is_vulnerable") == 0) {
                    current->is_vulnerable = atoi(value);
                } else if (strcmp(key, "cves") == 0) {
                    strncpy(current->cves, value, 255);
                }
            }
        }
    }

    fclose(f);

    if (profile_count == 0) {
        log_event_level(LOG_WARN, "No profiles loaded, using defaults");
        return create_default_profiles();
    }

    log_event_level(LOG_INFO, "Loaded profiles");
    return profile_count;
}

static int create_default_profiles(void) {
    profile_count = 6;

    // WHY THIS FIX: Real IoT devices use dropbear (tiny SSH server), NOT Ubuntu OpenSSH!
    // Using Ubuntu banners is like a teenager using a fake ID that says they're 45.
    // Attackers instantly recognize "Ubuntu" on a router as a honeypot.
    // These banners are from REAL devices captured in the wild.

    // Profile 1: TP-Link Archer C7 Router
    // Real TP-Link routers use dropbear, a lightweight SSH server for embedded systems
    snprintf(profiles[0].name, sizeof(profiles[0].name), "%s", "TP-Link_Archer_C7");
    snprintf(
        profiles[0].ssh_banner, sizeof(profiles[0].ssh_banner), "%s", "SSH-2.0-dropbear_2017.75");
    snprintf(profiles[0].telnet_banner,
             sizeof(profiles[0].telnet_banner),
             "%s",
             "TP-Link Archer C7 v4\r\nLogin: ");
    snprintf(profiles[0].router_html_path,
             sizeof(profiles[0].router_html_path),
             "%s",
             "services/fake-router-web/html/themes/tplink.html");
    snprintf(profiles[0].camera_html_path,
             sizeof(profiles[0].camera_html_path),
             "%s",
             "services/fake-camera-web/html/themes/default.html");
    snprintf(profiles[0].kernel_version, sizeof(profiles[0].kernel_version), "%s", "3.10.49");
    snprintf(profiles[0].arch, sizeof(profiles[0].arch), "%s", "mips");
    snprintf(profiles[0].mac_address, sizeof(profiles[0].mac_address), "%s", "14:cc:20");
    profiles[0].memory_mb = 128;
    profiles[0].cpu_mhz = 720;

    // Profile 2: D-Link DIR-615 Router
    // Older D-Link devices often use older dropbear or busybox telnetd
    snprintf(profiles[1].name, sizeof(profiles[1].name), "%s", "D-Link_DIR-615");
    snprintf(
        profiles[1].ssh_banner, sizeof(profiles[1].ssh_banner), "%s", "SSH-2.0-dropbear_2014.63");
    snprintf(profiles[1].telnet_banner,
             sizeof(profiles[1].telnet_banner),
             "%s",
             "D-Link DIR-615\r\nPassword: ");
    snprintf(profiles[1].router_html_path,
             sizeof(profiles[1].router_html_path),
             "%s",
             "services/fake-router-web/html/themes/dlink.html");
    snprintf(profiles[1].camera_html_path,
             sizeof(profiles[1].camera_html_path),
             "%s",
             "services/fake-camera-web/html/themes/default.html");
    snprintf(profiles[1].kernel_version, sizeof(profiles[1].kernel_version), "%s", "2.6.30");
    snprintf(profiles[1].arch, sizeof(profiles[1].arch), "%s", "mips");
    snprintf(profiles[1].mac_address, sizeof(profiles[1].mac_address), "%s", "00:1b:11");
    profiles[1].memory_mb = 32;
    profiles[1].cpu_mhz = 400;

    // Profile 3: Netgear R7000 Router
    // Netgear uses dropbear on most consumer routers
    snprintf(profiles[2].name, sizeof(profiles[2].name), "%s", "Netgear_R7000");
    snprintf(
        profiles[2].ssh_banner, sizeof(profiles[2].ssh_banner), "%s", "SSH-2.0-dropbear_2015.71");
    snprintf(profiles[2].telnet_banner,
             sizeof(profiles[2].telnet_banner),
             "%s",
             "NETGEAR R7000\r\nLogin: ");
    snprintf(profiles[2].router_html_path,
             sizeof(profiles[2].router_html_path),
             "%s",
             "services/fake-router-web/html/themes/netgear.html");
    snprintf(profiles[2].camera_html_path,
             sizeof(profiles[2].camera_html_path),
             "%s",
             "services/fake-camera-web/html/themes/default.html");
    snprintf(
        profiles[2].kernel_version, sizeof(profiles[2].kernel_version), "%s", "2.6.36.4brcmarm");
    snprintf(profiles[2].arch, sizeof(profiles[2].arch), "%s", "armv7l");
    snprintf(profiles[2].mac_address, sizeof(profiles[2].mac_address), "%s", "a0:63:91");
    profiles[2].memory_mb = 256;
    profiles[2].cpu_mhz = 1000;

    // Profile 4: Hikvision DS-2CD2 Camera
    // Chinese IP cameras often use older OpenSSH (NOT Ubuntu!) or dropbear
    snprintf(profiles[3].name, sizeof(profiles[3].name), "%s", "Hikvision_DS-2CD2");
    snprintf(profiles[3].ssh_banner, sizeof(profiles[3].ssh_banner), "%s", "SSH-2.0-OpenSSH_5.8p1");
    snprintf(profiles[3].telnet_banner,
             sizeof(profiles[3].telnet_banner),
             "%s",
             "Hikvision Digital Technology Co., Ltd.\r\nLogin: ");
    snprintf(profiles[3].router_html_path,
             sizeof(profiles[3].router_html_path),
             "%s",
             "services/fake-router-web/html/themes/default.html");
    snprintf(profiles[3].camera_html_path,
             sizeof(profiles[3].camera_html_path),
             "%s",
             "services/fake-camera-web/html/themes/hikvision.html");
    snprintf(profiles[3].kernel_version, sizeof(profiles[3].kernel_version), "%s", "3.0.8");
    snprintf(profiles[3].arch, sizeof(profiles[3].arch), "%s", "armv7l");
    snprintf(profiles[3].mac_address, sizeof(profiles[3].mac_address), "%s", "44:19:b6");
    profiles[3].memory_mb = 64;
    profiles[3].cpu_mhz = 600;

    // Profile 5: Dahua IPC-HDW Camera
    // Dahua cameras use similar old embedded OpenSSH
    snprintf(profiles[4].name, sizeof(profiles[4].name), "%s", "Dahua_IPC-HDW");
    snprintf(profiles[4].ssh_banner, sizeof(profiles[4].ssh_banner), "%s", "SSH-2.0-OpenSSH_6.0p1");
    snprintf(profiles[4].telnet_banner,
             sizeof(profiles[4].telnet_banner),
             "%s",
             "Dahua Technology Co., Ltd.\r\nLogin: ");
    snprintf(profiles[4].router_html_path,
             sizeof(profiles[4].router_html_path),
             "%s",
             "services/fake-router-web/html/themes/default.html");
    snprintf(profiles[4].camera_html_path,
             sizeof(profiles[4].camera_html_path),
             "%s",
             "services/fake-camera-web/html/themes/dahua.html");
    snprintf(profiles[4].kernel_version, sizeof(profiles[4].kernel_version), "%s", "3.4.35");
    snprintf(profiles[4].arch, sizeof(profiles[4].arch), "%s", "armv7l");
    snprintf(profiles[4].mac_address, sizeof(profiles[4].mac_address), "%s", "00:12:16");
    profiles[4].memory_mb = 128;
    profiles[4].cpu_mhz = 800;

    // Profile 6: Generic Router (fallback)
    // Generic profile using common dropbear version
    snprintf(profiles[5].name, sizeof(profiles[5].name), "%s", "Generic_Router");
    snprintf(
        profiles[5].ssh_banner, sizeof(profiles[5].ssh_banner), "%s", "SSH-2.0-dropbear_2019.78");
    snprintf(profiles[5].telnet_banner,
             sizeof(profiles[5].telnet_banner),
             "%s",
             "BusyBox v1.24.1 built-in shell\r\nlogin: ");
    snprintf(profiles[5].router_html_path,
             sizeof(profiles[5].router_html_path),
             "%s",
             "services/fake-router-web/html/themes/generic.html");
    snprintf(profiles[5].camera_html_path,
             sizeof(profiles[5].camera_html_path),
             "%s",
             "services/fake-camera-web/html/themes/default.html");
    snprintf(profiles[5].kernel_version, sizeof(profiles[5].kernel_version), "%s", "4.4.0");
    snprintf(profiles[5].arch, sizeof(profiles[5].arch), "%s", "armv7l");
    snprintf(profiles[5].mac_address, sizeof(profiles[5].mac_address), "%s", "00:11:22");
    profiles[5].memory_mb = 64;
    profiles[5].cpu_mhz = 533;

    return profile_count;
}

int get_profile_count(void) {
    return profile_count;
}

device_profile_t* get_profile(int index) {
    if (index < 0 || index >= profile_count) {
        return NULL;
    }
    return &profiles[index];
}

int get_current_profile_index(void) {
    return current_profile_index;
}

int set_current_profile(int index) {
    if (index < 0 || index >= profile_count) {
        return -1;
    }
    current_profile_index = index;
    return 0;
}

// Morphing functions
int morph_cowrie_banners(const device_profile_t* profile) {
    if (!profile)
        return -1;

    char cowrie_cfg_path[] = "services/cowrie/etc/cowrie.cfg";
    char cowrie_cfg_local[] = "services/cowrie/etc/cowrie.cfg.local";

    // Create cowrie config directory if it doesn't exist
    create_dir("services/cowrie/etc");

    // Cowrie reads configuration from cowrie.cfg (main file)
    // The [shell] section controls uname output - this is what affects command responses
    // Environment variables take precedence, but we also write the config file
    char config_content[4096];
    snprintf(config_content,
             sizeof(config_content),
             "# Cowrie configuration - Auto-generated by CERBERUS morph engine\n"
             "# Profile: %s\n"
             "# This file is read by Cowrie at startup for uname/hostname commands\n\n"
             "[output_jsonlog]\n"
             "enabled = true\n"
             "logfile = ${honeypot:log_path}/cowrie.json\n\n"
             "[output_textlog]\n"
             "enabled = true\n"
             "logfile = ${honeypot:log_path}/cowrie.log\n\n"
             "[ssh]\n"
             "# SSH settings\n"
             "listen_endpoints = tcp:2222:interface=0.0.0.0\n"
             "version = %s\n"
             "banner = %s\n\n"
             "[telnet]\n"
             "# Telnet settings\n"
             "listen_endpoints = tcp:2323:interface=0.0.0.0\n"
             "banner = %s\n\n"
             "[honeypot]\n"
             "# Honeypot hostname (appears in logs and prompt)\n"
             "hostname = %s\n"
             "# Session timeout in seconds (600 = 10 minutes)\n"
             "timeout = 600\n"
             "# Realistic login attempt limits\n"
             "login_attempt_limit = 10\n\n"
             "[shell]\n"
             "# Shell configuration - controls uname and hostname command outputs\n"
             "# These values are what Cowrie returns for uname -a, uname -r, hostname, etc.\n"
             "kernel_name = Linux\n"
             "kernel_version = %s\n"
             "kernel_build_string = #1 SMP PREEMPT %s\n"
             "hardware_platform = %s\n"
             "operating_system = GNU/Linux\n"
             "hostname = %s\n"
             "arch = %s\n"
             "prompt = %s\n",
             profile->name,
             profile->ssh_banner,
             profile->ssh_banner,
             profile->telnet_banner,
             profile->name,
             profile->kernel_version,
             profile->arch,
             profile->arch,
             profile->name,
             strcmp(profile->arch, "mips") == 0 ? "linux-mips-lsb"
             : (strcmp(profile->arch, "armv7l") == 0 || strcmp(profile->arch, "arm") == 0)
                 ? "linux-arm-lsb"
             : (strcmp(profile->arch, "aarch64") == 0 || strcmp(profile->arch, "armv8l") == 0)
                 ? "linux-aarch64-lsb"
             : strcmp(profile->arch, "powerpc") == 0 ? "linux-powerpc-lsb"
             : (strcmp(profile->arch, "x86_64") == 0 || strcmp(profile->arch, "amd64") == 0)
                 ? "linux-x64-lsb"
                 : "linux-arm-lsb",
             profile->shell_prompt);

    // Write to MAIN config file (cowrie.cfg) - this is what Cowrie reads!
    if (write_file(cowrie_cfg_path, config_content) != 0) {
        log_event_level(LOG_ERROR, "Failed to write Cowrie config");
        return -1;
    }

    // Also write to local config for backwards compatibility
    if (write_file(cowrie_cfg_local, config_content) != 0) {
        log_event_level(LOG_WARN, "Failed to write Cowrie local config");
    }

    // Generate environment file for Docker - Cowrie checks env vars FIRST
    // This is the most reliable way to set uname values
    char env_file_path[] = "services/cowrie/etc/cowrie.env";
    char env_content[1024];
    snprintf(env_content,
             sizeof(env_content),
             "# Cowrie environment variables - Auto-generated by CERBERUS morph engine\n"
             "# Profile: %s\n"
             "# These override ALL config file values for uname/hostname commands\n"
             "COWRIE_SHELL_KERNEL_NAME=Linux\n"
             "COWRIE_SHELL_KERNEL_VERSION=%s\n"
             "COWRIE_SHELL_KERNEL_BUILD_STRING=#1 SMP PREEMPT %s\n"
             "COWRIE_SHELL_HARDWARE_PLATFORM=%s\n"
             "COWRIE_SHELL_OPERATING_SYSTEM=GNU/Linux\n"
             "COWRIE_HONEYPOT_HOSTNAME=%s\n"
             "COWRIE_SHELL_ARCH=%s\n",
             profile->name,
             profile->kernel_version,
             profile->arch,
             profile->arch,
             profile->name,
             strcmp(profile->arch, "mips") == 0 ? "linux-mips-lsb"
             : (strcmp(profile->arch, "armv7l") == 0 || strcmp(profile->arch, "arm") == 0)
                 ? "linux-arm-lsb"
             : (strcmp(profile->arch, "aarch64") == 0 || strcmp(profile->arch, "armv8l") == 0)
                 ? "linux-aarch64-lsb"
             : strcmp(profile->arch, "powerpc") == 0 ? "linux-powerpc-lsb"
             : (strcmp(profile->arch, "x86_64") == 0 || strcmp(profile->arch, "amd64") == 0)
                 ? "linux-x64-lsb"
                 : "linux-arm-lsb");

    if (write_file(env_file_path, env_content) != 0) {
        log_event_level(LOG_WARN, "Failed to write Cowrie env file");
    }

    // Update honeyfs (fake filesystem) files for shell responses
    // Create honeyfs directory structure if it doesn't exist
    create_dir("services/cowrie/honeyfs/etc");
    create_dir("services/cowrie/honeyfs/usr/lib");
    create_dir("services/cowrie/honeyfs/proc");

    // Update /etc/hostname
    char hostname_path[] = "services/cowrie/honeyfs/etc/hostname";
    write_file(hostname_path, profile->name);

    // Update /etc/os-release (vitals for modern scanners)
    char os_release_path[] = "services/cowrie/honeyfs/etc/os-release";
    char os_release_content[1024];
    snprintf(os_release_content,
             sizeof(os_release_content),
             "PRETTY_NAME=\"%s Embedded Linux\"\n"
             "NAME=\"%s\"\n"
             "ID=%s\n"
             "ID_LIKE=debian\n"
             "VERSION_ID=\"1.0\"\n"
             "VERSION=\"1.0 (Cerberus)\"\n"
             "HOME_URL=\"http://www.tplink.com/\"\n"
             "SUPPORT_URL=\"http://www.tplink.com/support\"\n",
             profile->name,
             profile->name,
             profile->name);
    write_file(os_release_path, os_release_content);
    write_file("services/cowrie/honeyfs/usr/lib/os-release", os_release_content);

    // Update /etc/issue.net (remote login banner)
    char issue_net_path[] = "services/cowrie/honeyfs/etc/issue.net";
    char issue_net_content[256];
    snprintf(issue_net_content, sizeof(issue_net_content), "%s Console Login\n", profile->name);
    write_file(issue_net_path, issue_net_content);

    // Update /proc/version (what 'uname -a' reads)
    char proc_version_path[] = "services/cowrie/honeyfs/proc/version";
    char proc_version[512];
    snprintf(proc_version,
             sizeof(proc_version),
             "Linux version %s (root@localhost) (gcc version 4.6.3) #1 SMP PREEMPT %s",
             profile->kernel_version,
             profile->arch);
    if (write_file(proc_version_path, proc_version) != 0) {
        log_event_level(LOG_WARN, "Failed to write honeyfs proc/version");
    }

    // Update /etc/issue (login banner)
    char issue_path[] = "services/cowrie/honeyfs/etc/issue";
    char issue_content[256];
    snprintf(issue_content,
             sizeof(issue_content),
             "%s Router\nKernel \\r on an \\m (\\l)\n\n",
             profile->name);
    if (write_file(issue_path, issue_content) != 0) {
        log_event_level(LOG_WARN, "Failed to write honeyfs issue");
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "Cowrie banners and honeyfs updated for profile: %s", profile->name);
    log_event_level(LOG_INFO, msg);
    return 0;
}

int morph_router_html(const device_profile_t* profile) {
    if (!profile)
        return -1;

    // Check if theme file exists
    if (!file_exists(profile->router_html_path)) {
        log_event_level(LOG_WARN, "Router HTML theme not found, using default");
        // Create default HTML
        create_dir("services/fake-router-web/html");
        char default_html[2048];
        snprintf(default_html,
                 sizeof(default_html),
                 "<!DOCTYPE html>\n"
                 "<html><head><title>Router Admin - %s</title></head>\n"
                 "<body><h1>Router Administration</h1><p>Device: %s</p></body></html>\n",
                 profile->name,
                 profile->name);
        write_file("services/fake-router-web/html/index.html", default_html);
        return 0;
    }

    // Copy theme to active location
    if (copy_file(profile->router_html_path, "services/fake-router-web/html/index.html") != 0) {
        log_event_level(LOG_ERROR, "Failed to copy router HTML");
        return -1;
    }

    log_event_level(LOG_INFO, "Router HTML theme updated");
    return 0;
}

int morph_camera_html(const device_profile_t* profile) {
    if (!profile)
        return -1;

    // Check if theme file exists
    if (!file_exists(profile->camera_html_path)) {
        log_event_level(LOG_WARN, "Camera HTML theme not found, using default");
        // Create default HTML
        create_dir("services/fake-camera-web/html");
        char default_html[2048];
        snprintf(default_html,
                 sizeof(default_html),
                 "<!DOCTYPE html>\n"
                 "<html><head><title>Camera View - %s</title></head>\n"
                 "<body><h1>CCTV Camera Feed</h1><p>Device: %s</p></body></html>\n",
                 profile->name,
                 profile->name);
        write_file("services/fake-camera-web/html/index.html", default_html);
        return 0;
    }

    // Copy theme to active location
    if (copy_file(profile->camera_html_path, "services/fake-camera-web/html/index.html") != 0) {
        log_event_level(LOG_ERROR, "Failed to copy camera HTML");
        return -1;
    }

    log_event_level(LOG_INFO, "Camera HTML theme updated");
    return 0;
}

/**
 * Phase 1: Network Layer Variation
 * Generates randomized network interfaces, routing tables, ARP cache entries
 */
int morph_phase1_network(const char* base_ip, const char* profile_type) {
    log_event_level(LOG_INFO, "Phase 1: Network Layer Variation");
    return morph_network_config(base_ip ? base_ip : "192.168.1.1", profile_type);
}

/**
 * Phase 2: Filesystem Dynamics
 * Creates varying filesystem structures with randomized timestamps
 *
 * WHY THIS FIX: Before, we generated beautiful fake filesystem data... then threw it away!
 * Like cooking a meal and dumping it in the trash. Now we actually SAVE the outputs
 * so Cowrie can serve them to attackers.
 */
int morph_phase2_filesystem(const char* device_type) {
    log_event_level(LOG_INFO, "Phase 2: Filesystem Dynamics");

    filesystem_snapshot_t* fs = create_filesystem_snapshot("/");
    if (!fs) {
        log_event_level(LOG_WARN, "Failed to create filesystem snapshot");
        return -1;
    }

    // Generate variations based on device type
    // Think of this like rearranging furniture - same room, different layout each time
    generate_directory_variations(fs);
    generate_file_size_variations(fs);
    vary_permissions(fs);

    // Generate a realistic boot time for consistent timestamps
    time_t boot_time = time(NULL) - get_realistic_uptime_seconds();
    generate_random_timestamps(fs, boot_time);

    // Create output directory
    create_dir("build/cowrie-dynamic/bin");

    // Generate and SAVE ls output (this is what attackers see when they type "ls")
    char ls_output[4096];
    generate_ls_output(fs, "/", ls_output, sizeof(ls_output));
    write_file("build/cowrie-dynamic/bin/ls", ls_output);

    // Generate and SAVE find output
    char find_output[4096];
    generate_find_output(fs, NULL, find_output, sizeof(find_output));
    write_file("build/cowrie-dynamic/bin/find", find_output);

    // Generate and SAVE du (disk usage) output
    char du_output[4096];
    generate_du_output(fs, du_output, sizeof(du_output));
    write_file("build/cowrie-dynamic/bin/du", du_output);

    // Log what device type we're emulating
    char msg[256];
    snprintf(msg,
             sizeof(msg),
             "Filesystem morphing complete for: %s",
             device_type ? device_type : "Generic");
    log_event_level(LOG_INFO, msg);

    // Clean up
    free_filesystem_snapshot(fs);

    return 0;
}

/**
 * Phase 3: Process Simulation
 * Generates realistic process lists with varying PIDs
 *
 * WHY THIS FIX: We were calling generate_core_processes() and generate_service_processes()
 * TWICE - once inside create_process_list() and once here. That's like washing dishes
 * that are already clean. Now we only randomize (which IS needed after creation).
 */
int morph_phase3_processes(const char* device_profile) {
    log_event_level(LOG_INFO, "Phase 3: Process Simulation");

    const char* profile = device_profile ? device_profile : "Generic_Router";

    // create_process_list() already calls generate_core_processes() and
    // generate_service_processes() internally - don't call them again!
    process_list_t* procs = create_process_list(profile);
    if (!procs) {
        log_event_level(LOG_WARN, "Failed to create process list");
        return -1;
    }

    // Only randomize - the processes are already generated inside create_process_list()
    // Think of it like: the list is made, now we're just shuffling the cards
    randomize_pids(procs);
    randomize_memory_usage(procs, 128 * 1024); // 128 MB in KB (was wrong unit!)

    // Generate outputs and write to Cowrie's dynamic directory
    char ps_output[4096];
    generate_ps_output(procs, ps_output, sizeof(ps_output));

    // Actually save the output so Cowrie can use it!
    char bin_dir[512];
    snprintf(bin_dir, sizeof(bin_dir), "build/cowrie-dynamic/bin");
    create_dir(bin_dir);
    write_file("build/cowrie-dynamic/bin/ps", ps_output);

    // Also generate ps aux output
    char ps_aux_output[8192];
    generate_ps_aux_output(procs, ps_aux_output, sizeof(ps_aux_output));
    write_file("build/cowrie-dynamic/bin/ps_aux", ps_aux_output);

    // Generate top output
    char top_output[8192];
    generate_top_output(procs, top_output, sizeof(top_output));
    write_file("build/cowrie-dynamic/bin/top", top_output);

    // DEEP BEHAVIOR: Generate uname-specific files for custom uname command
    char uname_a[512];
    snprintf(uname_a,
             sizeof(uname_a),
             "Linux %s %s #1 SMP PREEMPT %s %s GNU/Linux",
             profile,
             "3.10.49",
             "Mon Jan 1 00:00:00 UTC 2026",
             "mips");
    write_file("build/cowrie-dynamic/bin/uname_a", uname_a);
    write_file("build/cowrie-dynamic/bin/uname_r", "3.10.49");
    write_file("build/cowrie-dynamic/bin/uname_m", "mips");

    // NEW: Generate cpuinfo and meminfo
    char cpu_info[4096];
    snprintf(cpu_info,
             sizeof(cpu_info),
             "processor\t: 0\n"
             "model name\t: MIPS 1004Kc V1.0\n"
             "BogoMIPS\t: 500.00\n"
             "wait instruction\t: yes\n"
             "microsecond timers\t: yes\n"
             "tlb_entries\t: 64\n"
             "Hardware\t: %s\n",
             profile);
    write_file("build/cowrie-dynamic/bin/cpuinfo", cpu_info);

    char mem_info[1024];
    snprintf(mem_info,
             sizeof(mem_info),
             "MemTotal:\t131072 kB\n"
             "MemFree:\t%d kB\n"
             "Buffers:\t%d kB\n",
             (rand() % 10000) + 1000,
             (rand() % 2000) + 500);
    write_file("build/cowrie-dynamic/bin/meminfo", mem_info);

    char os_release[512];
    snprintf(os_release,
             sizeof(os_release),
             "PRETTY_NAME=\"%s Embedded Linux\"\n"
             "NAME=\"%s\"\n"
             "ID=%s\n"
             "VERSION_ID=\"1.0\"\n",
             profile,
             profile,
             "cerberus");
    write_file("build/cowrie-dynamic/bin/os-release", os_release);

    // Clean up
    free_process_list(procs);

    log_event_level(LOG_INFO, "Process morphing complete - outputs written to cowrie-dynamic");
    return 0;
}

/**
 * Phase 4: Behavioral Adaptation
 * Adds realistic command execution delays and error messages
 *
 * WHY THIS FIX: Before, we calculated behavior settings but didn't save them anywhere!
 * Now we write a config file that tells Cowrie how to behave (delays, errors, etc.)
 * Think of it like writing stage directions for an actor.
 */
int morph_phase4_behavior(const device_profile_t* profile) {
    log_event_level(LOG_INFO, "Phase 4: Behavioral Adaptation");

    if (!profile)
        return -1;

    // Generate behavioral profiles
    session_behavior_t session = generate_session_behavior(profile);

    // Create a behavior config file that other parts of the system can read
    create_dir("build/cowrie-dynamic");

    char behavior_config[2048];
    snprintf(behavior_config,
             sizeof(behavior_config),
             "# Behavioral configuration - Auto-generated by CERBERUS\n"
             "# Profile: %s\n"
             "# These settings make the honeypot feel like a real slow IoT device\n\n"
             "[delays]\n"
             "min_delay_ms=%u\n"
             "max_delay_ms=%u\n"
             "response_variance=%.2f\n\n"
             "[session]\n"
             "timeout_seconds=%u\n"
             "max_failed_auth=%u\n"
             "timeout_error=%s\n\n"
             "[vulnerabilities]\n"
             "is_vulnerable=%d\n"
             "cves=%s\n",
             profile->name,
             session.min_delay_ms,
             session.max_delay_ms,
             session.response_variance,
             session.timeout_seconds,
             session.failed_auth_attempts,
             get_timeout_error("network"),
             profile->is_vulnerable,
             profile->cves);

    write_file("build/cowrie-dynamic/behavior.conf", behavior_config);

    // Log behavior characteristics
    char msg[256];
    snprintf(msg,
             sizeof(msg),
             "Session behavior configured: %u-%u ms delays, %us timeout",
             session.min_delay_ms,
             session.max_delay_ms,
             session.timeout_seconds);
    log_event_level(LOG_INFO, msg);

    return 0;
}

/**
 * Phase 5: Temporal Evolution
 * Simulates system uptime that grows realistically and accumulates logs
 *
 * WHY THIS FIX: We generated fake uptime, kernel messages, and logs... then forgot to save them!
 * Now we write these to files so when an attacker types "uptime" or "dmesg", they see
 * a consistent, realistic system that appears to have been running for days/months.
 */
int morph_phase5_temporal(void) {
    log_event_level(LOG_INFO, "Phase 5: Temporal Evolution");

    // Get a realistic boot time (system appears to have been running 1-365 days)
    time_t boot_time = get_realistic_boot_time();

    system_state_t* state = create_initial_system_state(boot_time);
    if (!state) {
        log_event_level(LOG_WARN, "Failed to create system state");
        return -1;
    }

    // Simulate the system aging - adds fake log entries, service restarts, etc.
    simulate_system_aging(state);
    accumulate_log_files(state);

    // Create output directories
    create_dir("build/cowrie-dynamic/bin");
    create_dir("build/cowrie-dynamic/var/log");

    // Generate and SAVE uptime output
    // This is what attackers see when they type "uptime"
    char uptime_output[512];
    generate_system_uptime(state, uptime_output, sizeof(uptime_output));
    write_file("build/cowrie-dynamic/bin/uptime", uptime_output);

    // Generate and SAVE kernel messages (dmesg output)
    char dmesg_output[4096];
    generate_kernel_messages(state, dmesg_output, sizeof(dmesg_output));
    write_file("build/cowrie-dynamic/bin/dmesg", dmesg_output);

    // Generate and SAVE syslog
    char syslog_output[8192];
    generate_syslog(state, syslog_output, sizeof(syslog_output));
    write_file("build/cowrie-dynamic/var/log/syslog", syslog_output);

    // Save the boot time so other phases can use it for consistent timestamps
    char boot_info[256];
    snprintf(boot_info,
             sizeof(boot_info),
             "boot_time=%ld\nuptime_seconds=%u\nkernel_version=%s\n",
             boot_time,
             state->uptime_seconds,
             state->kernel_version);
    write_file("build/cowrie-dynamic/boot_info.conf", boot_info);

    // Log what we did
    char msg[256];
    snprintf(msg,
             sizeof(msg),
             "Temporal morphing complete - system appears %u days old",
             state->uptime_seconds / 86400);
    log_event_level(LOG_INFO, msg);

    // Clean up
    free_system_state(state);

    return 0;
}

/**
 * Setup the fake filesystem (honeyfs) for Cowrie
 *
 * WHY THIS EXISTS: Every time we morph to a new device, we need to update
 * the fake filesystem to match. A router should have router files, a camera
 * should have camera files. This calls our setup script to rebuild honeyfs.
 *
 * Think of it like changing the set decorations when a play moves to a new scene.
 */
int setup_honeyfs_for_profile(const char* device_name, const char* profile_type) {
    log_event_level(LOG_INFO, "Setting up honeyfs for device profile...");

    // Build the command to run our setup script
    // The script creates a realistic fake filesystem based on device type
    char* argv[] = {"./scripts/setup_honeyfs.sh",
                    "services/cowrie/honeyfs",
                    (char*)profile_type,
                    (char*)device_name,
                    NULL};

    // Check if script exists before trying to run it
    if (file_exists("scripts/setup_honeyfs.sh")) {
        int result = execute_command_safely(argv[0], argv);
        if (result == 0) {
            char msg[256];
            snprintf(msg, sizeof(msg), "Honeyfs configured for %s (%s)", device_name, profile_type);
            log_event_level(LOG_INFO, msg);
            return 0;
        } else {
            log_event_level(LOG_WARN, "Honeyfs setup script returned non-zero");
        }
    } else {
        log_event_level(LOG_WARN, "Honeyfs setup script not found - using existing filesystem");
    }

    return -1;
}

/**
 * Determine if a profile is a camera or router based on its name
 *
 * WHY: Different devices have different files and configurations.
 * We need to know what type so we create the right fake filesystem.
 */
const char* get_profile_type(const char* device_name) {
    if (!device_name)
        return "router";

    // Camera keywords
    if (strstr(device_name, "Camera") || strstr(device_name, "DS-2CD") ||
        strstr(device_name, "Hikvision") || strstr(device_name, "Dahua") ||
        strstr(device_name, "IPC") || strstr(device_name, "DVR") || strstr(device_name, "NVR")) {
        return "camera";
    }

    // Default to router
    return "router";
}

/**
 * Phase 6: Quorum-Based Adaptation
 * Detects coordinated attacks and triggers adaptive responses
 *
 * WHY THIS FIX: Before, this phase just logged "monitoring enabled" and did NOTHING!
 * It's like a security guard who says "I'm watching" but has their eyes closed.
 *
 * Now it actually checks for emergency signals from the quorum engine and responds.
 * Think of it like checking your voicemail - the quorum engine leaves messages
 * (signal files), and this phase reads them and takes action.
 */
int morph_phase6_quorum(void) {
    log_event_level(LOG_INFO, "Phase 6: Quorum-Based Adaptation");

    // Check if there's an emergency morph signal from the quorum engine
    // The quorum engine writes this file when it detects coordinated attacks
    const char* emergency_signal = "build/signals/emergency_morph.signal";

    if (file_exists(emergency_signal)) {
        log_event_level(LOG_WARN, "ALERT: Emergency morph signal detected!");

        // Read the signal to see why we're being asked to morph
        char signal_content[512];
        if (read_file(emergency_signal, signal_content, sizeof(signal_content)) > 0) {
            // Log what triggered the emergency
            if (strstr(signal_content, "coordinated_attack")) {
                log_event_level(LOG_WARN, "Reason: Coordinated attack detected by quorum engine");
            }
        }

        // Delete the signal file so we don't process it again
        // It's like throwing away a "call me back" note after you've called
        remove(emergency_signal);

        log_event_level(LOG_INFO, "Emergency signal processed and cleared");
    } else {
        log_event_level(LOG_INFO, "No emergency signals - normal operation");
    }

    // Check the morph frequency configuration
    // The quorum engine can tell us to morph more often when under attack
    const char* freq_config = "build/signals/morph_frequency.conf";

    if (file_exists(freq_config)) {
        char freq_content[256];
        if (read_file(freq_config, freq_content, sizeof(freq_content)) > 0) {
            // Parse the frequency (simple approach - look for the number)
            char* freq_line = strstr(freq_content, "frequency_minutes=");
            if (freq_line) {
                int freq = atoi(freq_line + 18); // Skip "frequency_minutes="
                if (freq > 0 && freq < 60) {
                    char msg[256];
                    snprintf(msg,
                             sizeof(msg),
                             "HIGH ALERT MODE: Quorum requests morphing every %d minutes",
                             freq);
                    log_event_level(LOG_WARN, msg);
                }
            }
        }
    }

    // Check the attacker blocklist to see who's being naughty
    const char* blocklist = "build/signals/attacker_blocklist.txt";

    if (file_exists(blocklist)) {
        // Count how many attackers are on the list
        char blocklist_content[4096];
        if (read_file(blocklist, blocklist_content, sizeof(blocklist_content)) > 0) {
            int attacker_count = 0;
            char* line = blocklist_content;
            while ((line = strchr(line, '\n')) != NULL) {
                attacker_count++;
                line++;
            }

            if (attacker_count > 0) {
                char msg[256];
                snprintf(msg,
                         sizeof(msg),
                         "Blocklist active: %d attacker(s) receiving fake errors",
                         attacker_count);
                log_event_level(LOG_INFO, msg);
            }
        }
    }

    log_event_level(LOG_INFO, "Quorum adaptation check complete");
    return 0;
}

/**
 * Setup device-specific filesystem based on profile type
 *
 * WHY THIS FIX: The old code had a buffer overflow risk - we were trying to
 * stuff too many characters into a 512-byte buffer. Also, system() is a
 * security risk (attackers could inject commands). Now we use safe C functions.
 */
int setup_device_filesystem(const char* device_name) {
    if (!device_name) {
        return -1;
    }

    // Determine if it's a router or camera based on profile name
    // Think of this like sorting mail - router stuff goes to router box, camera to camera box
    int is_router = 1; // Default to router
    if (strstr(device_name, "Camera") || strstr(device_name, "DS-2CD") ||
        strstr(device_name, "Hikvision") || strstr(device_name, "Dahua") ||
        strstr(device_name, "IPC")) {
        is_router = 0;
    }

    const char* profile_type = is_router ? "router" : "camera";

    // Create the destination directory if it doesn't exist
    create_dir("services/cowrie/honeyfs/etc");

    // Copy key files individually instead of using dangerous system() call
    // This is like moving items one by one instead of dumping the whole drawer
    char src_file[512];
    char dst_file[512];

    // List of files to copy from profile to honeyfs
    const char* profile_files[] = {
        "passwd", "shadow", "group", "hostname", "hosts", "resolv.conf", "issue", "motd", NULL};

    for (int i = 0; profile_files[i] != NULL; i++) {
        snprintf(src_file,
                 sizeof(src_file),
                 "services/cowrie/honeyfs-profiles/%s/etc/%s",
                 profile_type,
                 profile_files[i]);
        snprintf(dst_file, sizeof(dst_file), "services/cowrie/honeyfs/etc/%s", profile_files[i]);

        // Only copy if source exists (no error if it doesn't)
        if (file_exists(src_file)) {
            copy_file(src_file, dst_file);
        }
    }

    char msg[256];
    snprintf(
        msg, sizeof(msg), "Device filesystem configured for: %s (%s)", device_name, profile_type);
    log_event_level(LOG_INFO, msg);

    return 0;
}

int morph_device(void) {
    if (profile_count == 0) {
        log_event_level(LOG_ERROR, "No profiles loaded");
        return -1;
    }

    // Rotate to next profile
    int next_index = (current_profile_index + 1) % profile_count;
    if (current_profile_index < 0) {
        next_index = 0;
    }

    device_profile_t* new_profile = get_profile(next_index);
    if (!new_profile) {
        log_event_level(LOG_ERROR, "Invalid profile index");
        return -1;
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "Morphing to profile: %s", new_profile->name);
    log_event_level(LOG_INFO, msg);

    // Apply morphing - Core functions
    int result = 0;
    result += morph_cowrie_banners(new_profile);
    result += morph_router_html(new_profile);
    result += morph_camera_html(new_profile);

    // Setup device-specific filesystem using our honeyfs script
    // This creates a realistic fake filesystem that matches the device type
    const char* profile_type = get_profile_type(new_profile->name);
    setup_honeyfs_for_profile(new_profile->name, profile_type);

    // Also run the old setup for backwards compatibility
    setup_device_filesystem(new_profile->name);

    // Apply all 6 phases of morphing
    if (result == 0) {
        log_event_level(LOG_INFO, "=== Starting 6-Phase Morphing Cycle ===");

        // Phase 1: Network Layer Variation
        result += morph_phase1_network("192.168.1.1", profile_type);

        // Phase 2: Filesystem Dynamics
        result += morph_phase2_filesystem(new_profile->name);

        // Phase 3: Process Simulation
        result += morph_phase3_processes(new_profile->name);

        // Phase 4: Behavioral Adaptation
        result += morph_phase4_behavior(new_profile);

        // Phase 5: Temporal Evolution
        result += morph_phase5_temporal();

        // Phase 6: Quorum-Based Adaptation
        result += morph_phase6_quorum();

        log_event_level(LOG_INFO, "=== 6-Phase Morphing Cycle Complete ===");
    }

    if (result == 0) {
        current_profile_index = next_index;
        save_current_profile(state_file_path);

        // PERSISTENCE: Stash the generated artifacts for this profile index
        // so returning IPs can see their assigned profile even after global rotation
        char stash_cmd[512];
        snprintf(stash_cmd,
                 sizeof(stash_cmd),
                 "mkdir -p build/cowrie-dynamic/profiles/%d && cp -r build/cowrie-dynamic/bin "
                 "build/cowrie-dynamic/profiles/%d/",
                 current_profile_index,
                 current_profile_index);
        system(stash_cmd);

        snprintf(msg, sizeof(msg), "Successfully morphed to profile: %s", new_profile->name);
        log_event_level(LOG_INFO, msg);
        log_to_file("build/morph-events.log", msg);
    } else {
        log_event_level(LOG_ERROR, "Morphing failed");
    }

    return result;
}

int save_current_profile(const char* state_file) {
    if (current_profile_index < 0) {
        return -1;
    }

    char state_content[128];
    snprintf(state_content, sizeof(state_content), "current_profile=%d\n", current_profile_index);

    return write_file(state_file, state_content);
}

int load_current_profile(const char* state_file) {
    if (!file_exists(state_file)) {
        current_profile_index = 0; // Default to first profile
        return 0;
    }

    char value[32];
    if (read_config_value(state_file, "current_profile", value, sizeof(value)) == 0) {
        current_profile_index = atoi(value);
        if (current_profile_index < 0 || current_profile_index >= profile_count) {
            current_profile_index = 0;
        }
        return 0;
    }

    current_profile_index = 0;
    return 0;
}

int init_morph_engine(const char* config_file, const char* state_file) {
    if (state_file) {
        strncpy(state_file_path, state_file, MAX_PATH_SIZE - 1);
    }

    // WHY THIS FIX: The code assumed directories exist, but they might not!
    // It's like trying to put groceries in a refrigerator that hasn't been delivered.
    // Now we create ALL required directories at startup, so nothing fails later.
    log_event_level(LOG_INFO, "Initializing directory structure...");

    // Build output directories
    create_dir("build");
    create_dir("build/cowrie-dynamic");
    create_dir("build/cowrie-dynamic/bin");
    create_dir("build/cowrie-dynamic/sbin");
    create_dir("build/cowrie-dynamic/usr/bin");
    create_dir("build/cowrie-dynamic/var/log");

    // Service directories for Cowrie
    create_dir("services/cowrie/etc");
    create_dir("services/cowrie/logs");
    create_dir("services/cowrie/honeyfs");
    create_dir("services/cowrie/honeyfs/etc");
    create_dir("services/cowrie/honeyfs/proc");
    create_dir("services/cowrie/honeyfs/var/log");

    // Profile-specific honeyfs directories (router and camera have different files)
    create_dir("services/cowrie/honeyfs-profiles");
    create_dir("services/cowrie/honeyfs-profiles/router/etc");
    create_dir("services/cowrie/honeyfs-profiles/camera/etc");

    // Web service directories
    create_dir("services/fake-router-web/html");
    create_dir("services/fake-router-web/html/themes");
    create_dir("services/fake-router-web/logs");
    create_dir("services/fake-camera-web/html");
    create_dir("services/fake-camera-web/html/themes");
    create_dir("services/fake-camera-web/logs");

    // RTSP service directory
    create_dir("services/rtsp/logs");

    log_event_level(LOG_INFO, "Directory structure initialized");

    // Load profiles
    if (load_profiles(config_file ? config_file : "profiles.conf") < 0) {
        log_event_level(LOG_WARN, "Using default profiles");
    }

    // Load current profile state
    load_current_profile(state_file_path);

    return 0;
}

// Session variation functions - generate unique values for each session
void generate_random_mac(char* mac_out, size_t size, const char* vendor_prefix) {
    // Generate random MAC address with vendor prefix
    // vendor_prefix should be like "14:cc:20" (first 3 octets)
    // NOTE: We don't call srand() here anymore - it's called once in main()
    // Calling it multiple times would reset the sequence and reduce randomness!
    snprintf(mac_out,
             size,
             "%s:%02x:%02x:%02x",
             vendor_prefix,
             rand() % 256,
             rand() % 256,
             rand() % 256);
}

int generate_session_variations(const device_profile_t* profile) {
    if (!profile)
        return -1;

    // Generate random MAC address based on vendor prefix
    char session_mac[MAX_MAC_ADDR];
    generate_random_mac(session_mac, sizeof(session_mac), profile->mac_address);

    // Calculate random uptime (1-365 days in seconds)
    // NOTE: srand() removed - already called once in main()
    int uptime_seconds = (rand() % (365 * 24 * 3600)) + (24 * 3600);

    // Add small random variation to memory (±10%)
    int memory_variation = profile->memory_mb + ((rand() % 21) - 10) * profile->memory_mb / 100;
    if (memory_variation < 1)
        memory_variation = profile->memory_mb;

    char msg[512];
    snprintf(msg,
             sizeof(msg),
             "Session variations: MAC=%s, Uptime=%d days, Memory=%dMB",
             session_mac,
             uptime_seconds / (24 * 3600),
             memory_variation);
    log_event_level(LOG_INFO, msg);

    // These values would be written to Cowrie's txtcmds or similar
    // For now, just log them - full implementation would write to files
    return 0;
}

int main(int argc, char* argv[]) {
    printf("Bio-Adaptive IoT Honeynet Morphing Engine\n");

    // IMPORTANT: Seed random number generator ONCE at program start
    // WHY THIS FIX: Before, srand() was called in multiple places throughout the code.
    // Each call resets the random sequence! If two calls happen in the same second,
    // you get identical "random" numbers. By seeding once with time + PID, we get
    // truly varied results across the entire program run.
    // Think of it like shuffling a deck of cards ONCE before the game, not re-shuffling
    // every time you draw a card.
    srand((unsigned int)(time(NULL) ^ getpid()));

    // Initialize with config file if provided
    const char* config_file = (argc > 1) ? argv[1] : NULL;
    const char* state_file = (argc > 2) ? argv[2] : NULL;

    init_morph_engine(config_file, state_file);

    time_t now = time(NULL);
    printf("Morph event: Rotating device profile at %s", ctime(&now));

    int result = morph_device();

    // Generate session-specific variations
    if (result == 0) {
        device_profile_t* current = get_profile(get_current_profile_index());
        if (current) {
            generate_session_variations(current);
        }
    }

    return (result == 0) ? 0 : 1;
}
