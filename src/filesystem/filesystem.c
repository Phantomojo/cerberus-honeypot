/**
 * filesystem.c - Filesystem dynamics module
 * 
 * Generates varying filesystem structures, timestamps, and available commands
 * to make each session unique.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "filesystem.h"
#include "utils.h"

// Common binaries for different device types
static const char* router_binaries[] = {
    "ping", "ifconfig", "route", "arp", "netstat", "ps", "cat", "ls", "cd",
    "echo", "uname", "hostname", "whoami", "pwd", "mkdir", "rm", "cp",
    "mv", "chmod", "chown", "kill", "iptables", "tc", "brctl", "vconfig",
    "dhclient", "udhcpc", "dnsmasq", "hostapd", "wpa_supplicant"
};
static const int router_binaries_count = sizeof(router_binaries) / sizeof(router_binaries[0]);

static const char* camera_binaries[] = {
    "ping", "ifconfig", "route", "arp", "netstat", "ps", "cat", "ls", "cd",
    "echo", "uname", "hostname", "whoami", "pwd", "kill", "ffmpeg", "rtsp",
    "motion", "mjpeg_streamer", "gsoap", "openssl", "wget", "curl"
};
static const int camera_binaries_count = sizeof(camera_binaries) / sizeof(camera_binaries[0]);

// Device-specific commands
typedef struct {
    const char* type;
    const char** commands;
    int count;
} device_commands_t;

static const device_commands_t device_commands_map[] = {
    { "router", router_binaries, router_binaries_count },
    { "camera", camera_binaries, camera_binaries_count }
};

// Common IoT device directories
static const char* common_dirs[] = {
    "/bin", "/sbin", "/usr/bin", "/usr/sbin", "/lib", "/usr/lib",
    "/etc", "/etc/config", "/etc/ssl", "/var", "/var/run", "/var/log",
    "/tmp", "/home", "/root", "/proc", "/sys", "/dev", "/opt"
};
static const int common_dirs_count = sizeof(common_dirs) / sizeof(common_dirs[0]);

/**
 * Create a filesystem snapshot with random structure
 */
filesystem_snapshot_t* create_filesystem_snapshot(const char* base_path) {
    filesystem_snapshot_t* fs = (filesystem_snapshot_t*)malloc(sizeof(filesystem_snapshot_t));
    if (!fs) return NULL;

    memset(fs, 0, sizeof(filesystem_snapshot_t));
    strncpy(fs->root_path, base_path, MAX_PATH_LEN - 1);

    // Add common directories
    for (int i = 0; i < common_dirs_count && fs->file_count < MAX_FILES; i++) {
        file_entry_t* entry = &fs->files[fs->file_count];
        strncpy(entry->path, common_dirs[i], MAX_PATH_LEN - 1);
        strncpy(entry->name, common_dirs[i], MAX_FILENAME - 1);
        entry->size = 4096;  // Directory size
        entry->permissions = 0755;
        entry->is_directory = true;
        entry->is_symlink = false;
        entry->owner = 0;  // root
        entry->group = 0;  // root
        fs->file_count++;
    }

    // Add some random files
    int random_files = 5 + (rand() % 15);
    for (int i = 0; i < random_files && fs->file_count < MAX_FILES; i++) {
        file_entry_t* entry = &fs->files[fs->file_count];
        snprintf(entry->path, MAX_PATH_LEN, "/etc/config-file-%d", rand() % 1000);
        snprintf(entry->name, MAX_FILENAME, "config-file-%d", rand() % 1000);
        entry->size = 1024 + (rand() % 10240);
        entry->permissions = 0644;
        entry->is_directory = false;
        entry->is_symlink = false;
        entry->owner = 0;
        entry->group = 0;
        fs->file_count++;
    }

    return fs;
}

/**
 * Generate random timestamps for files (consistent with uptime)
 */
void generate_random_timestamps(filesystem_snapshot_t* fs, time_t base_time) {
    if (!fs) return;

    time_t now = time(NULL);
    int max_age = (now - base_time);  // Files can't be newer than system startup

    for (int i = 0; i < fs->file_count; i++) {
        int file_age = rand() % (max_age > 0 ? max_age : 86400);  // Max 1 day if no uptime
        time_t file_time = now - file_age;

        fs->files[i].modify_time = file_time;
        fs->files[i].access_time = now - (rand() % 3600);  // Accessed recently
        fs->files[i].change_time = file_time;
    }
}

/**
 * Generate directory variations (some dirs present, others absent)
 */
void generate_directory_variations(filesystem_snapshot_t* fs) {
    if (!fs) return;

    // 30% chance to remove some directories
    if (rand() % 100 < 30) {
        int to_remove = rand() % (fs->file_count / 3);
        for (int i = 0; i < to_remove; i++) {
            int idx = rand() % fs->file_count;
            if (fs->files[idx].is_directory) {
                // Mark as missing by clearing path
                fs->files[idx].path[0] = '\0';
            }
        }
    }
}

/**
 * Generate file size variations
 */
void generate_file_size_variations(filesystem_snapshot_t* fs) {
    if (!fs) return;

    for (int i = 0; i < fs->file_count; i++) {
        if (!fs->files[i].is_directory) {
            // Vary file sizes by ±20%
            int variance = (fs->files[i].size * 20) / 100;
            int new_size = fs->files[i].size + (rand() % (variance * 2)) - variance;
            fs->files[i].size = new_size > 0 ? new_size : 1024;
        }
    }
}

/**
 * Create session-specific log files
 */
void create_session_log_files(filesystem_snapshot_t* fs, const char* session_id) {
    if (!fs || !session_id) return;

    // Add session-specific logs
    if (fs->file_count < MAX_FILES) {
        file_entry_t* entry = &fs->files[fs->file_count];
        snprintf(entry->path, MAX_PATH_LEN, "/var/log/session-%s.log", session_id);
        snprintf(entry->name, MAX_FILENAME, "session-%s.log", session_id);
        entry->size = 1024 + (rand() % 5120);
        entry->permissions = 0644;
        entry->is_directory = false;
        entry->modify_time = time(NULL);
        entry->access_time = time(NULL);
        entry->change_time = time(NULL);
        fs->file_count++;
    }
}

/**
 * Vary file permissions
 */
void vary_permissions(filesystem_snapshot_t* fs) {
    if (!fs) return;

    for (int i = 0; i < fs->file_count; i++) {
        if (fs->files[i].is_directory) {
            // Directories: 755 or 750
            fs->files[i].permissions = (rand() % 100 < 80) ? 0755 : 0750;
        } else {
            // Files: 644, 755, 600, etc.
            static const mode_t perms[] = { 0644, 0755, 0600, 0640, 0750, 0700 };
            fs->files[i].permissions = perms[rand() % 6];
        }
    }
}

/**
 * Get device-specific commands
 */
char** get_device_specific_commands(const char* device_type, int* count) {
    if (!device_type || !count) return NULL;

    for (int i = 0; i < 2; i++) {
        if (strstr(device_type, device_commands_map[i].type)) {
            *count = device_commands_map[i].count;
            return (char**)device_commands_map[i].commands;
        }
    }

    // Default to router commands
    *count = router_binaries_count;
    return (char**)router_binaries;
}

/**
 * Get available binaries for architecture
 */
char** get_available_binaries(const char* architecture, int* count) {
    if (!architecture || !count) return NULL;

    // For now, return all for all architectures
    // In future, filter by ARM vs MIPS vs x86
    *count = router_binaries_count;
    return (char**)router_binaries;
}

/**
 * Generate ls output
 */
int generate_ls_output(filesystem_snapshot_t* fs, const char* path, char* output, size_t output_size) {
    if (!fs || !output || output_size < 256) return -1;

    output[0] = '\0';
    char buffer[512];

    // Find files matching path
    int found = 0;
    for (int i = 0; i < fs->file_count && found < 10; i++) {
        if (strstr(fs->files[i].path, path) && fs->files[i].path[0] != '\0') {
            struct tm* tm_info = localtime(&fs->files[i].modify_time);
            char time_str[32];
            strftime(time_str, sizeof(time_str), "%b %d %H:%M", tm_info);

            snprintf(buffer, sizeof(buffer),
                     "%c%o %-3u %-8u %-8u %10ld %s %s\n",
                     fs->files[i].is_directory ? 'd' : '-',
                     fs->files[i].permissions,
                     1, fs->files[i].owner, fs->files[i].group,
                     fs->files[i].size, time_str, fs->files[i].name);

            if (strlen(output) + strlen(buffer) < output_size) {
                strcat(output, buffer);
                found++;
            }
        }
    }

    return strlen(output);
}

/**
 * Generate find output
 */
int generate_find_output(filesystem_snapshot_t* fs, const char* pattern, char* output, size_t output_size) {
    if (!fs || !output || output_size < 256) return -1;

    output[0] = '\0';

    for (int i = 0; i < fs->file_count; i++) {
        if (fs->files[i].path[0] == '\0') continue;
        
        if (!pattern || strstr(fs->files[i].path, pattern)) {
            if (strlen(output) + strlen(fs->files[i].path) + 2 < output_size) {
                strcat(output, fs->files[i].path);
                strcat(output, "\n");
            }
        }
    }

    return strlen(output);
}

/**
 * Generate du (disk usage) output
 */
int generate_du_output(filesystem_snapshot_t* fs, char* output, size_t output_size) {
    if (!fs || !output || output_size < 512) return -1;

    output[0] = '\0';
    char buffer[256];

    long total_size = 0;
    for (int i = 0; i < fs->file_count; i++) {
        total_size += fs->files[i].size;
        snprintf(buffer, sizeof(buffer),
                 "%ld\t%s\n",
                 (fs->files[i].size + 1023) / 1024,  // Convert to KB blocks
                 fs->files[i].path);

        if (strlen(output) + strlen(buffer) < output_size) {
            strcat(output, buffer);
        }
    }

    // Add total
    snprintf(buffer, sizeof(buffer),
             "%ld\ttotal\n",
             (total_size + 1023) / 1024);
    if (strlen(output) + strlen(buffer) < output_size) {
        strcat(output, buffer);
    }

    return strlen(output);
}

/**
 * Free filesystem snapshot
 */
void free_filesystem_snapshot(filesystem_snapshot_t* fs) {
    if (fs) {
        free(fs);
    }
}
