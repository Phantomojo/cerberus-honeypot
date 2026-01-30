#ifndef BEHAVIOR_H
#define BEHAVIOR_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// Command response behavior
typedef struct {
    uint32_t execution_delay_ms; // Delay before response
    uint32_t response_time_ms;   // Duration of response
    bool returns_error;          // 40% of commands fail
    uint32_t error_code;         // Fake error code
    const char* error_message;   // Custom error
} command_behavior_t;

// Session behavior profile
typedef struct {
    float response_variance;       // 0.0-1.0: how much responses vary
    uint32_t min_delay_ms;         // Minimum command delay
    uint32_t max_delay_ms;         // Maximum command delay
    uint32_t timeout_seconds;      // Session timeout
    uint32_t failed_auth_attempts; // Max before disconnect
    bool has_delays;               // Add simulated network delays
    bool has_jitter;               // Vary response times
} session_behavior_t;

// Error response templates
typedef struct {
    const char* command;
    const char* error_messages[5];
    int message_count;
} error_template_t;

// Functions
command_behavior_t generate_command_behavior(const char* command);
session_behavior_t generate_session_behavior(const char* device_type);
uint32_t calculate_command_delay(command_behavior_t* behavior, const char* command);
int apply_behavioral_variations(char* output, size_t output_size, command_behavior_t* behavior);
const char* get_realistic_error(const char* command);
const char* get_timeout_error(const char* command);
const char* get_permission_error(const char* command, const char* path);
int generate_error_response(const char* command, char* output, size_t output_size);
uint32_t get_session_timeout(session_behavior_t* behavior);
float get_jitter_factor(session_behavior_t* behavior);

#endif // BEHAVIOR_H
