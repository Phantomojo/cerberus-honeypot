#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "morph.h"
#include "utils.h"

// Global state
static device_profile_t profiles[MAX_PROFILES];
static int profile_count = 0;
static int current_profile_index = -1;
static char state_file_path[MAX_PATH_SIZE] = "build/morph-state.txt";

// Forward declaration
static int create_default_profiles(void);

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
                strncpy(current->name, line + 1, MAX_PROFILE_NAME - 1);
                current->name[strlen(current->name) - 1] = '\0'; // Remove closing ]
                current->name[MAX_PROFILE_NAME - 1] = '\0';
                
                // Set defaults
                snprintf(current->ssh_banner, MAX_BANNER_SIZE, "SSH-2.0-OpenSSH_7.4");
                snprintf(current->telnet_banner, MAX_BANNER_SIZE, "Welcome to device");
                snprintf(current->router_html_path, MAX_PATH_SIZE, "services/fake-router-web/html/index.html");
                snprintf(current->camera_html_path, MAX_PATH_SIZE, "services/fake-camera-web/html/index.html");
                
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
    
    // Profile 2: D-Link DIR-615 Router
    strcpy(profiles[1].name, "D-Link_DIR-615");
    strcpy(profiles[1].ssh_banner, "SSH-2.0-OpenSSH_6.7p1");
    strcpy(profiles[1].telnet_banner, "D-Link System");
    strcpy(profiles[1].router_html_path, "services/fake-router-web/html/themes/dlink.html");
    strcpy(profiles[1].camera_html_path, "services/fake-camera-web/html/themes/default.html");
    
    // Profile 3: Netgear R7000 Router
    strcpy(profiles[2].name, "Netgear_R7000");
    strcpy(profiles[2].ssh_banner, "SSH-2.0-dropbear_2015.71");
    strcpy(profiles[2].telnet_banner, "NETGEAR ReadyNAS");
    strcpy(profiles[2].router_html_path, "services/fake-router-web/html/themes/netgear.html");
    strcpy(profiles[2].camera_html_path, "services/fake-camera-web/html/themes/default.html");
    
    // Profile 4: Hikvision DS-2CD2 Camera
    strcpy(profiles[3].name, "Hikvision_DS-2CD2");
    strcpy(profiles[3].ssh_banner, "SSH-2.0-OpenSSH_5.8p1");
    strcpy(profiles[3].telnet_banner, "Hikvision IP Camera");
    strcpy(profiles[3].router_html_path, "services/fake-router-web/html/themes/default.html");
    strcpy(profiles[3].camera_html_path, "services/fake-camera-web/html/themes/hikvision.html");
    
    // Profile 5: Dahua IPC-HDW Camera
    strcpy(profiles[4].name, "Dahua_IPC-HDW");
    strcpy(profiles[4].ssh_banner, "SSH-2.0-OpenSSH_6.0p1");
    strcpy(profiles[4].telnet_banner, "Dahua Technology");
    strcpy(profiles[4].router_html_path, "services/fake-router-web/html/themes/default.html");
    strcpy(profiles[4].camera_html_path, "services/fake-camera-web/html/themes/dahua.html");
    
    // Profile 6: Generic Router (fallback)
    strcpy(profiles[5].name, "Generic_Router");
    strcpy(profiles[5].ssh_banner, "SSH-2.0-OpenSSH_7.4");
    strcpy(profiles[5].telnet_banner, "Welcome to Router Admin");
    strcpy(profiles[5].router_html_path, "services/fake-router-web/html/themes/generic.html");
    strcpy(profiles[5].camera_html_path, "services/fake-camera-web/html/themes/default.html");
    
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
    
    // Cowrie uses cowrie.cfg.local to override default settings
    // This file is merged with the main config
    char config_content[4096];
    snprintf(config_content, sizeof(config_content),
        "# Cowrie local configuration - Auto-generated by CERBERUS morph engine\n"
        "# Profile: %s\n"
        "# This file overrides default Cowrie settings\n\n"
        "[output_jsonlog]\n"
        "enabled = true\n"
        "logfile = log/cowrie.json\n\n"
        "[output_textlog]\n"
        "enabled = true\n"
        "logfile = log/cowrie.log\n\n"
        "[ssh]\n"
        "# SSH banner that will be shown to connecting clients\n"
        "version = %s\n"
        "banner = %s\n\n"
        "[telnet]\n"
        "# Telnet banner\n"
        "banner = %s\n\n"
        "[honeypot]\n"
        "# Honeypot hostname (appears in logs)\n"
        "hostname = %s\n",
        profile->name,
        profile->ssh_banner,
        profile->ssh_banner,
        profile->telnet_banner,
        profile->name);
    
    // Write to both files (Cowrie checks both)
    if (write_file(cowrie_cfg_local, config_content) != 0) {
        log_event_level(LOG_ERROR, "Failed to write Cowrie local config");
        return -1;
    }
    
    // Also create a main config if it doesn't exist
    if (!file_exists(cowrie_cfg_path)) {
        char main_config[1024];
        snprintf(main_config, sizeof(main_config),
            "# Cowrie main configuration\n"
            "# Local overrides are in cowrie.cfg.local\n\n"
            "[output_jsonlog]\n"
            "enabled = true\n"
            "logfile = log/cowrie.json\n\n"
            "[output_textlog]\n"
            "enabled = true\n"
            "logfile = log/cowrie.log\n\n"
            "[ssh]\n"
            "listen_endpoints = tcp:2222:interface=0.0.0.0\n\n"
            "[telnet]\n"
            "listen_endpoints = tcp:2323:interface=0.0.0.0\n");
        write_file(cowrie_cfg_path, main_config);
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), "Cowrie banners updated for profile: %s", profile->name);
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
    
    // Apply morphing
    int result = 0;
    result += morph_cowrie_banners(new_profile);
    result += morph_router_html(new_profile);
    result += morph_camera_html(new_profile);
    
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

int main(int argc, char* argv[]) {
    printf("Bio-Adaptive IoT Honeynet Morphing Engine\n");
    
    // Initialize with config file if provided
    const char* config_file = (argc > 1) ? argv[1] : NULL;
    const char* state_file = (argc > 2) ? argv[2] : NULL;
    
    init_morph_engine(config_file, state_file);
    
    time_t now = time(NULL);
    printf("Morph event: Rotating device profile at %s", ctime(&now));
    
    int result = morph_device();
    
    return (result == 0) ? 0 : 1;
}