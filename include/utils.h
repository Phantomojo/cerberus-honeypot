#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>

// Logging functions
typedef enum { LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR } log_level_t;

void log_event(const char* msg);
void log_event_level(log_level_t level, const char* msg);
void log_to_file(const char* filepath, const char* msg);
void log_event_file(log_level_t level, const char* filepath, const char* msg);

// File I/O functions
bool file_exists(const char* filepath);
bool dir_exists(const char* dirpath);
int read_file(const char* filepath, char* buffer, size_t buffer_size);
int write_file(const char* filepath, const char* content);
int append_file(const char* filepath, const char* content);
int copy_file(const char* src, const char* dst);

// String utilities
void trim_string(char* str);
void replace_string(char* str, const char* old, const char* new_str);
char* str_dup(const char* str);

// Path utilities
int create_dir(const char* dirpath);
int get_file_size(const char* filepath);
int execute_command_safely(const char* cmd, char* const argv[]);

// Path security functions
bool is_safe_path(const char* filepath);
int sanitize_path(const char* input_path, char* output_path, size_t output_size);
bool is_path_traversal(const char* filepath);

// Configuration helpers
int read_config_value(const char* filepath, const char* key, char* value, size_t value_size);
int write_config_value(const char* filepath, const char* key, const char* value);

#endif // UTILS_H
