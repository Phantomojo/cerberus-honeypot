/**
 * temporal.c - Temporal evolution module
 * 
 * Simulates system aging, log accumulation, and configuration changes
 * to make the honeypot appear to have a realistic history.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "temporal.h"
#include "utils.h"

// Common kernel messages
static const char* kernel_messages[] = {
    "kernel: [0.000000] Linux version 3.10.49",
    "kernel: [0.000000] Memory: 262144K/262144K available",
    "kernel: [0.028476] pid_max: default: 32768 minimum: 301",
    "kernel: [0.032081] Dentry cache hash table entries: 16384 (order: 4, 65536 bytes)",
    "kernel: [0.042015] Inode-cache hash table entries: 8192 (order: 3, 32768 bytes)",
    "kernel: [0.052381] Mount-cache hash table entries: 512",
    "kernel: [0.062081] CPU: Testing write protected pages.",
    "kernel: [0.072387] CPU: write protected page tables enabled",
    "kernel: [0.082081] Intel Pentium Processor stepping 02",
    "kernel: [0.091827] SMP alternatives:  forcing SMP lock prefixes on UP",
    "kernel: [0.101987] SMP alternatives: switching to UP code",
    "kernel: [0.111281] ACPI Exception: AE_NOT_FOUND, Module [DSDT], Line 0",
    "kernel: [0.121381] ACPI: System BIOS age 0 ACPI Subsystem version",
    "systemd[1]: Started Session 1 of user root.",
    "systemd[1]: Startup finished in 2.341s (kernel) + 1.823s (userspace) = 4.164s"
};
static const int kernel_messages_count = sizeof(kernel_messages) / sizeof(kernel_messages[0]);

// Realistic system messages
static const char* system_messages[] = {
    "cron[%d]: (root) CMD (run-parts --report-only /etc/cron.daily)",
    "sshd[%d]: Received disconnect from 192.168.1.100 port 54321 [preauth]",
    "sshd[%d]: Invalid user admin from 192.168.1.105 port 43251",
    "sshd[%d]: Accepted publickey for root from 192.168.1.110 port 45678",
    "ntpd[%d]: Freq 0.000 +0.000 f000",
    "dnsmasq[%d]: listening on 192.168.1.1#53",
    "dnsmasq[%d]: query[A] google.com from 192.168.1.50",
    "dhcpd: Received DHCPDISCOVER from 00:11:22:33:44:55 via eth0",
    "kernel: [12345.678901] Out of memory: Kill process %d (%s) score %u or sacrifice child"
};
static const int system_messages_count = sizeof(system_messages) / sizeof(system_messages[0]);

/**
 * Get realistic uptime in seconds (1 day to 1 year)
 */
int get_realistic_uptime_seconds(void) {
    // Biased toward recent boots, but some old systems
    int days = 1 + (rand() % 365);
    
    // 20% chance of longer uptime (2-5 years)
    if (rand() % 100 < 20) {
        days = 365 + (rand() % (365 * 4));
    }

    return days * 86400;
}

/**
 * Get realistic boot time
 */
time_t get_realistic_boot_time(void) {
    time_t now = time(NULL);
    int uptime_seconds = get_realistic_uptime_seconds();
    return now - uptime_seconds;
}

/**
 * Create initial system state
 */
system_state_t* create_initial_system_state(time_t boot_time) {
    system_state_t* state = (system_state_t*)malloc(sizeof(system_state_t));
    if (!state) return NULL;

    memset(state, 0, sizeof(system_state_t));
    state->timestamp = boot_time;
    state->uptime_seconds = time(NULL) - boot_time;
    state->total_boots = 1 + (rand() % 20);
    state->patch_level = rand() % 50;
    state->log_entries_count = 0;

    // Random kernel versions
    static const char* kernels[] = {
        "3.10.49", "3.0.8", "2.6.36.4", "4.4.0", "2.6.30"
    };
    strncpy(state->kernel_version, kernels[rand() % 5], 127);

    // Random update timestamps
    time_t last_update = boot_time + (rand() % state->uptime_seconds);
    struct tm* tm = localtime(&last_update);
    strftime(state->last_update, 127, "%Y-%m-%d %H:%M:%S", tm);

    return state;
}

/**
 * Advance system time
 */
void advance_system_time(system_state_t* state, uint32_t seconds) {
    if (!state) return;
    state->uptime_seconds += seconds;
    state->timestamp += seconds;
}

/**
 * Simulate system aging
 */
void simulate_system_aging(system_state_t* state) {
    if (!state) return;

    // Update uptime gradually
    advance_system_time(state, 3600 + (rand() % 86400));

    // Occasional patch updates
    if (rand() % 100 < 5) {
        state->patch_level++;
    }

    // Occasional reboots (less common as uptime increases)
    if (state->uptime_seconds > 86400 && rand() % 1000 < 3) {
        state->total_boots++;
        state->uptime_seconds = rand() % 3600;  // Reset to short uptime
    }
}

/**
 * Accumulate log files
 */
void accumulate_log_files(system_state_t* state) {
    if (!state) return;

    // Add kernel messages
    if (state->log_entries_count < MAX_LOG_ENTRIES && rand() % 100 < 10) {
        const char* msg = kernel_messages[rand() % kernel_messages_count];
        add_log_entry(state, "INFO", "kernel", (char*)msg);
    }

    // Add system messages
    if (state->log_entries_count < MAX_LOG_ENTRIES && rand() % 100 < 15) {
        const char* msg_template = system_messages[rand() % system_messages_count];
        char msg[256];
        snprintf(msg, sizeof(msg), msg_template, rand() % 10000);
        add_log_entry(state, "INFO", "system", msg);
    }
}

/**
 * Simulate configuration changes
 */
void simulate_configuration_changes(system_state_t* state) {
    if (!state) return;

    // Occasional configuration updates
    if (rand() % 100 < 3) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Configuration file updated: %s",
                 "/etc/config");
        add_log_entry(state, "INFO", "config", msg);
    }
}

/**
 * Simulate service restarts
 */
void simulate_service_restarts(system_state_t* state) {
    if (!state) return;

    static const char* services[] = {
        "sshd", "dnsmasq", "httpd", "ntpd", "syslog", "cron"
    };

    if (rand() % 100 < 2) {
        const char* service = services[rand() % 6];
        char msg[256];
        snprintf(msg, sizeof(msg), "Service restarted: %s", service);
        add_log_entry(state, "WARN", "init", msg);
    }
}

/**
 * Generate uptime output
 */
int generate_system_uptime(system_state_t* state, char* output, size_t output_size) {
    if (!state || !output || output_size < 256) return -1;

    uint32_t uptime = state->uptime_seconds;
    uint32_t days = uptime / 86400;
    uint32_t hours = (uptime % 86400) / 3600;
    uint32_t mins = (uptime % 3600) / 60;

    float load1 = (float)(rand() % 20) / 10.0f;
    float load5 = (float)(rand() % 30) / 10.0f;
    float load15 = (float)(rand() % 40) / 10.0f;

    snprintf(output, output_size,
             " %02u:%02u:%02u up %u day%s, %2u:%02u, %d user%s, load average: %.2f, %.2f, %.2f\n",
             0, 0, 0,  // Current time (would be filled in real use)
             days, days > 1 ? "s" : "",
             hours, mins,
             1,
             "s",
             load1, load5, load15);

    return strlen(output);
}

/**
 * Generate kernel messages output
 */
int generate_kernel_messages(system_state_t* state, char* output, size_t output_size) {
    if (!state || !output || output_size < 512) return -1;

    output[0] = '\0';
    
    // Add various kernel messages
    for (int i = 0; i < 5 && strlen(output) < output_size - 256; i++) {
        const char* msg = kernel_messages[rand() % kernel_messages_count];
        strcat(output, msg);
        strcat(output, "\n");
    }

    return strlen(output);
}

/**
 * Generate syslog output
 */
int generate_syslog(system_state_t* state, char* output, size_t output_size) {
    if (!state || !output || output_size < 1024) return -1;

    output[0] = '\0';
    char buffer[512];

    // Syslog header
    time_t now = time(NULL);
    struct tm* tm = localtime(&now);
    char date_str[32];
    strftime(date_str, sizeof(date_str), "%b %d %H:%M:%S", tm);

    snprintf(buffer, sizeof(buffer), "%s device-hostname kernel: Linux version\n", date_str);
    strcat(output, buffer);

    // Add some log entries
    for (int i = 0; i < 10 && strlen(output) < output_size - 256; i++) {
        const char* msg = system_messages[rand() % system_messages_count];
        
        time_t entry_time = now - (rand() % 3600);
        tm = localtime(&entry_time);
        strftime(date_str, sizeof(date_str), "%b %d %H:%M:%S", tm);

        snprintf(buffer, sizeof(buffer), "%s device-hostname %s: %s\n",
                 date_str, 
                 (rand() % 2 == 0) ? "kernel" : "sshd",
                 msg);

        if (strlen(output) + strlen(buffer) < output_size) {
            strcat(output, buffer);
        }
    }

    return strlen(output);
}

/**
 * Add log entry
 */
void add_log_entry(system_state_t* state, const char* level, const char* source, const char* message) {
    if (!state || !level || !source || !message) return;
    if (state->log_entries_count >= MAX_LOG_ENTRIES) return;

    // In a full implementation, would store this
    // For now, just count it
    state->log_entries_count++;
}

/**
 * Free system state
 */
void free_system_state(system_state_t* state) {
    if (state) {
        free(state);
    }
}
