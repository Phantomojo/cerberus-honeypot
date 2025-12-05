#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// Simple monitoring implementation without complex dependencies

// Alert levels
typedef enum {
    ALERT_LEVEL_INFO = 0,
    ALERT_LEVEL_WARN = 1,
    ALERT_LEVEL_ERROR = 2,
    ALERT_LEVEL_CRITICAL = 3
} alert_level_t;

// Alert types
typedef enum {
    ALERT_TYPE_SECURITY = 0,
    ALERT_TYPE_PERFORMANCE = 1,
    ALERT_TYPE_SYSTEM = 2,
    ALERT_TYPE_NETWORK = 3
} alert_type_t;

// Simple alert structure
typedef struct {
    uint64_t id;
    alert_type_t type;
    alert_level_t level;
    char timestamp[32];
    char source[64];
    char message[256];
    bool is_active;
} simple_alert_t;

// Simple monitoring context
typedef struct {
    bool is_running;
    uint64_t last_alert_id;
    simple_alert_t alerts[100];
    size_t alert_count;
    char log_file[256];
} simple_monitoring_t;

static simple_monitoring_t g_monitoring_ctx;

// Get current timestamp
void get_current_timestamp(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

// Log monitoring event
void log_monitoring_event(const char* event, const char* details) {
    char timestamp[32];
    get_current_timestamp(timestamp, sizeof(timestamp));

    printf("[%s] MONITORING: %s - %s\n", timestamp, event, details ? details : "");

    // Log to file if configured
    if (strlen(g_monitoring_ctx.log_file) > 0) {
        int fd = open(g_monitoring_ctx.log_file, O_WRONLY | O_CREAT | O_APPEND, 0600);
        if (fd >= 0) {
            fchmod(fd, 0600);
            FILE* f = fdopen(fd, "a");
            if (f) {
                fprintf(f, "[%s] %s - %s\n", timestamp, event, details ? details : "");
                fclose(f);
            } else {
                close(fd);
            }
        }
    }
}

// Initialize monitoring
int monitoring_init(const char* log_file) {
    printf("Initializing monitoring system...\n");

    memset(&g_monitoring_ctx, 0, sizeof(simple_monitoring_t));
    g_monitoring_ctx.is_running = false;
    g_monitoring_ctx.last_alert_id = 0;
    g_monitoring_ctx.alert_count = 0;

    if (log_file) {
        strncpy(g_monitoring_ctx.log_file, log_file, sizeof(g_monitoring_ctx.log_file) - 1);
    } else {
        snprintf(g_monitoring_ctx.log_file,
                 sizeof(g_monitoring_ctx.log_file),
                 "/var/log/cerberus-monitoring.log");
    }

    log_monitoring_event("INIT", "Monitoring system initialized");
    return 0;
}

// Start monitoring
int monitoring_start(void) {
    if (g_monitoring_ctx.is_running) {
        log_monitoring_event("START", "Monitoring already running");
        return -1;
    }

    g_monitoring_ctx.is_running = true;
    log_monitoring_event("START", "Monitoring started");
    return 0;
}

// Stop monitoring
int monitoring_stop(void) {
    if (!g_monitoring_ctx.is_running) {
        log_monitoring_event("STOP", "Monitoring not running");
        return -1;
    }

    g_monitoring_ctx.is_running = false;
    log_monitoring_event("STOP", "Monitoring stopped");
    return 0;
}

// Create alert
uint64_t monitoring_create_alert(alert_type_t type,
                                 alert_level_t level,
                                 const char* source,
                                 const char* message,
                                 const char* details) {
    if (g_monitoring_ctx.alert_count >= 100) {
        log_monitoring_event("ALERT", "Maximum alerts reached");
        return 0;
    }

    simple_alert_t* alert = &g_monitoring_ctx.alerts[g_monitoring_ctx.alert_count];

    alert->id = ++g_monitoring_ctx.last_alert_id;
    alert->type = type;
    alert->level = level;
    alert->is_active = true;

    get_current_timestamp(alert->timestamp, sizeof(alert->timestamp));
    strncpy(alert->source, source ? source : "unknown", sizeof(alert->source) - 1);
    strncpy(alert->message, message ? message : "No message", sizeof(alert->message) - 1);

    if (details) {
        strncpy(alert->message + strlen(alert->message),
                " - ",
                sizeof(alert->message) - strlen(alert->message) - 1);
        strncat(alert->message, details, sizeof(alert->message) - strlen(alert->message) - 1);
    }

    g_monitoring_ctx.alert_count++;

    char alert_info[512];
    snprintf(alert_info,
             sizeof(alert_info),
             "ALERT [%lu]: %s - %s",
             alert->id,
             alert->source,
             alert->message);

    log_monitoring_event("ALERT", alert_info);

    return alert->id;
}

// Check system resources
void monitoring_check_system_resources(void) {
    // Simple system resource checks
    FILE* f;
    char buffer[256];

    // Check memory usage
    f = fopen("/proc/meminfo", "r");
    if (f) {
        while (fgets(buffer, sizeof(buffer), f)) {
            if (strstr(buffer, "MemTotal:")) {
                // Simple memory check
                log_monitoring_event("RESOURCE", "Memory information available");
                break;
            }
        }
        fclose(f);
    }

    // Check disk usage
    f = fopen("/proc/diskstats", "r");
    if (f) {
        while (fgets(buffer, sizeof(buffer), f)) {
            if (strstr(buffer, "sda")) {
                log_monitoring_event("RESOURCE", "Disk information available");
                break;
            }
        }
        fclose(f);
    }

    // Check CPU usage
    f = fopen("/proc/loadavg", "r");
    if (f) {
        if (fgets(buffer, sizeof(buffer), f)) {
            log_monitoring_event("RESOURCE", "CPU load information available");
        }
        fclose(f);
    }
}

// Check service status
void monitoring_check_service_status(const char* service_name) {
    char command[256];
    snprintf(command, sizeof(command), "systemctl is-active %s", service_name);

    int result = system(command);

    char status_info[256];
    snprintf(status_info,
             sizeof(status_info),
             "SERVICE [%s]: %s",
             service_name,
             result == 0 ? "ACTIVE" : "INACTIVE");

    log_monitoring_event("SERVICE", status_info);
}

// Main monitoring loop
void monitoring_main_loop(void) {
    log_monitoring_event("LOOP", "Starting monitoring main loop");

    while (g_monitoring_ctx.is_running) {
        // Check system resources
        monitoring_check_system_resources();

        // Check honeypot services
        monitoring_check_service_status("cerberus-cowrie");
        monitoring_check_service_status("cerberus-rtsp");
        monitoring_check_service_status("cerberus-web");

        // Sleep for monitoring interval
        sleep(30); // Check every 30 seconds
    }
}

// Get alert level string
const char* get_alert_level_string(alert_level_t level) {
    switch (level) {
        case ALERT_LEVEL_INFO:
            return "INFO";
        case ALERT_LEVEL_WARN:
            return "WARN";
        case ALERT_LEVEL_ERROR:
            return "ERROR";
        case ALERT_LEVEL_CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

// Get alert type string
const char* get_alert_type_string(alert_type_t type) {
    switch (type) {
        case ALERT_TYPE_SECURITY:
            return "SECURITY";
        case ALERT_TYPE_PERFORMANCE:
            return "PERFORMANCE";
        case ALERT_TYPE_SYSTEM:
            return "SYSTEM";
        case ALERT_TYPE_NETWORK:
            return "NETWORK";
        default:
            return "UNKNOWN";
    }
}

// Print monitoring status
void monitoring_print_status(void) {
    printf("=== Monitoring Status ===\n");
    printf("Running: %s\n", g_monitoring_ctx.is_running ? "Yes" : "No");
    printf("Alerts: %zu\n", g_monitoring_ctx.alert_count);
    printf("Last Alert ID: %lu\n", g_monitoring_ctx.last_alert_id);
    printf("Log File: %s\n", g_monitoring_ctx.log_file);

    if (g_monitoring_ctx.alert_count > 0) {
        printf("\nRecent Alerts:\n");
        for (size_t i = 0; i < g_monitoring_ctx.alert_count && i < 5; i++) {
            simple_alert_t* alert = &g_monitoring_ctx.alerts[i];
            printf("  [%lu] %s %s: %s\n",
                   alert->id,
                   get_alert_type_string(alert->type),
                   get_alert_level_string(alert->level),
                   alert->message);
        }
    }
}

// Export alerts to JSON
void monitoring_export_alerts_json(const char* filename) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        log_monitoring_event("EXPORT", "Failed to open export file");
        return;
    }

    fprintf(f, "{\n");
    fprintf(f, "  \"alerts\": [\n");

    for (size_t i = 0; i < g_monitoring_ctx.alert_count; i++) {
        simple_alert_t* alert = &g_monitoring_ctx.alerts[i];
        fprintf(f, "    {\n");
        fprintf(f, "      \"id\": %lu,\n", alert->id);
        fprintf(f, "      \"type\": \"%s\",\n", get_alert_type_string(alert->type));
        fprintf(f, "      \"level\": \"%s\",\n", get_alert_level_string(alert->level));
        fprintf(f, "      \"timestamp\": \"%s\",\n", alert->timestamp);
        fprintf(f, "      \"source\": \"%s\",\n", alert->source);
        fprintf(f, "      \"message\": \"%s\",\n", alert->message);
        fprintf(f, "      \"active\": %s\n", alert->is_active ? "true" : "false");
        fprintf(f, "    }%s\n", i < g_monitoring_ctx.alert_count - 1 ? "," : "");
    }

    fprintf(f, "  ]\n");
    fprintf(f, "}\n");

    fclose(f);
    log_monitoring_event("EXPORT", "Alerts exported to JSON");
}

// Test monitoring functionality
void monitoring_test_functionality(void) {
    printf("Testing monitoring functionality...\n");

    // Test alert creation
    uint64_t alert_id = monitoring_create_alert(ALERT_TYPE_SECURITY,
                                                ALERT_LEVEL_WARN,
                                                "test",
                                                "Test alert for monitoring system",
                                                "This is a test alert");

    if (alert_id > 0) {
        printf("✓ Alert creation test: PASS (ID: %lu)\n", alert_id);
    } else {
        printf("✗ Alert creation test: FAIL\n");
    }

    // Test status reporting
    monitoring_print_status();

    // Test export
    monitoring_export_alerts_json("/tmp/test_alerts.json");

    printf("✓ Monitoring functionality test: COMPLETED\n");
}
