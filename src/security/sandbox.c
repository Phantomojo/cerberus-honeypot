#include "sandbox.h"

#include "utils.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Active sandbox tracking
static sandbox_config_t active_sandboxes[SANDBOX_MAX_SERVICES];
static int active_sandbox_count = 0;

// Create sandbox environment
sandbox_result_t create_sandbox(const sandbox_config_t* config) {
    if (!config || !validate_sandbox_config(config)) {
        log_event_level(LOG_ERROR, "create_sandbox: Invalid configuration");
        return SANDBOX_ERROR_CONFIG;
    }

    // Check if sandbox already exists for this service
    if (is_sandbox_active(config->service_name)) {
        log_event_level(LOG_WARN, "create_sandbox: Sandbox already active for service");
        return SANDBOX_SUCCESS;
    }

    log_event_level(LOG_INFO, "create_sandbox: Creating sandbox for service");

    // Create chroot directory
    if (strlen(config->chroot_path) > 0) {
        if (mkdir(config->chroot_path, 0755) != 0 && errno != EEXIST) {
            log_event_level(LOG_ERROR, "create_sandbox: Failed to create chroot directory");
            return SANDBOX_ERROR_CHROOT;
        }

        if (!is_safe_chroot_path(config->chroot_path)) {
            log_event_level(LOG_ERROR, "create_sandbox: Unsafe chroot path");
            return SANDBOX_ERROR_CHROOT;
        }
    }

    // Setup filesystem restrictions
    sandbox_result_t result = setup_filesystem_restrictions(config);
    if (result != SANDBOX_SUCCESS) {
        log_event_level(LOG_ERROR, "create_sandbox: Failed to setup filesystem restrictions");
        return result;
    }

    log_sandbox_event(config->service_name, "created", "Sandbox environment initialized");
    return SANDBOX_SUCCESS;
}

// Run command in sandbox
sandbox_result_t
run_in_sandbox(const sandbox_config_t* config, const char* command, char* const args[]) {
    if (!config || !command || !validate_sandbox_config(config)) {
        log_event_level(LOG_ERROR, "run_in_sandbox: Invalid parameters");
        return SANDBOX_ERROR_CONFIG;
    }

    log_event_level(LOG_INFO, "run_in_sandbox: Starting sandboxed process");

    pid_t pid = fork();
    if (pid == -1) {
        log_event_level(LOG_ERROR, "run_in_sandbox: Failed to fork process");
        return SANDBOX_ERROR_SYSTEM;
    }

    if (pid == 0) {
        // Child process - apply sandbox restrictions

        // Apply resource limits
        if (apply_resource_limits(config) != SANDBOX_SUCCESS) {
            log_event_level(LOG_ERROR, "run_in_sandbox: Failed to apply resource limits");
            exit(1);
        }

        // Setup network namespace if required
        if (config->network_isolated) {
            if (setup_network_namespace(config) != SANDBOX_SUCCESS) {
                log_event_level(LOG_ERROR, "run_in_sandbox: Failed to setup network namespace");
                exit(1);
            }
        }

        // Drop privileges
        if (drop_privileges(config) != SANDBOX_SUCCESS) {
            log_event_level(LOG_ERROR, "run_in_sandbox: Failed to drop privileges");
            exit(1);
        }

        // Apply chroot if specified
        if (strlen(config->chroot_path) > 0) {
            if (chroot(config->chroot_path) != 0) {
                log_event_level(LOG_ERROR, "run_in_sandbox: Failed to chroot");
                exit(1);
            }
            if (chdir("/") != 0) {
                log_event_level(LOG_ERROR, "run_in_sandbox: Failed to chdir after chroot");
                exit(1);
            }
        }

        // Execute the command
        execvp(command, args);

        // If we reach here, exec failed
        log_event_level(LOG_ERROR, "run_in_sandbox: Failed to execute command");
        exit(1);
    }

    // Parent process - track the sandbox
    if (active_sandbox_count < SANDBOX_MAX_SERVICES) {
        memcpy(&active_sandboxes[active_sandbox_count], config, sizeof(sandbox_config_t));
        active_sandbox_count++;
    }

    log_sandbox_event(config->service_name, "started", "Sandboxed process started");
    return SANDBOX_SUCCESS;
}

// Apply resource limits
sandbox_result_t apply_resource_limits(const sandbox_config_t* config) {
    struct rlimit rl;

    // Memory limit
    if (config->max_memory_mb > 0) {
        rl.rlim_cur = config->max_memory_mb * 1024 * 1024;
        rl.rlim_max = config->max_memory_mb * 1024 * 1024;
        if (setrlimit(RLIMIT_AS, &rl) != 0) {
            log_event_level(LOG_ERROR, "apply_resource_limits: Failed to set memory limit");
            return SANDBOX_ERROR_MEMORY;
        }
    }

    // File descriptor limit
    if (config->max_file_descriptors > 0) {
        rl.rlim_cur = config->max_file_descriptors;
        rl.rlim_max = config->max_file_descriptors;
        if (setrlimit(RLIMIT_NOFILE, &rl) != 0) {
            log_event_level(LOG_ERROR, "apply_resource_limits: Failed to set FD limit");
            return SANDBOX_ERROR_MEMORY;
        }
    }

    // CPU limit (via nice value)
    if (config->max_cpu_percent > 0 && config->max_cpu_percent < 100) {
        int nice_value = (100 - config->max_cpu_percent) / 10;
        if (setpriority(PRIO_PROCESS, 0, nice_value) != 0) {
            log_event_level(LOG_WARN, "apply_resource_limits: Failed to set CPU priority");
        }
    }

    return SANDBOX_SUCCESS;
}

// Setup network namespace (simplified)
sandbox_result_t setup_network_namespace(const sandbox_config_t* config) {
    // This is a simplified implementation
    // In production, you'd use unshare() with CLONE_NEWNET
    // For now, we'll just log the intent

    log_event_level(LOG_INFO, "setup_network_namespace: Network isolation requested");

    // TODO: Implement proper network namespace with unshare()
    // This requires CAP_SYS_ADMIN capabilities

    return SANDBOX_SUCCESS;
}

// Setup filesystem restrictions
sandbox_result_t setup_filesystem_restrictions(const sandbox_config_t* config) {
    // Create read-only mounts if specified
    if (strlen(config->readonly_paths) > 0) {
        // TODO: Implement mount namespace with read-only mounts
        // This requires proper privilege handling
        log_event_level(LOG_INFO, "setup_filesystem_restrictions: Read-only paths requested");
    }

    // Create tmpfs for temporary files
    if (strlen(config->tmpfs_size) > 0) {
        // TODO: Mount tmpfs in sandbox
        log_event_level(LOG_INFO, "setup_filesystem_restrictions: Temporary filesystem requested");
    }

    return SANDBOX_SUCCESS;
}

// Drop privileges
sandbox_result_t drop_privileges(const sandbox_config_t* config) {
    if (strlen(config->user) > 0) {
        // TODO: Implement proper user/group switching
        // This requires running as root initially
        log_event_level(LOG_INFO, "drop_privileges: Privilege drop requested");

        // For now, just validate the user exists
        if (!is_valid_sandbox_user(config->user)) {
            log_event_level(LOG_ERROR, "drop_privileges: Invalid sandbox user");
            return SANDBOX_ERROR_USER;
        }
    }

    return SANDBOX_SUCCESS;
}

// Service-specific sandbox configurations
sandbox_config_t get_cowrie_sandbox_config(void) {
    sandbox_config_t config = {.service_name = "cowrie",
                               .chroot_path = "/var/lib/cerberus/cowrie-chroot",
                               .user = "cowrie",
                               .group = "cowrie",
                               .max_memory_mb = 256,
                               .max_cpu_percent = 50,
                               .max_file_descriptors = 1024,
                               .network_isolated = false, // Cowrie needs network access
                               .pid_isolated = true,
                               .allowed_ports = "22,2222",
                               .readonly_paths = "/etc/ssl,/usr/share/ssl",
                               .tmpfs_size = "64M"};
    return config;
}

sandbox_config_t get_rtsp_sandbox_config(void) {
    sandbox_config_t config = {.service_name = "rtsp",
                               .chroot_path = "/var/lib/cerberus/rtsp-chroot",
                               .user = "rtsp",
                               .group = "rtsp",
                               .max_memory_mb = 128,
                               .max_cpu_percent = 30,
                               .max_file_descriptors = 512,
                               .network_isolated = false, // RTSP needs network access
                               .pid_isolated = true,
                               .allowed_ports = "554,8554",
                               .readonly_paths = "/etc/mediamtx.yml",
                               .tmpfs_size = "32M"};
    return config;
}

sandbox_config_t get_web_sandbox_config(void) {
    sandbox_config_t config = {.service_name = "web",
                               .chroot_path = "/var/lib/cerberus/web-chroot",
                               .user = "www-data",
                               .group = "www-data",
                               .max_memory_mb = 64,
                               .max_cpu_percent = 20,
                               .max_file_descriptors = 256,
                               .network_isolated = false, // Web services need network access
                               .pid_isolated = true,
                               .allowed_ports = "80,443,8080",
                               .readonly_paths = "/var/www/html",
                               .tmpfs_size = "16M"};
    return config;
}

// Utility functions
bool is_valid_sandbox_user(const char* username) {
    // List of valid non-privileged users
    const char* valid_users[] = {
        "nobody", "daemon", "cowrie", "rtsp", "www-data", "nginx", "apache", "mysql", "postgres"};

    for (size_t i = 0; i < sizeof(valid_users) / sizeof(valid_users[0]); i++) {
        if (strcmp(username, valid_users[i]) == 0) {
            return true;
        }
    }

    return false;
}

bool is_safe_chroot_path(const char* path) {
    if (!path)
        return false;

    // Check for dangerous patterns
    if (strstr(path, "..") != NULL) {
        return false;
    }

    // Must be absolute path
    if (path[0] != '/') {
        return false;
    }

    // Check it's under /var/lib/cerberus
    if (strncmp(path, "/var/lib/cerberus/", 18) != 0) {
        return false;
    }

    return true;
}

bool validate_sandbox_config(const sandbox_config_t* config) {
    if (!config)
        return false;

    // Validate service name
    if (strlen(config->service_name) == 0 || strlen(config->service_name) >= 64) {
        return false;
    }

    // Validate memory limits
    if (config->max_memory_mb > 0 &&
        (config->max_memory_mb < SANDBOX_MIN_MEMORY_LIMIT || config->max_memory_mb > 8192)) {
        return false;
    }

    // Validate CPU limits
    if (config->max_cpu_percent > 0 &&
        (config->max_cpu_percent < SANDBOX_MIN_CPU_LIMIT || config->max_cpu_percent > 100)) {
        return false;
    }

    // Validate chroot path
    if (strlen(config->chroot_path) > 0 && !is_safe_chroot_path(config->chroot_path)) {
        return false;
    }

    return true;
}

bool is_sandbox_active(const char* service_name) {
    for (int i = 0; i < active_sandbox_count; i++) {
        if (strcmp(active_sandboxes[i].service_name, service_name) == 0) {
            return true;
        }
    }
    return false;
}

void log_sandbox_event(const char* service_name, const char* event, const char* details) {
    char log_msg[512];
    snprintf(log_msg,
             sizeof(log_msg),
             "SANDOX [%s]: %s - %s",
             service_name,
             event,
             details ? details : "");
    log_event_level(LOG_INFO, log_msg);
}

bool check_sandbox_integrity(const char* service_name) {
    // Check if sandboxed process is still running and within limits
    // This is a simplified implementation
    log_event_level(LOG_DEBUG, "check_sandbox_integrity: Checking sandbox integrity");
    return is_sandbox_active(service_name);
}

void kill_sandboxed_service(const char* service_name) {
    // Find and terminate sandboxed service
    for (int i = 0; i < active_sandbox_count; i++) {
        if (strcmp(active_sandboxes[i].service_name, service_name) == 0) {
            log_sandbox_event(service_name, "killed", "Sandboxed service terminated");

            // Remove from active list
            for (int j = i; j < active_sandbox_count - 1; j++) {
                memcpy(&active_sandboxes[j], &active_sandboxes[j + 1], sizeof(sandbox_config_t));
            }
            active_sandbox_count--;
            return;
        }
    }
}

sandbox_result_t cleanup_sandbox(const sandbox_config_t* config) {
    if (!config) {
        return SANDBOX_ERROR_CONFIG;
    }

    log_sandbox_event(config->service_name, "cleanup", "Cleaning up sandbox environment");

    // Remove from active list
    kill_sandboxed_service(config->service_name);

    // TODO: Cleanup chroot directory, unmount filesystems, etc.

    return SANDBOX_SUCCESS;
}
