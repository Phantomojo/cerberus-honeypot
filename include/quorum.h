#ifndef QUORUM_H
#define QUORUM_H

#include <stdbool.h>
#include <time.h>

#define MAX_IPS 1000
#define MAX_IP_STRING 46 // IPv6 max length
#define MAX_SERVICES 10
#define MAX_SERVICE_NAME 64
#define MAX_LOG_LINE 2048

typedef struct {
    char ip[MAX_IP_STRING];
    int service_count;
    char services[MAX_SERVICES][MAX_SERVICE_NAME];
    time_t first_seen;
    time_t last_seen;
    time_t last_hit_time; // NEW: Track the exact last hit for intensity decay
    float intensity;      // NEW: Progressive score (0.0 - 100.0)
    int hit_count;
    int kill_chain_state; // 0=RECON, 1=ACCESS, 2=EXPLOIT, 3=PERSIST
} ip_tracking_t;

typedef struct {
    char name[MAX_SERVICE_NAME];
    char log_path[MAX_LOG_LINE];
    bool enabled;
} service_config_t;

// Quorum detection functions
int run_quorum_logic(void);
int scan_logs_for_ips(void);
int detect_coordinated_attacks(void);
int parse_log_file(const char* filepath, const char* service_name);
int extract_ip_from_line(const char* line, char* ip_out);
bool is_ip_in_tracking(const char* ip);
int add_ip_to_tracking(const char* ip, const char* service);
int generate_alert(const ip_tracking_t* ip_track);

// Configuration
int load_service_configs(const char* config_file);
int add_service_config(const char* name, const char* log_path);

// Statistics
int get_tracked_ip_count(void);
ip_tracking_t* get_tracked_ip(int index);

#endif // QUORUM_H
