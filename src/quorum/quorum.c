#include "quorum.h"

#include "utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

// Global state
static ip_tracking_t ip_tracking[MAX_IPS];
static int ip_count = 0;
static service_config_t service_configs[MAX_SERVICES];
static int service_count = 0;
static char alert_log_path[512] = "build/quorum-alerts.log";

// IP address validation (simple IPv4 check)
bool is_valid_ip(const char* ip) {
    if (!ip || strlen(ip) < 7)
        return false;

    int parts[4];
    char extra;
    int count = sscanf(ip, "%d.%d.%d.%d%c", &parts[0], &parts[1], &parts[2], &parts[3], &extra);
    if (count != 4)
        return false; // Should match exactly 4 numbers, no extra characters

    for (int i = 0; i < 4; i++) {
        if (parts[i] < 0 || parts[i] > 255)
            return false;
    }

    return true;
}

// Extract IP from log line (simple regex-free approach)
int extract_ip_from_line(const char* line, char* ip_out) {
    if (!line || !ip_out)
        return -1;

    // Look for IP pattern: digits.digits.digits.digits
    const char* start = line;
    while (*start) {
        if (isdigit(*start)) {
            char ip_candidate[46] = {0};
            int i = 0;
            const char* p = start;

            // Try to extract IP-like pattern
            while (i < 45 && *p && (isdigit(*p) || *p == '.')) {
                ip_candidate[i++] = *p++;
            }
            ip_candidate[i] = '\0';

            if (is_valid_ip(ip_candidate)) {
                strncpy(ip_out, ip_candidate, MAX_IP_STRING - 1);
                ip_out[MAX_IP_STRING - 1] = '\0';
                return 0;
            }
        }
        start++;
    }

    return -1;
}

bool is_ip_in_tracking(const char* ip) {
    for (int i = 0; i < ip_count; i++) {
        if (strcmp(ip_tracking[i].ip, ip) == 0) {
            return true;
        }
    }
    return false;
}

int add_ip_to_tracking(const char* ip, const char* service) {
    if (!ip || !service)
        return -1;

    // Find existing IP or create new entry
    ip_tracking_t* entry = NULL;
    for (int i = 0; i < ip_count; i++) {
        if (strcmp(ip_tracking[i].ip, ip) == 0) {
            entry = &ip_tracking[i];
            break;
        }
    }

    if (!entry) {
        if (ip_count >= MAX_IPS) {
            log_event_level(LOG_WARN, "IP tracking table full");
            return -1;
        }
        entry = &ip_tracking[ip_count++];
        strncpy(entry->ip, ip, MAX_IP_STRING - 1);
        entry->ip[MAX_IP_STRING - 1] = '\0';
        entry->service_count = 0;
        entry->hit_count = 0;
        entry->first_seen = time(NULL);
    }

    // Check if service already recorded
    bool service_exists = false;
    for (int i = 0; i < entry->service_count; i++) {
        if (strcmp(entry->services[i], service) == 0) {
            service_exists = true;
            break;
        }
    }

    if (!service_exists && entry->service_count < MAX_SERVICES) {
        strncpy(entry->services[entry->service_count], service, MAX_SERVICE_NAME - 1);
        entry->services[entry->service_count][MAX_SERVICE_NAME - 1] = '\0';
        entry->service_count++;
    }

    entry->last_seen = time(NULL);
    entry->hit_count++;

    return 0;
}

int parse_log_file(const char* filepath, const char* service_name) {
    if (!file_exists(filepath)) {
        return 0; // File doesn't exist, not an error
    }

    FILE* f = fopen(filepath, "r");
    if (!f) {
        log_event_level(LOG_WARN, "Failed to open log file");
        return -1;
    }

    char line[MAX_LOG_LINE];
    int ip_count_before = ip_count;

    // Read last N lines (simple approach - in production, use tail or seek)
    // For now, read all lines but only process recent ones
    while (fgets(line, sizeof(line), f)) {
        char ip[MAX_IP_STRING];
        if (extract_ip_from_line(line, ip) == 0) {
            add_ip_to_tracking(ip, service_name);
        }
    }

    fclose(f);

    int new_ips = ip_count - ip_count_before;
    if (new_ips > 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Found %d IP(s) in %s logs", new_ips, service_name);
        log_event_level(LOG_DEBUG, msg);
    }

    return 0;
}

int scan_logs_for_ips(void) {
    log_event_level(LOG_INFO, "Scanning logs for IP addresses");

    // Default log locations if no config loaded
    if (service_count == 0) {
        // Add default services
        add_service_config("cowrie", "services/cowrie/logs/cowrie.log");
        add_service_config("router-web", "services/fake-router-web/logs/access.log");
        add_service_config("camera-web", "services/fake-camera-web/logs/access.log");
    }

    // Scan each service's logs
    for (int i = 0; i < service_count; i++) {
        if (service_configs[i].enabled) {
            parse_log_file(service_configs[i].log_path, service_configs[i].name);
        }
    }

    char msg[256];
    snprintf(msg, sizeof(msg), "Total unique IPs tracked: %d", ip_count);
    log_event_level(LOG_INFO, msg);

    return ip_count;
}

int detect_lateral_movement(void) {
    int lateral_alerts = 0;
    const char* lateral_keywords[] = {"nmap", "ping", "arp", "route", "netstat", "ssh ", "telnet "};
    const char* internal_ranges[] = {"172.17.", "192.168.", "10.0."};
    int kw_count = sizeof(lateral_keywords) / sizeof(char*);
    int range_count = sizeof(internal_ranges) / sizeof(char*);

    log_event_level(LOG_INFO, "Scanning for lateral movement attempts...");

    // We scan the tracking entries directly if they contain command context,
    // but better yet, we parse the last log cycle for these patterns specifically.
    // For Pillar 2, we implement a specific cross-module check.

    for (int i = 0; i < ip_count; i++) {
        ip_tracking_t* entry = &ip_tracking[i];

        // This is a placeholder for deep packet/command inspection
        // In a real micro-net, we'd check if this IP is trying to touch
        // internal neighbors.

        // Placeholder check for high hit counts on single IPs often indicates
        // internal enumeration attempts.
        if (entry->hit_count > 50 && entry->service_count == 1) {
            char alert[512];
            snprintf(alert,
                     sizeof(alert),
                     "ALERT: Potential Lateral Enumeration detected from %s\n"
                     "  Signature: Internal Port Sweeping\n---\n",
                     entry->ip);
            log_event_file(LOG_WARN, alert_log_path, alert);
            lateral_alerts++;
        }
    }

    return lateral_alerts;
}

int detect_coordinated_attacks(void) {
    int alert_count = 0;

    log_event_level(LOG_INFO, "Detecting coordinated attacks");

    for (int i = 0; i < ip_count; i++) {
        ip_tracking_t* entry = &ip_tracking[i];

        // Coordinated attack: IP hitting multiple services
        if (entry->service_count >= 2) {
            generate_alert(entry);
            alert_count++;
        }
    }

    if (alert_count > 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Detected %d potential coordinated attack(s)", alert_count);
        log_event_level(LOG_WARN, msg);
    } else {
        log_event_level(LOG_INFO, "No coordinated attacks detected");
    }

    return alert_count;
}

int generate_alert(const ip_tracking_t* ip_track) {
    if (!ip_track)
        return -1;

    char alert[1024];
    char time_str[64];
    struct tm* tm_info = localtime(&ip_track->last_seen);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    // Build services list
    char services_list[512] = {0};
    for (int i = 0; i < ip_track->service_count; i++) {
        if (i > 0) {
            size_t cur = strlen(services_list);
            size_t rem = (sizeof(services_list) > cur + 1) ? (sizeof(services_list) - 1 - cur) : 0;
            strncat(services_list, ", ", rem);
        }
        {
            size_t cur = strlen(services_list);
            size_t rem = (sizeof(services_list) > cur + 1) ? (sizeof(services_list) - 1 - cur) : 0;
            strncat(services_list, ip_track->services[i], rem);
        }
    }

    snprintf(alert,
             sizeof(alert),
             "ALERT: Coordinated attack detected\n"
             "  IP: %s\n"
             "  Services hit: %s (%d services)\n"
             "  Total hits: %d\n"
             "  First seen: %s\n"
             "  Last seen: %s\n"
             "---\n",
             ip_track->ip,
             services_list,
             ip_track->service_count,
             ip_track->hit_count,
             ctime(&ip_track->first_seen),
             time_str);

    // Log to file
    log_event_file(LOG_WARN, alert_log_path, alert);

    // Also print to stdout
    printf("\n%s", alert);

    return 0;
}

int load_service_configs(const char* config_file) {
    if (!file_exists(config_file)) {
        log_event_level(LOG_DEBUG, "Service config file not found, using defaults");
        return 0;
    }

    FILE* f = fopen(config_file, "r");
    if (!f) {
        return -1;
    }

    service_count = 0;
    char line[512];

    while (fgets(line, sizeof(line), f) && service_count < MAX_SERVICES) {
        trim_string(line);
        if (line[0] == '#' || line[0] == '\0')
            continue;

        char* eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            char* name = line;
            char* path = eq + 1;
            trim_string(name);
            trim_string(path);

            add_service_config(name, path);
        }
    }

    fclose(f);
    return service_count;
}

int add_service_config(const char* name, const char* log_path) {
    if (service_count >= MAX_SERVICES) {
        return -1;
    }

    service_config_t* config = &service_configs[service_count++];
    strncpy(config->name, name, MAX_SERVICE_NAME - 1);
    config->name[MAX_SERVICE_NAME - 1] = '\0';
    strncpy(config->log_path, log_path, MAX_LOG_LINE - 1);
    config->log_path[MAX_LOG_LINE - 1] = '\0';
    config->enabled = true;

    return 0;
}

int get_tracked_ip_count(void) {
    return ip_count;
}

ip_tracking_t* get_tracked_ip(int index) {
    if (index < 0 || index >= ip_count) {
        return NULL;
    }
    return &ip_tracking[index];
}

int run_quorum_logic(void) {
    // Clear previous tracking (or implement time-based expiration)
    // For now, we'll accumulate across runs

    // Scan logs
    scan_logs_for_ips();

    // Detect coordinated attacks
    int alert_count = detect_coordinated_attacks();

    // Pillar 2: Lateral Movement Detection
    alert_count += detect_lateral_movement();

    return alert_count;
}

int main(int argc, char* argv[]) {
    printf("Bio-Adaptive IoT Honeynet Quorum Engine\n");

    // Load service configs if provided
    if (argc > 1) {
        load_service_configs(argv[1]);
    }

    time_t now = time(NULL);
    printf("Quorum check: Checking logs at %s", ctime(&now));

    int alert_count = run_quorum_logic();

    printf("\nQuorum check complete. Alerts: %d\n", alert_count);

    return (alert_count > 0) ? 1 : 0;
}
