#include "security_utils.h"

#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Safe character sets
const char* SEC_SAFE_FILENAME_CHARS =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_";
const char* SEC_SAFE_INTERFACE_CHARS =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
const char* SEC_SAFE_IP_CHARS = "0123456789.";

// Input validation functions
sec_result_t sec_validate_string(const char* input, size_t max_length, bool allow_null) {
    if (!input && !allow_null) {
        return SEC_NULL_POINTER;
    }

    if (input && strlen(input) > max_length) {
        return SEC_STRING_TOO_LONG;
    }

    return SEC_VALID;
}

sec_result_t sec_validate_filename(const char* filename) {
    if (!filename) {
        return SEC_NULL_POINTER;
    }

    size_t len = strlen(filename);
    if (len == 0 || len > SEC_MAX_FILENAME_LENGTH) {
        return SEC_INVALID_LENGTH;
    }

    // Check for dangerous patterns
    if (strstr(filename, "..") != NULL) {
        return SEC_INVALID_CHARACTER;
    }

    // Check for safe characters only
    if (!sec_contains_only_safe_chars(filename, SEC_SAFE_FILENAME_CHARS)) {
        return SEC_INVALID_CHARACTER;
    }

    // Don't allow starting with dot (hidden files)
    if (filename[0] == '.') {
        return SEC_INVALID_CHARACTER;
    }

    return SEC_VALID;
}

sec_result_t sec_validate_ip_address(const char* ip) {
    if (!ip) {
        return SEC_NULL_POINTER;
    }

    size_t len = strlen(ip);
    if (len == 0 || len > SEC_MAX_IP_ADDRESS) {
        return SEC_INVALID_LENGTH;
    }

    // Check for safe characters only
    if (!sec_contains_only_safe_chars(ip, SEC_SAFE_IP_CHARS)) {
        return SEC_INVALID_CHARACTER;
    }

    // Basic IP validation - check for 4 octets
    int octets = 0;
    char* ip_copy = strdup(ip);
    char* token = strtok(ip_copy, ".");

    while (token != NULL && octets < 4) {
        // Check if token is numeric and in valid range
        char* endptr;
        long val = strtol(token, &endptr, 10);

        if (*endptr != '\0' || val < 0 || val > 255) {
            free(ip_copy);
            return SEC_INVALID_CHARACTER;
        }

        octets++;
        token = strtok(NULL, ".");
    }

    free(ip_copy);

    if (octets != 4) {
        return SEC_INVALID_CHARACTER;
    }

    return SEC_VALID;
}

sec_result_t sec_validate_interface_name(const char* name) {
    if (!name) {
        return SEC_NULL_POINTER;
    }

    size_t len = strlen(name);
    if (len == 0 || len > SEC_MAX_INTERFACE_NAME) {
        return SEC_INVALID_LENGTH;
    }

    // Check for safe characters only
    if (!sec_contains_only_safe_chars(name, SEC_SAFE_INTERFACE_CHARS)) {
        return SEC_INVALID_CHARACTER;
    }

    return SEC_VALID;
}

sec_result_t sec_validate_path(const char* path, bool allow_absolute) {
    if (!path) {
        return SEC_NULL_POINTER;
    }

    size_t len = strlen(path);
    if (len == 0 || len > SEC_MAX_PATH_LENGTH) {
        return SEC_INVALID_LENGTH;
    }

    // Check for directory traversal attempts
    if (strstr(path, "..") != NULL) {
        return SEC_INVALID_CHARACTER;
    }

    // Check if absolute paths are allowed
    if (!allow_absolute && path[0] == '/') {
        return SEC_INVALID_CHARACTER;
    }

    return SEC_VALID;
}

sec_result_t sec_validate_numeric_range(int value, int min_val, int max_val) {
    if (value < min_val || value > max_val) {
        return SEC_OUT_OF_RANGE;
    }
    return SEC_VALID;
}

sec_result_t sec_validate_port_number(int port) {
    return sec_validate_numeric_range(port, 1, 65535);
}

// Safe string operations with bounds checking
sec_result_t sec_safe_strcpy(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src) {
        return SEC_NULL_POINTER;
    }

    if (dest_size == 0) {
        return SEC_BUFFER_OVERFLOW;
    }

    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        return SEC_BUFFER_OVERFLOW;
    }

    memcpy(dest, src, src_len + 1);
    return SEC_VALID;
}

sec_result_t sec_safe_strcat(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src) {
        return SEC_NULL_POINTER;
    }

    if (dest_size == 0) {
        return SEC_BUFFER_OVERFLOW;
    }

    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);

    if (dest_len + src_len >= dest_size) {
        return SEC_BUFFER_OVERFLOW;
    }

    memcpy(dest + dest_len, src, src_len + 1);
    return SEC_VALID;
}

sec_result_t sec_safe_snprintf(char* dest, size_t dest_size, const char* format, ...) {
    if (!dest || !format) {
        return SEC_NULL_POINTER;
    }

    if (dest_size == 0) {
        return SEC_BUFFER_OVERFLOW;
    }

    va_list args;
    va_start(args, format);

    int result = vsnprintf(dest, dest_size, format, args);
    va_end(args);

    if (result < 0 || (size_t)result >= dest_size) {
        return SEC_BUFFER_OVERFLOW;
    }

    return SEC_VALID;
}

// Buffer validation
sec_result_t sec_validate_buffer(const void* buffer, size_t size, size_t max_size) {
    if (!buffer) {
        return SEC_NULL_POINTER;
    }

    if (size > max_size) {
        return SEC_BUFFER_OVERFLOW;
    }

    return SEC_VALID;
}

sec_result_t
sec_validate_buffer_bounds(const void* buffer, size_t size, const void* base, size_t base_size) {
    if (!buffer || !base) {
        return SEC_NULL_POINTER;
    }

    // Check if buffer is within base bounds
    if (buffer < base || (const char*)buffer + size > (const char*)base + base_size) {
        return SEC_BUFFER_OVERFLOW;
    }

    return SEC_VALID;
}

// Memory safety checks
bool sec_is_safe_size_t(size_t size) {
    return size < SIZE_MAX / 2;
}

bool sec_is_safe_multiplication(size_t a, size_t b) {
    if (a == 0 || b == 0)
        return true;
    return a <= SIZE_MAX / b;
}

bool sec_is_safe_addition(size_t a, size_t b) {
    return a <= SIZE_MAX - b;
}

// Character validation
bool sec_contains_only_safe_chars(const char* str, const char* allowed_chars) {
    if (!str || !allowed_chars) {
        return false;
    }

    for (size_t i = 0; str[i] != '\0'; i++) {
        if (strchr(allowed_chars, str[i]) == NULL) {
            return false;
        }
    }

    return true;
}

bool sec_contains_dangerous_chars(const char* str) {
    if (!str) {
        return false;
    }

    // List of dangerous characters that could cause issues
    const char* dangerous_chars = ";|&`$()<>[]{}'\"\\*?~";

    for (size_t i = 0; str[i] != '\0'; i++) {
        if (strchr(dangerous_chars, str[i]) != NULL) {
            return true;
        }
    }

    return false;
}

// Length validation
bool sec_is_safe_string_length(const char* str, size_t max_length) {
    if (!str) {
        return false;
    }

    return strlen(str) <= max_length;
}

// Sanitization functions
sec_result_t sec_sanitize_string(char* str, size_t max_length) {
    if (!str) {
        return SEC_NULL_POINTER;
    }

    size_t len = strlen(str);
    if (len > max_length) {
        str[max_length] = '\0';
        len = max_length;
    }

    // Remove dangerous characters
    const char* dangerous_chars = ";|&`$()<>[]{}'\"\\*?~";
    size_t write_pos = 0;

    for (size_t read_pos = 0; read_pos < len; read_pos++) {
        if (strchr(dangerous_chars, str[read_pos]) == NULL) {
            str[write_pos++] = str[read_pos];
        }
    }

    str[write_pos] = '\0';
    return SEC_VALID;
}

sec_result_t sec_sanitize_filename(char* filename) {
    sec_result_t result = sec_sanitize_string(filename, SEC_MAX_FILENAME_LENGTH);
    if (result != SEC_VALID) {
        return result;
    }

    // Replace spaces with underscores
    for (size_t i = 0; filename[i] != '\0'; i++) {
        if (filename[i] == ' ') {
            filename[i] = '_';
        }
    }

    return SEC_VALID;
}

sec_result_t sec_sanitize_path(char* path) {
    sec_result_t result = sec_sanitize_string(path, SEC_MAX_PATH_LENGTH);
    if (result != SEC_VALID) {
        return result;
    }

    // Remove directory traversal attempts
    char* src = path;
    char* dst = path;

    while (*src) {
        if (strncmp(src, "../", 3) == 0) {
            src += 3;
        } else if (strncmp(src, "./", 2) == 0) {
            src += 2;
        } else {
            *dst++ = *src++;
        }
    }

    *dst = '\0';
    return SEC_VALID;
}
