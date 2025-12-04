#ifndef MONITORING_H
#define MONITORING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Alert severity levels
typedef enum {
    ALERT_LEVEL_INFO = 0,
    ALERT_LEVEL_WARN = 1,
    ALERT_LEVEL_ERROR = 2,
    ALERT_LEVEL_CRITICAL = 3,
    ALERT_LEVEL_EMERGENCY = 4
} alert_level_t;

// Alert types
typedef enum {
    ALERT_TYPE_SECURITY = 0,
    ALERT_TYPE_PERFORMANCE = 1,
    ALERT_TYPE_SYSTEM = 2,
    ALERT_TYPE_NETWORK = 3,
    ALERT_TYPE_SANDBOX = 4,
    ALERT_TYPE_ENCRYPTION = 5,
    ALERT_TYPE_MORPHING = 6
} alert_type_t;

// Alert status
typedef enum {
    ALERT_STATUS_NEW = 0,
    ALERT_STATUS_ACKNOWLEDGED = 1,
    ALERT_STATUS_RESOLVED = 2,
    ALERT_STATUS_SUPPRESSED = 3
} alert_status_t;

// Alert structure
typedef struct {
    uint64_t id;                           // Unique alert ID
    alert_type_t type;                     // Type of alert
    alert_level_t level;                   // Severity level
    alert_status_t status;                  // Current status
    char timestamp[32];                    // ISO 8601 timestamp
    char source[64];                       // Source component/service
    char message[256];                     // Alert message
    char details[512];                    // Additional details
    char service_name[64];                 // Related service
    uint32_t occurrence_count;             // Number of occurrences
    char first_seen[32];                  // First occurrence time
    char last_seen[32];                   // Last occurrence time
    bool is_active;                        // Whether alert is currently active
} alert_t;

// Monitoring metrics
typedef struct {
    char service_name[64];                 // Service name
    uint32_t cpu_usage;                    // CPU usage percentage
    uint32_t memory_usage;                 // Memory usage in MB
    uint32_t disk_usage;                    // Disk usage percentage
    uint32_t network_connections;            // Active network connections
    uint32_t requests_per_second;           // Requests per second
    uint32_t error_rate;                    // Error rate percentage
    uint64_t uptime_seconds;                // Service uptime
    char status[32];                       // Service status
    char last_check[32];                   // Last check timestamp
} monitoring_metrics_t;

// Monitoring configuration
typedef struct {
    bool enable_real_time_monitoring;        // Enable real-time monitoring
    bool enable_alerting;                   // Enable alerting system
    bool enable_performance_monitoring;       // Enable performance monitoring
    bool enable_security_monitoring;         // Enable security monitoring
    uint32_t alert_threshold_cpu;            // CPU usage alert threshold
    uint32_t alert_threshold_memory;          // Memory usage alert threshold
    uint32_t alert_threshold_disk;           // Disk usage alert threshold
    uint32_t alert_threshold_connections;     // Network connections alert threshold
    uint32_t alert_threshold_error_rate;     // Error rate alert threshold
    uint32_t monitoring_interval_seconds;      // Monitoring interval
    char alert_webhook_url[256];            // Webhook URL for alerts
    char alert_email_recipients[512];        // Email recipients for alerts
    char log_file_path[256];                // Log file path
} monitoring_config_t;

// Monitoring context
typedef struct {
    monitoring_config_t config;                // Monitoring configuration
    alert_t alerts[1000];                   // Alert storage
    size_t alert_count;                      // Number of active alerts
    monitoring_metrics_t metrics[50];          // Service metrics
    size_t metrics_count;                    // Number of monitored services
    bool is_running;                          // Whether monitoring is active
    uint64_t last_alert_id;                   // Last generated alert ID
} monitoring_context_t;

// Function declarations
monitoring_result_t monitoring_init(monitoring_context_t* ctx, const monitoring_config_t* config);
monitoring_result_t monitoring_start(monitoring_context_t* ctx);
monitoring_result_t monitoring_stop(monitoring_context_t* ctx);
monitoring_result_t monitoring_add_service(monitoring_context_t* ctx, const char* service_name);
monitoring_result_t monitoring_remove_service(monitoring_context_t* ctx, const char* service_name);
monitoring_result_t monitoring_update_metrics(monitoring_context_t* ctx, const char* service_name, const monitoring_metrics_t* metrics);
monitoring_result_t monitoring_create_alert(monitoring_context_t* ctx, alert_type_t type, alert_level_t level, const char* source, const char* message, const char* details);
monitoring_result_t monitoring_acknowledge_alert(monitoring_context_t* ctx, uint64_t alert_id);
monitoring_result_t monitoring_resolve_alert(monitoring_context_t* ctx, uint64_t alert_id);
monitoring_result_t monitoring_get_alerts(monitoring_context_t* ctx, alert_t* alerts, size_t* count);
monitoring_result_t monitoring_get_metrics(monitoring_context_t* ctx, monitoring_metrics_t* metrics, size_t* count);
monitoring_result_t monitoring_export_alerts(monitoring_context_t* ctx, const char* filename);
monitoring_result_t monitoring_export_metrics(monitoring_context_t* ctx, const char* filename);

// Alert notification functions
monitoring_result_t monitoring_send_email_alert(monitoring_context_t* ctx, const alert_t* alert);
monitoring_result_t monitoring_send_webhook_alert(monitoring_context_t* ctx, const alert_t* alert);
monitoring_result_t monitoring_send_syslog_alert(monitoring_context_t* ctx, const alert_t* alert);

// Utility functions
const char* monitoring_get_alert_level_string(alert_level_t level);
const char* monitoring_get_alert_type_string(alert_type_t type);
const char* monitoring_get_alert_status_string(alert_status_t status);
bool monitoring_is_alert_critical(alert_level_t level);
bool monitoring_should_send_alert(const monitoring_config_t* config, const alert_t* alert);
void monitoring_format_timestamp(char* buffer, size_t size);
void monitoring_log_event(const char* event, const char* details);

// Constants
#define MONITORING_MAX_ALERTS 1000
#define MONITORING_MAX_SERVICES 50
#define MONITORING_DEFAULT_CPU_THRESHOLD 80
#define MONITORING_DEFAULT_MEMORY_THRESHOLD 512
#define MONITORING_DEFAULT_DISK_THRESHOLD 85
#define MONITORING_DEFAULT_CONNECTIONS_THRESHOLD 100
#define MONITORING_DEFAULT_ERROR_RATE_THRESHOLD 10
#define MONITORING_DEFAULT_INTERVAL 30
#define MONITORING_LOG_FILE "/var/log/cerberus-monitoring.log"

#endif // MONITORING_H