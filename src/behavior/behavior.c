/**
 * behavior.c - Behavioral adaptation module
 * 
 * Adds realistic behavioral variations to honeypot responses:
 * - Command execution delays
 * - Varying error messages
 * - Different response formats
 * - Session timeouts
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "behavior.h"
#include "utils.h"

// Realistic error messages by command
static error_template_t error_templates[] = {
    {
        "ls",
        {
            "ls: cannot access '/nonexistent': No such file or directory",
            "ls: cannot open directory '.': Permission denied",
            "ls: command not found",
            NULL, NULL
        },
        3
    },
    {
        "cat",
        {
            "cat: /etc/shadow: Permission denied",
            "cat: /nonexistent/file: No such file or directory",
            "-bash: cat: command not found",
            NULL, NULL
        },
        3
    },
    {
        "ssh",
        {
            "ssh: connect to host 192.168.1.1 port 22: Connection refused",
            "ssh: connect to host 192.168.1.1 port 22: Connection timed out",
            "Permission denied (publickey,password)",
            "ssh: no identities available",
            NULL
        },
        4
    },
    {
        "ping",
        {
            "ping: unknown host example.com",
            "ping: destination host unreachable",
            "PING example.com (93.184.216.34) 56(84) bytes of data.",  // Success
            NULL, NULL
        },
        3
    },
    {
        "ifconfig",
        {
            "ifconfig: command not found",
            "No such device",
            NULL, NULL, NULL
        },
        2
    },
    {
        "su",
        {
            "su: Authentication failure",
            "su: incorrect password",
            "su: Permission denied",
            NULL, NULL
        },
        3
    },
    {
        "sudo",
        {
            "sudo: command not found",
            "user is not in the sudoers file",
            "sudo: sorry, you must have a tty to run sudo",
            "sudo: unable to read /etc/sudoers",
            NULL
        },
        4
    },
    {
        "cd",
        {
            "-bash: cd: /root: Permission denied",
            "-bash: cd: /nonexistent: No such file or directory",
            NULL, NULL, NULL
        },
        2
    }
};
static const int error_templates_count = sizeof(error_templates) / sizeof(error_templates[0]);

// Device-specific behavioral profiles
static const session_behavior_t router_behavior = {
    .response_variance = 0.3f,
    .min_delay_ms = 50,
    .max_delay_ms = 500,
    .timeout_seconds = 300,
    .failed_auth_attempts = 3,
    .has_delays = true,
    .has_jitter = true
};

static const session_behavior_t camera_behavior = {
    .response_variance = 0.4f,
    .min_delay_ms = 100,
    .max_delay_ms = 800,
    .timeout_seconds = 180,
    .failed_auth_attempts = 2,
    .has_delays = true,
    .has_jitter = true
};

/**
 * Generate behavioral characteristics for a command
 */
command_behavior_t generate_command_behavior(const char* command) {
    command_behavior_t behavior = {
        .execution_delay_ms = 50 + (rand() % 450),
        .response_time_ms = 10 + (rand() % 200),
        .returns_error = (rand() % 100) < 15,  // 15% error rate
        .error_code = 127 + (rand() % 3),
        .error_message = NULL
    };

    // Some commands are slower
    if (strstr(command, "ping") || strstr(command, "ssh") || 
        strstr(command, "scp") || strstr(command, "wget")) {
        behavior.execution_delay_ms = 500 + (rand() % 2000);
    }

    // Some commands rarely fail
    if (strstr(command, "echo") || strstr(command, "cat")) {
        behavior.returns_error = (rand() % 100) < 30;
    }

    return behavior;
}

/**
 * Generate session behavior based on device type
 */
session_behavior_t generate_session_behavior(const char* device_type) {
    if (!device_type) {
        return router_behavior;
    }

    if (strstr(device_type, "camera") || strstr(device_type, "Hikvision") ||
        strstr(device_type, "Dahua")) {
        return camera_behavior;
    }

    return router_behavior;
}

/**
 * Calculate realistic command delay
 */
uint32_t calculate_command_delay(command_behavior_t* behavior, const char* command) {
    if (!behavior || !command) return 100;

    uint32_t base_delay = behavior->execution_delay_ms;

    // Add jitter (±20%)
    int variance = (base_delay * 20) / 100;
    int jitter = (rand() % (variance * 2)) - variance;

    return base_delay + jitter;
}

/**
 * Apply behavioral variations to output
 */
int apply_behavioral_variations(char* output, size_t output_size, 
                                command_behavior_t* behavior) {
    if (!output || output_size < 64) return -1;
    if (!behavior || !behavior->returns_error) return 0;

    // Replace output with error message
    const char* error = "Command execution failed";
    snprintf(output, output_size, "%s: %s (exit code: %u)",
             error, behavior->error_message ? behavior->error_message : "Unknown error",
             behavior->error_code);

    return strlen(output);
}

/**
 * Get realistic error for command
 */
const char* get_realistic_error(const char* command) {
    if (!command) return "Unknown error";

    for (int i = 0; i < error_templates_count; i++) {
        if (strstr(command, error_templates[i].command)) {
            int idx = rand() % error_templates[i].message_count;
            return error_templates[i].error_messages[idx];
        }
    }

    // Generic errors
    static const char* generic_errors[] = {
        "command not found",
        "Permission denied",
        "No such file or directory",
        "Invalid argument",
        "Operation timed out"
    };
    return generic_errors[rand() % 5];
}

/**
 * Get timeout error
 */
const char* get_timeout_error(const char* command) {
    static const char* timeouts[] = {
        "Connection timed out",
        "Operation timed out",
        "Read timed out",
        "Timeout waiting for response",
        "Request timed out"
    };
    return timeouts[rand() % 5];
}

/**
 * Get permission error
 */
const char* get_permission_error(const char* command, const char* path) {
    static const char* perm_errors[] = {
        "Permission denied",
        "Operation not permitted",
        "Access denied",
        "User not authorized",
        "Insufficient privileges"
    };

    static char full_error[256];
    const char* base = perm_errors[rand() % 5];
    
    if (path) {
        snprintf(full_error, sizeof(full_error), "%s: %s", base, path);
    } else {
        strncpy(full_error, base, sizeof(full_error) - 1);
    }

    return full_error;
}

/**
 * Generate error response
 */
int generate_error_response(const char* command, char* output, size_t output_size) {
    if (!command || !output || output_size < 128) return -1;

    const char* error_msg = NULL;
    int error_type = rand() % 4;

    switch (error_type) {
        case 0:  // Command not found
            error_msg = "command not found";
            break;
        case 1:  // Permission denied
            error_msg = "Permission denied";
            break;
        case 2:  // Timeout
            error_msg = get_timeout_error(command);
            break;
        case 3:  // Realistic command-specific
            error_msg = get_realistic_error(command);
            break;
    }

    snprintf(output, output_size, "-bash: %s: %s", command, error_msg);
    return strlen(output);
}

/**
 * Get session timeout
 */
uint32_t get_session_timeout(session_behavior_t* behavior) {
    if (!behavior) return 300;

    // Vary timeout by ±20%
    uint32_t variance = (behavior->timeout_seconds * 20) / 100;
    int jitter = (rand() % (variance * 2)) - variance;

    return behavior->timeout_seconds + jitter;
}

/**
 * Get jitter factor for response times
 */
float get_jitter_factor(session_behavior_t* behavior) {
    if (!behavior || !behavior->has_jitter) return 1.0f;

    // Return 0.8 to 1.2 (±20% variance)
    return 0.8f + ((float)rand() / RAND_MAX) * 0.4f;
}

/**
 * Generate realistic command output with delays
 */
int add_behavioral_delays(char* output, size_t output_size, 
                          command_behavior_t* behavior,
                          session_behavior_t* session) {
    if (!behavior || !session) return 0;

    // In a real implementation, this would:
    // 1. Sleep for execution_delay_ms
    // 2. Calculate response_time based on jitter
    // 3. Add delays between output chunks
    // 4. Potentially fail the command

    // For now, just return info for logging
    return 0;
}

/**
 * Vary error message format
 */
int vary_error_format(const char* base_error, char* output, size_t output_size) {
    if (!base_error || !output || output_size < 128) return -1;

    // Different formats for same error
    int format = rand() % 4;
    switch (format) {
        case 0:
            snprintf(output, output_size, "Error: %s", base_error);
            break;
        case 1:
            snprintf(output, output_size, "%s", base_error);
            break;
        case 2:
            snprintf(output, output_size, "ERROR: %s", base_error);
            break;
        case 3:
            snprintf(output, output_size, "[ERROR] %s", base_error);
            break;
    }

    return strlen(output);
}
