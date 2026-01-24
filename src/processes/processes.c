/**
 * processes.c - Process simulation module
 *
 * Generates realistic process lists with varying PIDs, memory, and services
 * based on device profile.
 */

#include "processes.h"

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Core system processes (always present)
static const char* core_processes[] = {"init",
                                       "kthreadd",
                                       "ksoftirqd",
                                       "kworker",
                                       "kdevtmpfs",
                                       "watchdog",
                                       "migration",
                                       "kswapd",
                                       "kthrotld",
                                       "khubd",
                                       "syslogd"};
static const int core_processes_count = sizeof(core_processes) / sizeof(core_processes[0]);

// Router-specific services
static const service_t router_services[] = {
    {"dnsmasq", "/usr/sbin/dnsmasq", true, 0},
    {"sshd", "/usr/sbin/sshd", true, 0},
    {"dropbear", "/usr/sbin/dropbear", true, 0},
    {"telnetd", "/usr/sbin/telnetd", true, 0},
    {"httpd", "/usr/sbin/httpd", true, 0},
    {"iptables", "/usr/sbin/iptables", false, 0},
    {"bridge", "/usr/sbin/brctl", false, 0},
    {"hostapd", "/usr/sbin/hostapd", false, 0},
    {"wpa_supplicant", "/usr/sbin/wpa_supplicant", false, 0},
    {"udhcpc", "/usr/bin/udhcpc", false, 0}};
static const int router_services_count = sizeof(router_services) / sizeof(router_services[0]);

// Camera-specific services
static const service_t camera_services[] = {{"mjpeg_streamer", "/usr/bin/mjpeg_streamer", true, 0},
                                            {"ffmpeg", "/usr/bin/ffmpeg", true, 0},
                                            {"motion", "/usr/sbin/motion", true, 0},
                                            {"rtsp", "/usr/bin/rtsp-server", true, 0},
                                            {"sshd", "/usr/sbin/sshd", true, 0},
                                            {"dropbear", "/usr/sbin/dropbear", true, 0},
                                            {"telnetd", "/usr/sbin/telnetd", true, 0},
                                            {"httpd", "/usr/sbin/httpd", false, 0},
                                            {"ntpd", "/usr/sbin/ntpd", false, 0}};
static const int camera_services_count = sizeof(camera_services) / sizeof(camera_services[0]);

/**
 * Create process list for device profile
 */
process_list_t* create_process_list(const char* device_profile) {
    process_list_t* processes = (process_list_t*)malloc(sizeof(process_list_t));
    if (!processes)
        return NULL;

    memset(processes, 0, sizeof(process_list_t));
    processes->snapshot_time = time(NULL);

    // Start with init process (PID 1)
    processes->processes[0].pid = 1;
    processes->processes[0].uid = 0;
    strncpy(processes->processes[0].name, "init", MAX_PROCESS_NAME - 1);
    processes->processes[0].memory_kb = 512;
    processes->processes[0].state = 'S';
    processes->process_count = 1;

    // Generate core processes
    generate_core_processes(processes, device_profile);

    // Generate service processes
    generate_service_processes(processes, device_profile);

    // Generate some random background processes
    generate_background_processes(processes, 3 + (rand() % 5));

    // Randomize everything
    randomize_pids(processes);
    randomize_memory_usage(processes, 128000); // 128MB total
    randomize_cpu_usage(processes);

    return processes;
}

/**
 * Generate core kernel processes
 */
void generate_core_processes(process_list_t* processes, const char* device_profile) {
    (void)device_profile; // Reserved for device-specific kernel process variations
    if (!processes)
        return;

    for (int i = 0; i < core_processes_count && processes->process_count < MAX_PROCESSES; i++) {
        process_t* p = &processes->processes[processes->process_count];
        p->pid = 2 + i;
        p->uid = 0;
        strncpy(p->name, core_processes[i], MAX_PROCESS_NAME - 1);
        snprintf(p->command, MAX_COMMAND_LINE, "[%s]", core_processes[i]);
        p->memory_kb = 100 + (rand() % 500);
        p->cpu_percent = rand() % 5;
        p->state = (rand() % 100 < 80) ? 'S' : 'R'; // Mostly sleeping
        p->thread_count = 1 + (rand() % 3);
        processes->process_count++;
    }
}

/**
 * Generate service processes based on device type
 */
void generate_service_processes(process_list_t* processes, const char* device_profile) {
    if (!processes || !device_profile)
        return;

    const service_t* services = NULL;
    int service_count = 0;

    // Select service list based on profile
    if (strstr(device_profile, "camera") || strstr(device_profile, "Hikvision") ||
        strstr(device_profile, "Dahua")) {
        services = camera_services;
        service_count = camera_services_count;
    } else {
        services = router_services;
        service_count = router_services_count;
    }

    // Add active services
    for (int i = 0; i < service_count && processes->process_count < MAX_PROCESSES; i++) {
        // 70% chance a service is enabled
        if (services[i].should_run || (rand() % 100 < 70)) {
            process_t* p = &processes->processes[processes->process_count];
            p->pid = 100 + i; // Will be randomized later
            p->uid = services[i].run_as;
            strncpy(p->name, services[i].service_name, MAX_PROCESS_NAME - 1);
            strncpy(p->command, services[i].command, MAX_COMMAND_LINE - 1);
            p->memory_kb = 1024 + (rand() % 10240);
            p->cpu_percent = rand() % 20;
            p->state = 'S'; // Services typically sleeping
            p->thread_count = 1 + (rand() % 2);
            processes->process_count++;
        }
    }
}

/**
 * Generate random background processes
 */
void generate_background_processes(process_list_t* processes, int count) {
    if (!processes)
        return;

    static const char* bg_names[] = {"syslog",
                                     "cron",
                                     "ntpd",
                                     "avahi",
                                     "printer",
                                     "scanner",
                                     "monitor",
                                     "logger",
                                     "audit",
                                     "system-update",
                                     "backup"};
    static const int bg_count = sizeof(bg_names) / sizeof(bg_names[0]);

    for (int i = 0; i < count && processes->process_count < MAX_PROCESSES; i++) {
        process_t* p = &processes->processes[processes->process_count];
        int idx = rand() % bg_count;
        p->pid = 200 + i;
        p->uid = (rand() % 100 < 30) ? 0 : (rand() % 1000);
        strncpy(p->name, bg_names[idx], MAX_PROCESS_NAME - 1);
        snprintf(p->command, MAX_COMMAND_LINE, "/usr/sbin/%s", bg_names[idx]);
        p->memory_kb = 512 + (rand() % 5120);
        p->cpu_percent = rand() % 10;
        p->state = 'S';
        p->thread_count = 1;
        processes->process_count++;
    }
}

/**
 * Randomize PIDs (realistic range: 1-30000)
 */
void randomize_pids(process_list_t* processes) {
    if (!processes)
        return;

    for (int i = 0; i < processes->process_count; i++) {
        processes->processes[i].pid = 1 + (rand() % 29999);
    }

    // Ensure unique PIDs
    for (int i = 0; i < processes->process_count; i++) {
        for (int j = i + 1; j < processes->process_count; j++) {
            if (processes->processes[i].pid == processes->processes[j].pid) {
                processes->processes[j].pid = 1 + (rand() % 29999);
            }
        }
    }
}

/**
 * Randomize memory usage
 */
void randomize_memory_usage(process_list_t* processes, uint32_t total_memory) {
    if (!processes)
        return;

    // Distribute memory (some large, some small)
    uint32_t allocated = 0;
    for (int i = 0; i < processes->process_count; i++) {
        uint32_t max_mem = (total_memory - allocated) / (processes->process_count - i);
        processes->processes[i].memory_kb = (rand() % max_mem) / 2;
        allocated += processes->processes[i].memory_kb;
    }

    processes->total_memory_kb = allocated;
}

/**
 * Randomize start times (consistent with system uptime)
 */
void randomize_start_times(process_list_t* processes, time_t base_time) {
    if (!processes)
        return;

    time_t now = time(NULL);
    int max_age = (now - base_time);

    for (int i = 0; i < processes->process_count; i++) {
        int age = rand() % (max_age > 0 ? max_age : 86400);
        processes->processes[i].start_time = now - age;
    }
}

/**
 * Randomize CPU usage
 */
void randomize_cpu_usage(process_list_t* processes) {
    if (!processes)
        return;

    for (int i = 0; i < processes->process_count; i++) {
        processes->processes[i].cpu_percent = rand() % 100;
    }
}

/**
 * Generate ps output
 */
int generate_ps_output(process_list_t* processes, char* output, size_t output_size) {
    if (!processes || !output || output_size < 512)
        return -1;

    output[0] = '\0';
    snprintf(output, output_size, "  PID TTY          TIME CMD\n");

    char buffer[256];
    for (int i = 0; i < processes->process_count && i < 20; i++) {
        process_t* p = &processes->processes[i];
        int minutes = (time(NULL) - p->start_time) / 60;
        int seconds = (time(NULL) - p->start_time) % 60;

        snprintf(buffer,
                 sizeof(buffer),
                 "%5u ?        %02d:%02d %s\n",
                 p->pid,
                 minutes,
                 seconds,
                 p->name);

        if (strlen(output) + strlen(buffer) < output_size) {
            size_t cur = strlen(output);
            size_t rem = (output_size > cur + 1) ? (output_size - 1 - cur) : 0;
            strncat(output, buffer, rem);
        }
    }

    return strlen(output);
}

/**
 * Generate ps aux output
 */
int generate_ps_aux_output(process_list_t* processes, char* output, size_t output_size) {
    if (!processes || !output || output_size < 1024)
        return -1;

    output[0] = '\0';
    snprintf(output,
             output_size,
             "USER       PID %%CPU %%MEM    VSZ   RSS TTY STAT START   TIME COMMAND\n");

    char buffer[256];
    for (int i = 0; i < processes->process_count && i < 30; i++) {
        process_t* p = &processes->processes[i];
        float cpu_pct = (float)p->cpu_percent;
        float mem_pct = ((float)p->memory_kb / processes->total_memory_kb) * 100.0f;

        snprintf(buffer,
                 sizeof(buffer),
                 "%-9u %5u %3.1f %3.1f %6u %5u ?   S    12:00  0:00 %s\n",
                 p->uid,
                 p->pid,
                 cpu_pct,
                 mem_pct,
                 p->memory_kb * 2,
                 p->memory_kb,
                 p->name);

        if (strlen(output) + strlen(buffer) < output_size) {
            size_t cur = strlen(output);
            size_t rem = (output_size > cur + 1) ? (output_size - 1 - cur) : 0;
            strncat(output, buffer, rem);
        }
    }

    return strlen(output);
}

/**
 * Generate top output (simplified)
 */
int generate_top_output(process_list_t* processes, char* output, size_t output_size) {
    if (!processes || !output || output_size < 1024)
        return -1;

    time_t now = time(NULL);
    int hours = (now / 3600) % 24;
    int mins = (now / 60) % 60;
    int uptime_days = now / 86400;
    int uptime_hours = (now % 86400) / 3600;
    int uptime_mins = (now % 3600) / 60;

    snprintf(output,
             output_size,
             "top - %02d:%02d:00 up %d day%s, %2d:%02d, %d user%s, load average: %.2f, %.2f, %.2f\n"
             "Tasks: %d total,  %d running,  %d sleeping,   0 stopped,   0 zombie\n"
             "%%Cpu(s):  %d.0%%us,  %d.0%%sy,  0.0%%ni, %d.0%%id,  0.0%%wa,  0.0%%hi,  0.0%%si,  "
             "0.0%%st\n"
             "KiB Mem : %8u total,%8u free,%8u used,%8u buff/cache\n"
             "KiB Swap: %8u total,%8u free,%8u used,%8u avail Mem\n\n"
             "  PID USER      PR  NI    VIRT    RES %%CPU %%MEM     TIME+ COMMAND\n",
             hours,
             mins,
             uptime_days,
             uptime_days > 1 ? "s" : "",
             uptime_hours,
             uptime_mins,
             1,
             "s",
             (float)(1 + rand() % 3),
             (float)(1 + rand() % 2),
             (float)(1 + rand() % 2),
             processes->process_count,
             1 + (rand() % 3),
             processes->process_count - 2,
             5 + (rand() % 10),
             10 + (rand() % 10),
             80 - (rand() % 20),
             processes->total_memory_kb * 2,
             (processes->total_memory_kb / 2),
             processes->total_memory_kb,
             (processes->total_memory_kb / 4),
             0,
             0,
             0,
             0);

    char buffer[256];
    for (int i = 0; i < processes->process_count && i < 10; i++) {
        process_t* p = &processes->processes[i];
        snprintf(buffer,
                 sizeof(buffer),
                 "%5u %-8u %2d   0 %7u %6u  %3u  %3u   0:00.25 %s\n",
                 p->pid,
                 p->uid,
                 20,
                 p->memory_kb * 2,
                 p->memory_kb,
                 p->cpu_percent,
                 (uint32_t)(((float)p->memory_kb / processes->total_memory_kb) * 100),
                 p->name);

        if (strlen(output) + strlen(buffer) < output_size) {
            size_t cur = strlen(output);
            size_t rem = (output_size > cur + 1) ? (output_size - 1 - cur) : 0;
            strncat(output, buffer, rem);
        }
    }

    return strlen(output);
}

/**
 * Generate /proc/stat output
 */
int generate_proc_stat_output(process_list_t* processes, char* output, size_t output_size) {
    (void)processes; // Reserved for process-based stat generation
    if (!output || output_size < 512)
        return -1;

    snprintf(output,
             output_size,
             "cpu  %u %u %u %u %u %u 0 0 0 0\n"
             "cpu0 %u %u %u %u %u %u 0 0 0 0\n"
             "intr %u %u %u %u\n"
             "ctxt %u\n"
             "btime %lu\n"
             "processes %u\n"
             "procs_running 1\n"
             "procs_blocked 0\n",
             1000 + (rand() % 1000),
             100 + (rand() % 200),
             500 + (rand() % 1000),
             10000 + (rand() % 50000),
             100 + (rand() % 500),
             50 + (rand() % 200),
             1000 + (rand() % 1000),
             100 + (rand() % 200),
             500 + (rand() % 1000),
             10000 + (rand() % 50000),
             100 + (rand() % 500),
             50 + (rand() % 200),
             1000000 + (rand() % 5000000),
             100000 + (rand() % 500000),
             10000 + (rand() % 50000),
             1000 + (rand() % 5000),
             5000 + (rand() % 20000),
             time(NULL) - 86400,
             100 + (rand() % 900));

    return strlen(output);
}

/**
 * Generate /proc/meminfo output
 */
int generate_proc_meminfo_output(uint32_t total_memory, char* output, size_t output_size) {
    if (!output || output_size < 512)
        return -1;

    uint32_t used = total_memory / 3;
    uint32_t free = total_memory - used;
    uint32_t buffers = free / 10;
    uint32_t cached = free / 5;

    snprintf(output,
             output_size,
             "MemTotal:       %8u kB\n"
             "MemFree:        %8u kB\n"
             "MemAvailable:   %8u kB\n"
             "Buffers:        %8u kB\n"
             "Cached:         %8u kB\n"
             "SwapTotal:      %8u kB\n"
             "SwapFree:       %8u kB\n",
             total_memory,
             free,
             free + cached,
             buffers,
             cached,
             total_memory / 4,
             total_memory / 4);

    return strlen(output);
}

/**
 * Free process list
 */
void free_process_list(process_list_t* processes) {
    if (processes) {
        free(processes);
    }
}
