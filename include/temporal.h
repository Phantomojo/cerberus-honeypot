#ifndef TEMPORAL_H
#define TEMPORAL_H

#include <stdint.h>
#include <time.h>

#define MAX_LOG_ENTRIES 1000
#define MAX_LOG_SIZE 4096

// System state at a point in time
typedef struct {
    time_t timestamp;
    uint32_t uptime_seconds;
    uint32_t total_boots;
    char kernel_version[128];
    char last_update[128];
    uint32_t patch_level;
    int log_entries_count;
} system_state_t;

// Log entry
typedef struct {
    time_t timestamp;
    const char* level;  // INFO, WARN, ERROR, DEBUG
    const char* source; // Kernel, service name, etc.
    char message[256];
} log_entry_t;

// Functions
system_state_t* create_initial_system_state(time_t boot_time);
void advance_system_time(system_state_t* state, uint32_t seconds);
void simulate_system_aging(system_state_t* state);
void accumulate_log_files(system_state_t* state);
void simulate_configuration_changes(system_state_t* state);
void simulate_service_restarts(system_state_t* state);
int generate_system_uptime(system_state_t* state, char* output, size_t output_size);
int generate_kernel_messages(system_state_t* state, char* output, size_t output_size);
int generate_syslog(system_state_t* state, char* output, size_t output_size);
int get_realistic_uptime_seconds(void);
time_t get_realistic_boot_time(void);
void add_log_entry(system_state_t* state,
                   const char* level,
                   const char* source,
                   const char* message);
void free_system_state(system_state_t* state);

#endif // TEMPORAL_H
