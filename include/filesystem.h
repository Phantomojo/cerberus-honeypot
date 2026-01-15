#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define MAX_FILES 100
#define MAX_PATH_LEN 512
#define MAX_FILENAME 256

// File entry structure
typedef struct {
    char path[MAX_PATH_LEN];
    char name[MAX_FILENAME];
    off_t size;
    time_t access_time;
    time_t modify_time;
    time_t change_time;
    mode_t permissions;
    uid_t owner;
    gid_t group;
    bool is_directory;
    bool is_symlink;
} file_entry_t;

// Filesystem snapshot
typedef struct {
    file_entry_t files[MAX_FILES];
    int file_count;
    char root_path[MAX_PATH_LEN];
} filesystem_snapshot_t;

// Available commands per device profile
typedef struct {
    char* command_name;
    bool available;
    char* output_file; // Location of predefined output
} device_command_t;

// Functions
filesystem_snapshot_t* create_filesystem_snapshot(const char* base_path);
void generate_random_timestamps(filesystem_snapshot_t* fs, time_t base_time);
void generate_directory_variations(filesystem_snapshot_t* fs);
void generate_file_size_variations(filesystem_snapshot_t* fs);
void create_session_log_files(filesystem_snapshot_t* fs, const char* session_id);
void vary_permissions(filesystem_snapshot_t* fs);
char** get_device_specific_commands(const char* device_type, int* count);
char** get_available_binaries(const char* architecture, int* count);
int generate_ls_output(filesystem_snapshot_t* fs,
                       const char* path,
                       char* output,
                       size_t output_size);
int generate_find_output(filesystem_snapshot_t* fs,
                         const char* pattern,
                         char* output,
                         size_t output_size);
int generate_du_output(filesystem_snapshot_t* fs, char* output, size_t output_size);
void free_filesystem_snapshot(filesystem_snapshot_t* fs);

#endif // FILESYSTEM_H
