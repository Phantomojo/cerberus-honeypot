#ifndef SECURITY_UTILS_H
#define SECURITY_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

// Security validation result codes
typedef enum {
    SEC_VALID = 0,
    SEC_NULL_POINTER = -1,
    SEC_BUFFER_OVERFLOW = -2,
    SEC_INVALID_LENGTH = -3,
    SEC_INVALID_CHARACTER = -4,
    SEC_OUT_OF_RANGE = -5,
    SEC_STRING_TOO_LONG = -6
} sec_result_t;

// Input validation functions
sec_result_t sec_validate_string(const char* input, size_t max_length, bool allow_null);
sec_result_t sec_validate_filename(const char* filename);
sec_result_t sec_validate_ip_address(const char* ip);
sec_result_t sec_validate_interface_name(const char* name);
sec_result_t sec_validate_path(const char* path, bool allow_absolute);
sec_result_t sec_validate_numeric_range(int value, int min_val, int max_val);
sec_result_t sec_validate_port_number(int port);

// Safe string operations with bounds checking
sec_result_t sec_safe_strcpy(char* dest, const char* src, size_t dest_size);
sec_result_t sec_safe_strcat(char* dest, const char* src, size_t dest_size);
sec_result_t sec_safe_snprintf(char* dest, size_t dest_size, const char* format, ...);

// Buffer validation
sec_result_t sec_validate_buffer(const void* buffer, size_t size, size_t max_size);
sec_result_t sec_validate_buffer_bounds(const void* buffer, size_t size, const void* base, size_t base_size);

// Memory safety checks
bool sec_is_safe_size_t(size_t size);
bool sec_is_safe_multiplication(size_t a, size_t b);
bool sec_is_safe_addition(size_t a, size_t b);

// Character validation
bool sec_contains_only_safe_chars(const char* str, const char* allowed_chars);
bool sec_contains_dangerous_chars(const char* str);

// Length validation
bool sec_is_safe_string_length(const char* str, size_t max_length);

// Sanitization functions
sec_result_t sec_sanitize_string(char* str, size_t max_length);
sec_result_t sec_sanitize_filename(char* filename);
sec_result_t sec_sanitize_path(char* path);

// Security constants
#define SEC_MAX_STRING_LENGTH 1024
#define SEC_MAX_FILENAME_LENGTH 255
#define SEC_MAX_PATH_LENGTH 4096
#define SEC_MAX_INTERFACE_NAME 32
#define SEC_MAX_IP_ADDRESS 45
#define SEC_SAFE_BUFFER_SIZE 8192

// Safe character sets
extern const char* SEC_SAFE_FILENAME_CHARS;
extern const char* SEC_SAFE_INTERFACE_CHARS;
extern const char* SEC_SAFE_IP_CHARS;

#endif // SECURITY_UTILS_H