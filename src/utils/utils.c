#include "utils.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Logging functions
void log_event(const char* msg) {
    log_event_level(LOG_INFO, msg);
}

void log_event_level(log_level_t level, const char* msg) {
    time_t now = time(NULL);
    char* time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0'; // Remove newline

    const char* level_str;
    switch (level) {
        case LOG_DEBUG:
            level_str = "DEBUG";
            break;
        case LOG_INFO:
            level_str = "INFO";
            break;
        case LOG_WARN:
            level_str = "WARN";
            break;
        case LOG_ERROR:
            level_str = "ERROR";
            break;
        default:
            level_str = "INFO";
            break;
    }

    printf("[%s] [%s] %s\n", time_str, level_str, msg);
}

void log_to_file(const char* filepath, const char* msg) {
    int fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd >= 0) {
        fchmod(fd, 0600);
        FILE* f = fdopen(fd, "a");
        if (f) {
            time_t now = time(NULL);
            char* time_str = ctime(&now);
            time_str[strlen(time_str) - 1] = '\0';
            fprintf(f, "[%s] %s\n", time_str, msg);
            fclose(f);
        } else {
            close(fd);
        }
    }
}

void log_event_file(log_level_t level, const char* filepath, const char* msg) {
    int fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd >= 0) {
        fchmod(fd, 0600);
        FILE* f = fdopen(fd, "a");
        if (f) {
            time_t now = time(NULL);
            char* time_str = ctime(&now);
            time_str[strlen(time_str) - 1] = '\0';

            const char* level_str;
            switch (level) {
                case LOG_DEBUG:
                    level_str = "DEBUG";
                    break;
                case LOG_INFO:
                    level_str = "INFO";
                    break;
                case LOG_WARN:
                    level_str = "WARN";
                    break;
                case LOG_ERROR:
                    level_str = "ERROR";
                    break;
                default:
                    level_str = "INFO";
                    break;
            }

            fprintf(f, "[%s] [%s] %s\n", time_str, level_str, msg);
            fclose(f);
        } else {
            close(fd);
        }
    }
    // Also log to stdout
    log_event_level(level, msg);
}

// File I/O functions
bool file_exists(const char* filepath) {
    struct stat st;
    return (stat(filepath, &st) == 0 && S_ISREG(st.st_mode));
}

bool dir_exists(const char* dirpath) {
    struct stat st;
    return (stat(dirpath, &st) == 0 && S_ISDIR(st.st_mode));
}

int read_file(const char* filepath, char* buffer, size_t buffer_size) {
    FILE* f = fopen(filepath, "r");
    if (!f) {
        return -1;
    }

    size_t bytes_read = fread(buffer, 1, buffer_size - 1, f);
    buffer[bytes_read] = '\0';
    fclose(f);

    return (int)bytes_read;
}

int write_file(const char* filepath, const char* content) {
    int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) {
        return -1;
    }
    fchmod(fd, 0600);
    FILE* f = fdopen(fd, "w");
    if (!f) {
        close(fd);
        return -1;
    }

    int result = fprintf(f, "%s", content);
    fclose(f);

    return (result >= 0) ? 0 : -1;
}

int append_file(const char* filepath, const char* content) {
    int fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd < 0) {
        return -1;
    }
    fchmod(fd, 0600);
    FILE* f = fdopen(fd, "a");
    if (!f) {
        close(fd);
        return -1;
    }

    int result = fprintf(f, "%s", content);
    fclose(f);

    return (result >= 0) ? 0 : -1;
}

int copy_file(const char* src, const char* dst) {
    FILE* src_f = fopen(src, "r");
    if (!src_f) {
        return -1;
    }

    int fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd < 0) {
        fclose(src_f);
        return -1;
    }
    fchmod(fd, 0600);
    FILE* dst_f = fdopen(fd, "w");
    if (!dst_f) {
        close(fd);
        fclose(src_f);
        return -1;
    }

    char buffer[4096];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src_f)) > 0) {
        if (fwrite(buffer, 1, bytes, dst_f) != bytes) {
            fclose(src_f);
            fclose(dst_f);
            return -1;
        }
    }

    fclose(src_f);
    fclose(dst_f);

    return 0;
}

// String utilities
void trim_string(char* str) {
    if (!str)
        return;

    // Trim leading whitespace
    char* start = str;
    while (isspace((unsigned char)*start))
        start++;

    // Trim trailing whitespace
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;
    end[1] = '\0';

    // Move trimmed string to start
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

// WARNING: This function has limitations and is not currently used
// - The destination buffer must be large enough to hold the result
// - Uses a fixed 4096-byte internal buffer which could overflow
// - Consider using a safer alternative for production code
void replace_string(char* str, const char* old, const char* new_str) {
    if (!str || !old || !new_str || old[0] == '\0')
        return;

    char buffer[4096];
    size_t out = 0;
    size_t in_len = strlen(str);
    size_t old_len = strlen(old);
    size_t new_len = strlen(new_str);

    size_t i = 0;
    while (i < in_len && out + 1 < sizeof(buffer)) {
        if (old_len > 0 && i <= in_len - old_len && memcmp(&str[i], old, old_len) == 0) {
            size_t to_copy = new_len;
            if (to_copy > sizeof(buffer) - 1 - out) {
                to_copy = sizeof(buffer) - 1 - out;
            }
            memcpy(&buffer[out], new_str, to_copy);
            out += to_copy;
            i += old_len;
        } else {
            buffer[out++] = str[i++];
        }
    }
    buffer[out] = '\0';

    // Copy back safely: truncate to original length to avoid overflow
    size_t result_len = strlen(buffer);
    size_t max_copy = result_len < in_len ? result_len : in_len;
    memcpy(str, buffer, max_copy);
    str[max_copy] = '\0';
}

char* str_dup(const char* str) {
    if (!str)
        return NULL;
    size_t len = strlen(str) + 1;
    char* dup = malloc(len);
    if (dup) {
        memcpy(dup, str, len);
    }
    return dup;
}

// Path utilities
int create_dir(const char* dirpath) {
    if (dir_exists(dirpath)) {
        return 0; // Already exists
    }

    // Try to create directory
    if (mkdir(dirpath, 0755) == 0) {
        return 0;
    }

    // If parent doesn't exist, try to create it recursively
    char* path_copy = str_dup(dirpath);
    char* last_slash = strrchr(path_copy, '/');

    if (last_slash && last_slash != path_copy) {
        *last_slash = '\0';
        if (create_dir(path_copy) == 0) {
            free(path_copy);
            return mkdir(dirpath, 0755) == 0 ? 0 : -1;
        }
    }

    free(path_copy);
    return -1;
}

int get_file_size(const char* filepath) {
    struct stat st;
    if (stat(filepath, &st) == 0) {
        return (int)st.st_size;
    }
    return -1;
}

// Configuration helpers
int read_config_value(const char* filepath, const char* key, char* value, size_t value_size) {
    FILE* f = fopen(filepath, "r");
    if (!f) {
        return -1;
    }

    char line[512];
    size_t key_len = strlen(key);

    while (fgets(line, sizeof(line), f)) {
        trim_string(line);

        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\0') {
            continue;
        }

        // Check if line starts with key
        if (strncmp(line, key, key_len) == 0 && line[key_len] == '=') {
            // Extract value after '='
            char* val_start = line + key_len + 1;
            trim_string(val_start);

            // Remove quotes if present
            if ((val_start[0] == '"' && val_start[strlen(val_start) - 1] == '"') ||
                (val_start[0] == '\'' && val_start[strlen(val_start) - 1] == '\'')) {
                val_start++;
                val_start[strlen(val_start) - 1] = '\0';
            }

            strncpy(value, val_start, value_size - 1);
            value[value_size - 1] = '\0';
            fclose(f);
            return 0;
        }
    }

    fclose(f);
    return -1; // Key not found
}

int write_config_value(const char* filepath, const char* key, const char* value) {
    // Simple implementation: append to file
    // For production, should read, modify, and rewrite
    int fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0600);
    if (fd < 0) {
        return -1;
    }
    fchmod(fd, 0600);
    FILE* f = fdopen(fd, "a");
    if (!f) {
        close(fd);
        return -1;
    }

    int rc = fprintf(f, "%s=%s\n", key, value);
    fclose(f);
    return (rc >= 0) ? 0 : -1;
}

int execute_command_safely(const char* cmd, char* const argv[]) {
    if (!cmd || !argv)
        return -1;

    pid_t pid = fork();
    if (pid == -1) {
        log_event_level(LOG_ERROR, "execute_command_safely: fork failed");
        return -1;
    }

    if (pid == 0) {
        // Child process
        execvp(cmd, argv);
        // If we reach here, exec failed
        perror("execvp");
        exit(1);
    }

    // Parent process
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        log_event_level(LOG_ERROR, "execute_command_safely: waitpid failed");
        return -1;
    }

    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return -1;
}
