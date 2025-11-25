#ifndef PROCESSES_H
#define PROCESSES_H

#include <stdbool.h>
#include <time.h>
#include <stdint.h>

#define MAX_PROCESSES 100
#define MAX_PROCESS_NAME 64
#define MAX_COMMAND_LINE 256

// Process structure
typedef struct {
    pid_t pid;
    uid_t uid;
    char name[MAX_PROCESS_NAME];
    char command[MAX_COMMAND_LINE];
    uint32_t memory_kb;
    uint32_t cpu_percent;
    time_t start_time;
    char state;  // R, S, D, Z, T, W, X
    uint32_t thread_count;
} process_t;

// Process list snapshot
typedef struct {
    process_t processes[MAX_PROCESSES];
    int process_count;
    uint32_t total_memory_kb;
    time_t snapshot_time;
} process_list_t;

// Available services by profile
typedef struct {
    char* service_name;
    char* command;
    bool should_run;
    uid_t run_as;
} service_t;

// Functions
process_list_t* create_process_list(const char* device_profile);
void generate_core_processes(process_list_t* processes, const char* device_profile);
void generate_service_processes(process_list_t* processes, const char* device_profile);
void generate_background_processes(process_list_t* processes, int count);
void randomize_pids(process_list_t* processes);
void randomize_memory_usage(process_list_t* processes, uint32_t total_memory);
void randomize_start_times(process_list_t* processes, time_t base_time);
void randomize_cpu_usage(process_list_t* processes);
int generate_ps_output(process_list_t* processes, char* output, size_t output_size);
int generate_ps_aux_output(process_list_t* processes, char* output, size_t output_size);
int generate_top_output(process_list_t* processes, char* output, size_t output_size);
int generate_proc_stat_output(process_list_t* processes, char* output, size_t output_size);
int generate_proc_meminfo_output(uint32_t total_memory, char* output, size_t output_size);
void free_process_list(process_list_t* processes);

#endif // PROCESSES_H
