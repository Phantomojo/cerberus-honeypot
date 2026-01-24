/**
 * state_engine.h - The Brain of CERBERUS Honeypot
 * ============================================================================
 *
 * THE RUBIK'S CUBE VISION:
 * ------------------------
 * This is the "single source of truth" for the entire fake system.
 * Instead of generating random files independently (which don't correlate),
 * we maintain ONE coherent system state, and ALL outputs derive from it.
 *
 * EXAMPLE OF WHY THIS MATTERS:
 * ----------------------------
 * BAD (old approach):
 *   - /proc/uptime says 45 days
 *   - /var/log/messages has entries from 2 days ago only
 *   - /proc/meminfo says 50 processes worth of memory
 *   - `ps` shows only 12 processes
 *   → Attacker notices inconsistency = HONEYPOT DETECTED
 *
 * GOOD (this approach):
 *   - SystemState says: uptime=45 days, processes=12, boot_time=Oct15
 *   - /proc/uptime → generated from state.uptime_seconds
 *   - /var/log/messages → generated with entries spanning 45 days
 *   - /proc/meminfo → calculated from state.processes memory usage
 *   - `ps` → shows exactly state.processes
 *   → Everything correlates = BELIEVABLE SYSTEM
 *
 * REACTIVE BEHAVIOR:
 * ------------------
 * When attacker does something, we update the state:
 *   - `touch /tmp/foo` → state_add_file("/tmp/foo")
 *   - `rm /tmp/bar` → state_remove_file("/tmp/bar")
 *   - `kill 1234` → state_kill_process(1234)
 * Then next time they query, it reflects their actions!
 *
 * MORPHING:
 * ---------
 * When we morph, we generate a NEW coherent state:
 *   - New uptime (random but consistent)
 *   - New process list (different PIDs, same logical processes)
 *   - New file timestamps (all relative to new boot_time)
 *   - New network config (different IPs, MACs)
 * Everything changes together, staying internally consistent.
 *
 * ============================================================================
 */

#ifndef STATE_ENGINE_H
#define STATE_ENGINE_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>

/* ============================================================================
 * CONFIGURATION LIMITS
 * ============================================================================ */

#define MAX_STATE_PROCESSES 128   /* Max fake processes */
#define MAX_STATE_FILES 512       /* Max tracked files */
#define MAX_STATE_USERS 32        /* Max fake users */
#define MAX_STATE_CONNECTIONS 64  /* Max fake network connections */
#define MAX_STATE_LOG_ENTRIES 256 /* Max log entries in memory */
#define MAX_STATE_INTERFACES 8    /* Max network interfaces */
#define MAX_STATE_MOUNTS 16       /* Max mounted filesystems */
#define MAX_STATE_ENV_VARS 64     /* Max environment variables */

#define MAX_PATH_LENGTH 256
#define MAX_NAME_LENGTH 64
#define MAX_CMDLINE_LENGTH 256
#define MAX_LOG_MESSAGE 512
#define MAX_IP_LENGTH 46 /* IPv6 max */
#define MAX_MAC_LENGTH 18

/* ============================================================================
 * DEVICE PROFILE - What kind of device are we pretending to be?
 * ============================================================================ */

typedef enum {
    DEVICE_TYPE_ROUTER,
    DEVICE_TYPE_CAMERA,
    DEVICE_TYPE_DVR,
    DEVICE_TYPE_NAS,
    DEVICE_TYPE_PRINTER,
    DEVICE_TYPE_GENERIC_IOT
} device_type_t;

typedef enum {
    ARCH_MIPS,
    ARCH_MIPSEL,
    ARCH_ARM,
    ARCH_ARMV7,
    ARCH_AARCH64,
    ARCH_X86,
    ARCH_X86_64
} cpu_arch_t;

typedef struct {
    char name[MAX_NAME_LENGTH];   /* e.g., "TP-Link_Archer_C7" */
    char vendor[MAX_NAME_LENGTH]; /* e.g., "TP-Link" */
    char model[MAX_NAME_LENGTH];  /* e.g., "Archer C7 v4" */
    device_type_t type;           /* Router, Camera, etc. */
    cpu_arch_t architecture;      /* MIPS, ARM, etc. */

    /* Hardware specs (for realistic /proc/cpuinfo, /proc/meminfo) */
    char cpu_model[MAX_NAME_LENGTH]; /* e.g., "MIPS 74Kc V5.0" */
    uint32_t cpu_mhz;                /* CPU frequency */
    uint32_t cpu_cores;              /* Number of cores */
    uint32_t total_ram_kb;           /* Total RAM in KB */
    uint32_t total_flash_kb;         /* Total flash storage */
    uint32_t bogomips;               /* BogoMIPS * 100 (for precision) */

    /* Software specs */
    char kernel_version[MAX_NAME_LENGTH];  /* e.g., "3.10.49" */
    char os_name[MAX_NAME_LENGTH];         /* e.g., "OpenWrt" */
    char os_version[MAX_NAME_LENGTH];      /* e.g., "18.06.4" */
    char busybox_version[MAX_NAME_LENGTH]; /* e.g., "1.24.1" */

    /* Network identity */
    char ssh_banner[MAX_CMDLINE_LENGTH];    /* SSH version string */
    char telnet_banner[MAX_CMDLINE_LENGTH]; /* Telnet welcome message */
    char mac_prefix[9];                     /* Vendor MAC prefix "AA:BB:CC" */

} device_profile_t;

/* ============================================================================
 * FAKE USER - Users in /etc/passwd
 * ============================================================================ */

typedef struct {
    char username[MAX_NAME_LENGTH];
    char password_hash[128]; /* For /etc/shadow (fake hash) */
    uid_t uid;
    gid_t gid;
    char home_dir[MAX_PATH_LENGTH];
    char shell[MAX_PATH_LENGTH];
    char gecos[MAX_NAME_LENGTH]; /* Comment field */
    bool is_system_user;         /* daemon, bin, etc. */
    bool can_login;              /* Has valid shell? */
} state_user_t;

/* ============================================================================
 * FAKE PROCESS - Processes in `ps`, /proc/[pid]/*
 * ============================================================================ */

typedef enum {
    PROC_STATE_RUNNING,   /* R */
    PROC_STATE_SLEEPING,  /* S */
    PROC_STATE_DISK_WAIT, /* D */
    PROC_STATE_ZOMBIE,    /* Z */
    PROC_STATE_STOPPED,   /* T */
} proc_state_t;

typedef struct {
    pid_t pid;
    pid_t ppid; /* Parent PID */
    uid_t uid;  /* Owner */
    gid_t gid;
    char name[MAX_NAME_LENGTH];       /* Process name (comm) */
    char cmdline[MAX_CMDLINE_LENGTH]; /* Full command line */
    proc_state_t state;

    /* Resource usage (for ps, top, /proc/[pid]/stat) */
    uint32_t memory_kb;   /* RSS in KB */
    uint32_t virtual_kb;  /* VSZ in KB */
    uint16_t cpu_percent; /* CPU% * 10 (for precision) */
    uint16_t mem_percent; /* MEM% * 10 */

    /* Timing (relative to boot_time for consistency!) */
    uint32_t start_time_offset; /* Seconds after boot when started */
    uint32_t cpu_time_ms;       /* Total CPU time in milliseconds */

    /* Flags */
    bool is_kernel_thread; /* [kworker], [ksoftirqd], etc. */
    bool is_service;       /* Long-running daemon */
    bool visible_in_ps;    /* Some processes hidden */

    /* TTY (for ps output) */
    char tty[16]; /* "pts/0", "tty1", "?" */
} state_process_t;

/* ============================================================================
 * FAKE FILE - Files in the filesystem
 * ============================================================================ */

typedef enum {
    FILE_TYPE_REGULAR,
    FILE_TYPE_DIRECTORY,
    FILE_TYPE_SYMLINK,
    FILE_TYPE_DEVICE_CHAR,
    FILE_TYPE_DEVICE_BLOCK,
    FILE_TYPE_FIFO,
    FILE_TYPE_SOCKET
} file_type_t;

typedef struct {
    char path[MAX_PATH_LENGTH]; /* Full path */
    char name[MAX_NAME_LENGTH]; /* Basename */
    file_type_t type;

    mode_t permissions; /* e.g., 0755 */
    uid_t owner;
    gid_t group;
    off_t size; /* File size in bytes */

    /* Timestamps (as offsets from boot_time for consistency!) */
    int32_t atime_offset; /* Access time: seconds from boot (can be negative = before boot) */
    int32_t mtime_offset; /* Modify time */
    int32_t ctime_offset; /* Change time */

    /* For symlinks */
    char link_target[MAX_PATH_LENGTH];

    /* For devices */
    uint32_t device_major;
    uint32_t device_minor;

    /* Content generation */
    bool has_dynamic_content;   /* Content generated on-demand? */
    char content_generator[64]; /* Function name to generate content */

    /* Flags */
    bool created_by_attacker; /* Track what attacker created */
    bool deleted;             /* Soft-deleted (still in state) */
} state_file_t;

/* ============================================================================
 * NETWORK STATE - Interfaces, connections, routing
 * ============================================================================ */

typedef struct {
    char name[MAX_NAME_LENGTH]; /* eth0, wlan0, br-lan, etc. */
    char ip_address[MAX_IP_LENGTH];
    char netmask[MAX_IP_LENGTH];
    char broadcast[MAX_IP_LENGTH];
    char gateway[MAX_IP_LENGTH];
    char mac_address[MAX_MAC_LENGTH];
    uint32_t mtu;

    /* Statistics (for /proc/net/dev, ifconfig) */
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint32_t rx_errors;
    uint32_t tx_errors;

    bool is_up;
    bool is_loopback;
    bool is_wireless;
} state_interface_t;

typedef enum {
    CONN_STATE_ESTABLISHED,
    CONN_STATE_LISTEN,
    CONN_STATE_TIME_WAIT,
    CONN_STATE_CLOSE_WAIT,
    CONN_STATE_SYN_SENT,
    CONN_STATE_SYN_RECV
} connection_state_t;

typedef struct {
    char protocol[8]; /* tcp, udp, tcp6, udp6 */
    char local_ip[MAX_IP_LENGTH];
    uint16_t local_port;
    char remote_ip[MAX_IP_LENGTH];
    uint16_t remote_port;
    connection_state_t state;
    pid_t owner_pid; /* Which process owns this */
} state_connection_t;

/* ============================================================================
 * LOG ENTRY - For generating realistic logs
 * ============================================================================ */

typedef enum {
    STATE_LOG_DEBUG,
    STATE_LOG_INFO,
    STATE_LOG_NOTICE,
    STATE_LOG_WARNING,
    STATE_LOG_ERROR,
    STATE_LOG_CRITICAL
} state_log_level_t;

typedef struct {
    int32_t time_offset; /* Seconds from boot_time */
    state_log_level_t level;
    char facility[16];             /* kern, auth, daemon, etc. */
    char service[MAX_NAME_LENGTH]; /* dropbear, dnsmasq, kernel */
    pid_t pid;                     /* Optional PID */
    char message[MAX_LOG_MESSAGE];
} state_log_entry_t;

/* ============================================================================
 * MOUNT POINT - For /proc/mounts, df, mount command
 * ============================================================================ */

typedef struct {
    char device[MAX_PATH_LENGTH];      /* /dev/root, tmpfs, etc. */
    char mount_point[MAX_PATH_LENGTH]; /* /, /tmp, /proc, etc. */
    char fs_type[32];                  /* jffs2, tmpfs, proc, etc. */
    char options[128];                 /* rw,noatime,... */

    uint64_t total_kb;     /* Total space */
    uint64_t used_kb;      /* Used space */
    uint64_t available_kb; /* Available space */
} state_mount_t;

/* ============================================================================
 * ATTACKER SESSION - Track what the current attacker has done
 * ============================================================================ */

typedef struct {
    char session_id[64];           /* Unique session identifier */
    time_t connect_time;           /* When they connected (real time) */
    char source_ip[MAX_IP_LENGTH]; /* Attacker's IP */
    uint16_t source_port;

    char username[MAX_NAME_LENGTH];    /* Who they logged in as */
    char current_dir[MAX_PATH_LENGTH]; /* pwd */

    /* Track attacker actions for reactive responses */
    uint32_t commands_executed;
    uint32_t files_created;
    uint32_t files_deleted;
    uint32_t processes_started;

    /* For behavioral analysis */
    time_t last_command_time;
    char last_command[MAX_CMDLINE_LENGTH];

    bool is_suspicious; /* Flagged by quorum? */
} attacker_session_t;

/* ============================================================================
 * MASTER SYSTEM STATE - The Single Source of Truth
 * ============================================================================ */

typedef struct {
    /* === Identity === */
    device_profile_t profile;       /* What device we're pretending to be */
    char hostname[MAX_NAME_LENGTH]; /* Current hostname */
    uint32_t state_seed;            /* Random seed for reproducibility */

    /* === Time (EVERYTHING derives from these!) === */
    time_t boot_time;        /* When the "system" booted (fake) */
    uint32_t uptime_seconds; /* Calculated: now - boot_time */
    time_t last_morph_time;  /* When we last morphed */

    /* === Users === */
    state_user_t users[MAX_STATE_USERS];
    int user_count;

    /* === Processes === */
    state_process_t processes[MAX_STATE_PROCESSES];
    int process_count;
    pid_t next_pid; /* For new processes */

    /* === Files === */
    state_file_t files[MAX_STATE_FILES];
    int file_count;

    /* === Network === */
    state_interface_t interfaces[MAX_STATE_INTERFACES];
    int interface_count;
    state_connection_t connections[MAX_STATE_CONNECTIONS];
    int connection_count;

    /* === Mounts === */
    state_mount_t mounts[MAX_STATE_MOUNTS];
    int mount_count;

    /* === Logs === */
    state_log_entry_t logs[MAX_STATE_LOG_ENTRIES];
    int log_count;
    int log_write_index; /* Circular buffer index */

    /* === Resource Usage (calculated from processes) === */
    uint32_t total_memory_kb;
    uint32_t used_memory_kb;
    uint32_t cached_memory_kb;
    uint32_t buffer_memory_kb;
    uint16_t cpu_usage_percent; /* Overall CPU% * 10 */
    uint16_t load_avg_1;        /* Load average * 100 */
    uint16_t load_avg_5;
    uint16_t load_avg_15;

    /* === Session Tracking === */
    attacker_session_t current_session;
    bool has_active_session;

    /* === Flags === */
    bool is_initialized;
    bool needs_recalculation;     /* State changed, recalc derived values */
    bool emergency_morph_pending; /* Quorum triggered emergency morph */

} system_state_t;

/* ============================================================================
 * STATE ENGINE API - Core Functions
 * ============================================================================ */

/**
 * Initialize the state engine with a device profile
 * This creates a complete, internally consistent fake system
 */
int state_engine_init(system_state_t* state, const device_profile_t* profile);

/**
 * Destroy and clean up state
 */
void state_engine_destroy(system_state_t* state);

/**
 * MORPH - Generate a completely new state while keeping the same profile
 * Everything changes but stays internally consistent
 *
 * @param state The state to morph
 * @param seed Optional seed for reproducibility (0 = random)
 * @return 0 on success
 */
int state_engine_morph(system_state_t* state, uint32_t seed);

/**
 * MORPH with new profile - Change device identity entirely
 */
int state_engine_morph_to_profile(system_state_t* state, const device_profile_t* new_profile);

/**
 * Update derived values (memory usage, CPU, load averages)
 * Call this after making changes to processes, files, etc.
 */
void state_engine_recalculate(system_state_t* state);

/**
 * Update uptime (call periodically or before generating output)
 */
void state_engine_update_time(system_state_t* state);

/* ============================================================================
 * STATE MUTATION API - Modify State (for attacker actions)
 * ============================================================================ */

/* File operations */
int state_add_file(
    system_state_t* state, const char* path, file_type_t type, uid_t owner, mode_t perms);
int state_remove_file(system_state_t* state, const char* path);
int state_modify_file(system_state_t* state, const char* path, off_t new_size);
state_file_t* state_get_file(system_state_t* state, const char* path);
bool state_file_exists(system_state_t* state, const char* path);

/* Process operations */
int state_add_process(
    system_state_t* state, const char* name, const char* cmdline, uid_t owner, pid_t ppid);
int state_kill_process(system_state_t* state, pid_t pid);
state_process_t* state_get_process(system_state_t* state, pid_t pid);
state_process_t* state_get_process_by_name(system_state_t* state, const char* name);

/* User operations */
int state_add_user(
    system_state_t* state, const char* username, uid_t uid, const char* home, const char* shell);
state_user_t* state_get_user(system_state_t* state, const char* username);
state_user_t* state_get_user_by_uid(system_state_t* state, uid_t uid);

/* Log operations */
int state_add_log(system_state_t* state,
                  state_log_level_t level,
                  const char* service,
                  const char* message);

/* Network operations */
int state_add_connection(system_state_t* state,
                         const char* proto,
                         const char* local_ip,
                         uint16_t local_port,
                         const char* remote_ip,
                         uint16_t remote_port,
                         connection_state_t conn_state,
                         pid_t owner);
int state_remove_connection(system_state_t* state, const char* local_ip, uint16_t local_port);

/* ============================================================================
 * OUTPUT GENERATION API - Generate File Contents from State
 * ============================================================================ */

/**
 * Generate content for a specific path
 * The content is derived from the current state
 *
 * @param state The system state
 * @param path The file path (e.g., "/proc/meminfo", "/etc/passwd")
 * @param buffer Output buffer
 * @param buffer_size Size of output buffer
 * @return Number of bytes written, or -1 on error
 */
int state_generate_file_content(system_state_t* state,
                                const char* path,
                                char* buffer,
                                size_t buffer_size);

/* Specific generators (used internally but exposed for flexibility) */
int state_generate_passwd(system_state_t* state, char* buf, size_t size);
int state_generate_shadow(system_state_t* state, char* buf, size_t size);
int state_generate_group(system_state_t* state, char* buf, size_t size);

int state_generate_proc_version(system_state_t* state, char* buf, size_t size);
int state_generate_proc_cpuinfo(system_state_t* state, char* buf, size_t size);
int state_generate_proc_meminfo(system_state_t* state, char* buf, size_t size);
int state_generate_proc_uptime(system_state_t* state, char* buf, size_t size);
int state_generate_proc_loadavg(system_state_t* state, char* buf, size_t size);
int state_generate_proc_mounts(system_state_t* state, char* buf, size_t size);
int state_generate_proc_stat(system_state_t* state, char* buf, size_t size);
int state_generate_proc_net_dev(system_state_t* state, char* buf, size_t size);

int state_generate_ps_output(system_state_t* state, char* buf, size_t size, bool aux_format);
int state_generate_top_output(system_state_t* state, char* buf, size_t size);
int state_generate_netstat_output(system_state_t* state, char* buf, size_t size);
int state_generate_ifconfig_output(system_state_t* state, char* buf, size_t size);
int state_generate_df_output(system_state_t* state, char* buf, size_t size);
int state_generate_free_output(system_state_t* state, char* buf, size_t size);
int state_generate_w_output(system_state_t* state, char* buf, size_t size);
int state_generate_uptime_output(system_state_t* state, char* buf, size_t size);
int state_generate_uname_output(system_state_t* state, char* buf, size_t size, const char* flags);

int state_generate_syslog(system_state_t* state, char* buf, size_t size, int max_entries);
int state_generate_authlog(system_state_t* state, char* buf, size_t size, int max_entries);
int state_generate_dmesg(system_state_t* state, char* buf, size_t size);

int state_generate_ls_output(system_state_t* state,
                             const char* path,
                             char* buf,
                             size_t size,
                             bool long_format,
                             bool show_hidden);

/* ============================================================================
 * SESSION TRACKING API
 * ============================================================================ */

int state_start_session(system_state_t* state,
                        const char* source_ip,
                        uint16_t source_port,
                        const char* username);
void state_end_session(system_state_t* state);
void state_record_command(system_state_t* state, const char* command);
void state_set_current_dir(system_state_t* state, const char* path);

/* ============================================================================
 * SERIALIZATION - Save/Load State
 * ============================================================================ */

/**
 * Save state to file (for persistence across restarts)
 */
int state_save_to_file(system_state_t* state, const char* filepath);

/**
 * Load state from file
 */
int state_load_from_file(system_state_t* state, const char* filepath);

/**
 * Export state as JSON (for debugging, dashboard, AI analysis)
 */
int state_export_json(system_state_t* state, char* buffer, size_t buffer_size);

/* ============================================================================
 * CORRELATION HELPERS - Ensure Internal Consistency
 * ============================================================================ */

/**
 * Recalculate memory usage based on processes
 */
void state_correlate_memory(system_state_t* state);

/**
 * Recalculate load averages based on process states
 */
void state_correlate_load(system_state_t* state);

/**
 * Ensure all timestamps are consistent with boot_time
 */
void state_correlate_timestamps(system_state_t* state);

/**
 * Ensure file ownership matches existing users
 */
void state_correlate_ownership(system_state_t* state);

/**
 * Full correlation pass - call after major state changes
 */
void state_correlate_all(system_state_t* state);

/* ============================================================================
 * PROFILE MANAGEMENT
 * ============================================================================ */

/**
 * Load a device profile from config file
 */
int state_load_profile(device_profile_t* profile, const char* filepath);

/**
 * Get a built-in profile by name
 */
int state_get_builtin_profile(device_profile_t* profile, const char* name);

/**
 * List available built-in profiles
 */
const char** state_list_builtin_profiles(int* count);

/* ============================================================================
 * RANDOMIZATION HELPERS
 * ============================================================================ */

/**
 * Generate random but realistic values
 * These use the state's seed for reproducibility
 */
uint32_t state_rand(system_state_t* state);
uint32_t state_rand_between(system_state_t* state, uint32_t min, uint32_t max);
void state_rand_ip(system_state_t* state, char* buf, size_t size, const char* type);
void state_rand_mac(system_state_t* state, char* buf, size_t size, const char* prefix);
void state_rand_string(system_state_t* state, char* buf, size_t size, const char* charset);

/* ============================================================================
 * GLOBAL STATE ACCESS (for convenience)
 * ============================================================================ */

/**
 * Get the global state instance
 * For single-threaded use only!
 */
system_state_t* state_get_global(void);

/**
 * Initialize global state with profile
 */
int state_init_global(const device_profile_t* profile);

/**
 * Destroy global state
 */
void state_destroy_global(void);

#endif /* STATE_ENGINE_H */
