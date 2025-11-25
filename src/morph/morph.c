#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "morph.h"
#include "utils.h"
#include "network.h"
#include "filesystem.h"
#include "processes.h"
#include "behavior.h"
#include "temporal.h"
#include "quorum_adapt.h"

// Global state
static device_profile_t profiles[MAX_PROFILES];
static int profile_count = 0;
static int current_profile_index = -1;
static char state_file_path[MAX_PATH_SIZE] = "build/morph-state.txt";

// Forward declaration
static int create_default_profiles(void);

// External function declaration for Phase 1 (Network)
extern int morph_network_config(const char* base_ip);

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
                snprintf(current->router_html_path, MAX_PATH_SIZE, "services/fake-router-web/html/index.html");
                snprintf(current->camera_html_path, MAX_PATH_SIZE, "services/fake-camera-web/html/index.html");
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
    
    // Profile 1: TP-Link Archer C7 Router
    strcpy(profiles[0].name, "TP-Link_Archer_C7");
    strcpy(profiles[0].ssh_banner, "SSH-2.0-dropbear_2017.75");
    strcpy(profiles[0].telnet_banner, "Welcome to TP-Link Router");
    strcpy(profiles[0].router_html_path, "services/fake-router-web/html/themes/tplink.html");
    strcpy(profiles[0].camera_html_path, "services/fake-camera-web/html/themes/default.html");
    strcpy(profiles[0].kernel_version, "3.10.49");
    strcpy(profiles[0].arch, "mips");
    strcpy(profiles[0].mac_address, "14:cc:20");
    profiles[0].memory_mb = 128;
    profiles[0].cpu_mhz = 720;
    
    // Profile 2: D-Link DIR-615 Router
    strcpy(profiles[1].name, "D-Link_DIR-615");
    strcpy(profiles[1].ssh_banner, "SSH-2.0-OpenSSH_6.7p1");
    strcpy(profiles[1].telnet_banner, "D-Link System");
    strcpy(profiles[1].router_html_path, "services/fake-router-web/html/themes/dlink.html");
    strcpy(profiles[1].camera_html_path, "services/fake-camera-web/html/themes/default.html");
    strcpy(profiles[1].kernel_version, "2.6.30");
    strcpy(profiles[1].arch, "mips");
    strcpy(profiles[1].mac_address, "00:1b:11");
    profiles[1].memory_mb = 32;
    profiles[1].cpu_mhz = 400;
    
    // Profile 3: Netgear R7000 Router
    strcpy(profiles[2].name, "Netgear_R7000");
    strcpy(profiles[2].ssh_banner, "SSH-2.0-dropbear_2015.71");
    strcpy(profiles[2].telnet_banner, "NETGEAR ReadyNAS");
    strcpy(profiles[2].router_html_path, "services/fake-router-web/html/themes/netgear.html");
    strcpy(profiles[2].camera_html_path, "services/fake-camera-web/html/themes/default.html");
    strcpy(profiles[2].kernel_version, "2.6.36.4brcmarm");
    strcpy(profiles[2].arch, "armv7l");
    strcpy(profiles[2].mac_address, "a0:63:91");
    profiles[2].memory_mb = 256;
    profiles[2].cpu_mhz = 1000;
    
    // Profile 4: Hikvision DS-2CD2 Camera
    strcpy(profiles[3].name, "Hikvision_DS-2CD2");
    strcpy(profiles[3].ssh_banner, "SSH-2.0-OpenSSH_5.8p1");
    strcpy(profiles[3].telnet_banner, "Hikvision IP Camera");
    strcpy(profiles[3].router_html_path, "services/fake-router-web/html/themes/default.html");
    strcpy(profiles[3].camera_html_path, "services/fake-camera-web/html/themes/hikvision.html");
    strcpy(profiles[3].kernel_version, "3.0.8");
    strcpy(profiles[3].arch, "armv7l");
    strcpy(profiles[3].mac_address, "44:19:b6");
    profiles[3].memory_mb = 64;
    profiles[3].cpu_mhz = 600;
    
    // Profile 5: Dahua IPC-HDW Camera
    strcpy(profiles[4].name, "Dahua_IPC-HDW");
    strcpy(profiles[4].ssh_banner, "SSH-2.0-OpenSSH_6.0p1");
    strcpy(profiles[4].telnet_banner, "Dahua Technology");
    strcpy(profiles[4].router_html_path, "services/fake-router-web/html/themes/default.html");
    strcpy(profiles[4].camera_html_path, "services/fake-camera-web/html/themes/dahua.html");
    strcpy(profiles[4].kernel_version, "3.4.35");
    strcpy(profiles[4].arch, "armv7l");
    strcpy(profiles[4].mac_address, "00:12:16");
    profiles[4].memory_mb = 128;
    profiles[4].cpu_mhz = 800;
    
    // Profile 6: Generic Router (fallback)
    strcpy(profiles[5].name, "Generic_Router");
    strcpy(profiles[5].ssh_banner, "SSH-2.0-OpenSSH_7.4");
    strcpy(profiles[5].telnet_banner, "Welcome to Router Admin");
    strcpy(profiles[5].router_html_path, "services/fake-router-web/html/themes/generic.html");
    strcpy(profiles[5].camera_html_path, "services/fake-camera-web/html/themes/default.html");
    strcpy(profiles[5].kernel_version, "4.4.0");
    strcpy(profiles[5].arch, "armv7l");
    strcpy(profiles[5].mac_address, "00:11:22");
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
    if (!profile) return -1;
    
    char cowrie_cfg_path[] = "services/cowrie/etc/cowrie.cfg";
    char cowrie_cfg_local[] = "services/cowrie/etc/cowrie.cfg.local";
    
    // Create cowrie config directory if it doesn't exist
    create_dir("services/cowrie/etc");
    
    // Cowrie reads configuration from cowrie.cfg (main file)
    // The [shell] section controls uname output - this is what affects command responses
    // Environment variables take precedence, but we also write the config file
    char config_content[4096];
    snprintf(config_content, sizeof(config_content),
        "# Cowrie configuration - Auto-generated by CERBERUS morph engine\n"
        "# Profile: %s\n"
        "# This file is read by Cowrie at startup for uname/hostname commands\n\n"
        "[output_jsonlog]\n"
        "enabled = true\n"
        "logfile = log/cowrie.json\n\n"
        "[output_textlog]\n"
        "enabled = true\n"
        "logfile = log/cowrie.log\n\n"
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
        "hostname = %s\n\n"
        "[shell]\n"
        "# Shell configuration - controls uname and hostname command outputs\n"
        "# These values are what Cowrie returns for uname -a, uname -r, hostname, etc.\n"
        "kernel_name = Linux\n"
        "kernel_version = %s\n"
        "kernel_build_string = #1 SMP PREEMPT %s\n"
        "hardware_platform = %s\n"
        "operating_system = GNU/Linux\n"
        "hostname = %s\n",
        profile->name,
        profile->ssh_banner,
        profile->ssh_banner,
        profile->telnet_banner,
        profile->name,
        profile->kernel_version,
        profile->arch,
        profile->arch,
        profile->name);
    
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
    snprintf(env_content, sizeof(env_content),
        "# Cowrie environment variables - Auto-generated by CERBERUS morph engine\n"
        "# Profile: %s\n"
        "# These override ALL config file values for uname/hostname commands\n"
        "COWRIE_SHELL_KERNEL_NAME=Linux\n"
        "COWRIE_SHELL_KERNEL_VERSION=%s\n"
        "COWRIE_SHELL_KERNEL_BUILD_STRING=#1 SMP PREEMPT %s\n"
        "COWRIE_SHELL_HARDWARE_PLATFORM=%s\n"
        "COWRIE_SHELL_OPERATING_SYSTEM=GNU/Linux\n"
        "COWRIE_HONEYPOT_HOSTNAME=%s\n",
        profile->name,
        profile->kernel_version,
        profile->arch,
        profile->arch,
        profile->name);
    
    if (write_file(env_file_path, env_content) != 0) {
        log_event_level(LOG_WARN, "Failed to write Cowrie env file");
    }
    
    // Update honeyfs (fake filesystem) files for shell responses
    // Create honeyfs directory structure if it doesn't exist
    create_dir("services/cowrie/honeyfs/etc");
    create_dir("services/cowrie/honeyfs/proc");
    
    // Update /etc/hostname (what 'hostname' command reads)
    char hostname_path[] = "services/cowrie/honeyfs/etc/hostname";
    if (write_file(hostname_path, profile->name) != 0) {
        log_event_level(LOG_WARN, "Failed to write honeyfs hostname");
    }
    
    // Update /proc/version (what 'uname -a' reads)
    char proc_version_path[] = "services/cowrie/honeyfs/proc/version";
    char proc_version[512];
    snprintf(proc_version, sizeof(proc_version),
        "Linux version %s (root@localhost) (gcc version 4.6.3) #1 SMP PREEMPT %s",
        profile->kernel_version, profile->arch);
    if (write_file(proc_version_path, proc_version) != 0) {
        log_event_level(LOG_WARN, "Failed to write honeyfs proc/version");
    }
    
    // Update /etc/issue (login banner)
    char issue_path[] = "services/cowrie/honeyfs/etc/issue";
    char issue_content[256];
    snprintf(issue_content, sizeof(issue_content),
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
    if (!profile) return -1;
    
    // Check if theme file exists
    if (!file_exists(profile->router_html_path)) {
        log_event_level(LOG_WARN, "Router HTML theme not found, using default");
        // Create default HTML
        create_dir("services/fake-router-web/html");
        char default_html[2048];
        snprintf(default_html, sizeof(default_html),
            "<!DOCTYPE html>\n"
            "<html><head><title>Router Admin - %s</title></head>\n"
            "<body><h1>Router Administration</h1><p>Device: %s</p></body></html>\n",
            profile->name, profile->name);
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
    if (!profile) return -1;
    
    // Check if theme file exists
    if (!file_exists(profile->camera_html_path)) {
        log_event_level(LOG_WARN, "Camera HTML theme not found, using default");
        // Create default HTML
        create_dir("services/fake-camera-web/html");
        char default_html[2048];
        snprintf(default_html, sizeof(default_html),
            "<!DOCTYPE html>\n"
            "<html><head><title>Camera View - %s</title></head>\n"
            "<body><h1>CCTV Camera Feed</h1><p>Device: %s</p></body></html>\n",
            profile->name, profile->name);
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
int morph_phase1_network(const char* base_ip) {
    log_event_level(LOG_INFO, "Phase 1: Network Layer Variation");
    return morph_network_config(base_ip ? base_ip : "192.168.1.1");
}

/**
 * Phase 2: Filesystem Dynamics
 * Creates varying filesystem structures with randomized timestamps
 */
int morph_phase2_filesystem(const char* device_type) {
    (void)device_type;  // Reserved for future device-specific filesystem variations
    log_event_level(LOG_INFO, "Phase 2: Filesystem Dynamics");
    
    filesystem_snapshot_t* fs = create_filesystem_snapshot("/");
    if (!fs) {
        log_event_level(LOG_WARN, "Failed to create filesystem snapshot");
        return -1;
    }
    
    // Generate variations based on device type
    generate_directory_variations(fs);
    generate_file_size_variations(fs);
    
    // Generate outputs for Cowrie
    char ls_output[2048];
    generate_ls_output(fs, "/", ls_output, sizeof(ls_output));
    
    // Clean up
    free_filesystem_snapshot(fs);
    
    log_event_level(LOG_INFO, "Filesystem morphing complete");
    return 0;
}

/**
 * Phase 3: Process Simulation
 * Generates realistic process lists with varying PIDs
 */
int morph_phase3_processes(const char* device_profile) {
    log_event_level(LOG_INFO, "Phase 3: Process Simulation");
    
    process_list_t* procs = create_process_list(device_profile ? device_profile : "Generic_Router");
    if (!procs) {
        log_event_level(LOG_WARN, "Failed to create process list");
        return -1;
    }
    
    // Generate variations
    generate_core_processes(procs, device_profile);
    generate_service_processes(procs, device_profile);
    randomize_pids(procs);
    randomize_memory_usage(procs, 128);  // MB
    
    // Generate outputs
    char ps_output[4096];
    generate_ps_output(procs, ps_output, sizeof(ps_output));
    
    // Clean up
    free_process_list(procs);
    
    log_event_level(LOG_INFO, "Process morphing complete");
    return 0;
}

/**
 * Phase 4: Behavioral Adaptation
 * Adds realistic command execution delays and error messages
 */
int morph_phase4_behavior(const char* device_profile) {
    log_event_level(LOG_INFO, "Phase 4: Behavioral Adaptation");
    
    // Generate behavioral profiles for various commands
    session_behavior_t session = generate_session_behavior(device_profile ? device_profile : "Generic_Router");
    
    // Log behavior characteristics
    char msg[256];
    snprintf(msg, sizeof(msg), "Session behavior: %d-%d ms delays", session.min_delay_ms, session.max_delay_ms);
    log_event_level(LOG_INFO, msg);
    
    log_event_level(LOG_INFO, "Behavioral morphing complete");
    return 0;
}

/**
 * Phase 5: Temporal Evolution
 * Simulates system uptime that grows realistically and accumulates logs
 */
int morph_phase5_temporal(void) {
    log_event_level(LOG_INFO, "Phase 5: Temporal Evolution");
    
    time_t boot_time = time(NULL) - (rand() % (365 * 24 * 3600));  // 0-365 days ago
    system_state_t* state = create_initial_system_state(boot_time);
    if (!state) {
        log_event_level(LOG_WARN, "Failed to create system state");
        return -1;
    }
    
    // Simulate aging
    simulate_system_aging(state);
    
    // Generate uptime output
    char uptime_output[256];
    generate_system_uptime(state, uptime_output, sizeof(uptime_output));
    
    // Clean up
    free_system_state(state);
    
    log_event_level(LOG_INFO, "Temporal morphing complete");
    return 0;
}

/**
 * Phase 6: Quorum-Based Adaptation
 * Detects coordinated attacks and triggers adaptive responses
 */
int morph_phase6_quorum(void) {
    log_event_level(LOG_INFO, "Phase 6: Quorum-Based Adaptation");
    
    // This phase is handled by the quorum binary
    // The morph engine just logs that it would be applied
    log_event_level(LOG_INFO, "Quorum adaptation monitoring enabled");
    
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
    
    // Apply all 6 phases of morphing
    if (result == 0) {
        log_event_level(LOG_INFO, "=== Starting 6-Phase Morphing Cycle ===");
        
        // Phase 1: Network Layer Variation
        result += morph_phase1_network("192.168.1.1");
        
        // Phase 2: Filesystem Dynamics
        result += morph_phase2_filesystem(new_profile->name);
        
        // Phase 3: Process Simulation
        result += morph_phase3_processes(new_profile->name);
        
        // Phase 4: Behavioral Adaptation
        result += morph_phase4_behavior(new_profile->name);
        
        // Phase 5: Temporal Evolution
        result += morph_phase5_temporal();
        
        // Phase 6: Quorum-Based Adaptation
        result += morph_phase6_quorum();
        
        log_event_level(LOG_INFO, "=== 6-Phase Morphing Cycle Complete ===");
    }
    
    if (result == 0) {
        current_profile_index = next_index;
        save_current_profile(state_file_path);
        
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
    
    // Load profiles
    if (load_profiles(config_file ? config_file : "build/profiles.conf") < 0) {
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
    srand(time(NULL) + getpid());
    snprintf(mac_out, size, "%s:%02x:%02x:%02x", 
             vendor_prefix,
             rand() % 256,
             rand() % 256,
             rand() % 256);
}

int generate_session_variations(const device_profile_t* profile) {
    if (!profile) return -1;
    
    // Generate random MAC address based on vendor prefix
    char session_mac[MAX_MAC_ADDR];
    generate_random_mac(session_mac, sizeof(session_mac), profile->mac_address);
    
    // Calculate random uptime (1-365 days in seconds)
    srand(time(NULL) + getpid());
    int uptime_seconds = (rand() % (365 * 24 * 3600)) + (24 * 3600);
    
    // Add small random variation to memory (±10%)
    int memory_variation = profile->memory_mb + ((rand() % 21) - 10) * profile->memory_mb / 100;
    if (memory_variation < 1) memory_variation = profile->memory_mb;
    
    char msg[512];
    snprintf(msg, sizeof(msg), 
             "Session variations: MAC=%s, Uptime=%d days, Memory=%dMB",
             session_mac, uptime_seconds / (24 * 3600), memory_variation);
    log_event_level(LOG_INFO, msg);
    
    // These values would be written to Cowrie's txtcmds or similar
    // For now, just log them - full implementation would write to files
    return 0;
}

int main(int argc, char* argv[]) {
    printf("Bio-Adaptive IoT Honeynet Morphing Engine\n");
    
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
