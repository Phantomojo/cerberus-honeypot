#ifndef SANDBOX_H
#define SANDBOX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Sandbox configuration
typedef struct {
    char service_name[64];         // Name of service (cowrie, rtsp, etc.)
    char chroot_path[256];         // Chroot directory path
    char user[32];                 // Unprivileged user to run as
    char group[32];                // Unprivileged group to run as
    uint32_t max_memory_mb;        // Memory limit in MB
    uint32_t max_cpu_percent;      // CPU usage limit
    uint32_t max_file_descriptors; // File descriptor limit
    bool network_isolated;         // Network namespace isolation
    bool pid_isolated;             // PID namespace isolation
    char allowed_ports[256];       // Comma-separated list of allowed ports
    char readonly_paths[512];      // Read-only mount paths
    char tmpfs_size[64];           // Size of temporary filesystem
} sandbox_config_t;

// Sandbox result codes
typedef enum {
    SANDBOX_SUCCESS = 0,
    SANDBOX_ERROR_PERMISSION = -1,
    SANDBOX_ERROR_CHROOT = -2,
    SANDBOX_ERROR_USER = -3,
    SANDBOX_ERROR_MEMORY = -4,
    SANDBOX_ERROR_NETWORK = -5,
    SANDBOX_ERROR_CONFIG = -6,
    SANDBOX_ERROR_SYSTEM = -7
} sandbox_result_t;

// Main sandbox functions
sandbox_result_t create_sandbox(const sandbox_config_t* config);
sandbox_result_t
run_in_sandbox(const sandbox_config_t* config, const char* command, char* const args[]);
sandbox_result_t cleanup_sandbox(const sandbox_config_t* config);
bool is_sandbox_active(const char* service_name);

// Service-specific sandbox configurations
sandbox_config_t get_cowrie_sandbox_config(void);
sandbox_config_t get_rtsp_sandbox_config(void);
sandbox_config_t get_web_sandbox_config(void);

// Security limits and restrictions
sandbox_result_t apply_resource_limits(const sandbox_config_t* config);
sandbox_result_t setup_network_namespace(const sandbox_config_t* config);
sandbox_result_t setup_filesystem_restrictions(const sandbox_config_t* config);
sandbox_result_t drop_privileges(const sandbox_config_t* config);

// Monitoring and logging
void log_sandbox_event(const char* service_name, const char* event, const char* details);
bool check_sandbox_integrity(const char* service_name);
void kill_sandboxed_service(const char* service_name);

// Utility functions
bool is_valid_sandbox_user(const char* username);
bool is_safe_chroot_path(const char* path);
bool validate_sandbox_config(const sandbox_config_t* config);

// Security constants
#define SANDBOX_MAX_SERVICES 10
#define SANDBOX_DEFAULT_MEMORY_LIMIT 256 // MB
#define SANDBOX_DEFAULT_CPU_LIMIT 50     // percent
#define SANDBOX_DEFAULT_FD_LIMIT 1024
#define SANDBOX_MIN_MEMORY_LIMIT 32 // MB
#define SANDBOX_MIN_CPU_LIMIT 10    // percent

#endif // SANDBOX_H
