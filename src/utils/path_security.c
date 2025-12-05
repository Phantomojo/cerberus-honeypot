#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <unistd.h>

// Path Security Functions - Protection against path traversal attacks

/**
 * Check if a path contains path traversal attempts
 * Returns true if the path contains suspicious patterns
 */
bool is_path_traversal(const char* filepath) {
    if (!filepath) return true;
    
    // Check for dangerous patterns
    if (strstr(filepath, "../") != NULL) return true;
    if (strstr(filepath, "..\\") != NULL) return true;
    if (strstr(filepath, "/..") != NULL) return true;
    if (strstr(filepath, "\\..") != NULL) return true;
    
    // Check for absolute paths trying to escape
    if (filepath[0] == '/' && strstr(filepath, "/../") != NULL) return true;
    
    // Check for null bytes (path truncation attack)
    size_t len = strlen(filepath);
    for (size_t i = 0; i < len; i++) {
        if (filepath[i] == '\0') return true;
    }
    
    // Check for URL encoding attempts
    if (strstr(filepath, "%2e%2e") != NULL) return true;
    if (strstr(filepath, "%2E%2E") != NULL) return true;
    if (strstr(filepath, "..%2f") != NULL) return true;
    if (strstr(filepath, "..%5c") != NULL) return true;
    
    // Check for double encoding
    if (strstr(filepath, "%252e") != NULL) return true;
    if (strstr(filepath, "%252f") != NULL) return true;
    
    return false;
}

/**
 * Check if a path is safe to use
 * Returns true if the path is considered safe
 */
bool is_safe_path(const char* filepath) {
    if (!filepath) return false;
    if (strlen(filepath) == 0) return false;
    if (strlen(filepath) >= PATH_MAX) return false;
    
    // Reject path traversal attempts
    if (is_path_traversal(filepath)) {
        log_event_level(LOG_WARN, "Path traversal attempt detected");
        return false;
    }
    
    // Reject absolute paths outside project (optional - can be configured)
    // This prevents access to system files like /etc/passwd
    if (filepath[0] == '/' && strncmp(filepath, "/tmp/", 5) != 0) {
        // Allow only specific absolute paths if needed
        // For now, be restrictive
        const char* allowed_prefixes[] = {
            "/tmp/cerberus/",
            "/var/log/cerberus/",
            NULL
        };
        
        bool allowed = false;
        for (int i = 0; allowed_prefixes[i] != NULL; i++) {
            if (strncmp(filepath, allowed_prefixes[i], strlen(allowed_prefixes[i])) == 0) {
                allowed = true;
                break;
            }
        }
        
        if (!allowed) {
            log_event_level(LOG_WARN, "Absolute path outside allowed directories");
            return false;
        }
    }
    
    // Reject paths with suspicious characters
    for (size_t i = 0; i < strlen(filepath); i++) {
        unsigned char c = (unsigned char)filepath[i];
        
        // Allow alphanumeric, dash, underscore, dot, slash
        if (!(isalnum(c) || c == '-' || c == '_' || c == '.' || 
              c == '/' || c == '\\')) {
            log_event_level(LOG_WARN, "Path contains suspicious characters");
            return false;
        }
    }
    
    return true;
}

/**
 * Sanitize a path by resolving relative components and validating
 * Returns 0 on success, -1 on failure
 */
int sanitize_path(const char* input_path, char* output_path, size_t output_size) {
    if (!input_path || !output_path || output_size == 0) {
        return -1;
    }
    
    // First check if the path is safe
    if (!is_safe_path(input_path)) {
        log_event_level(LOG_ERROR, "Unsafe path rejected during sanitization");
        return -1;
    }
    
    // Use realpath to resolve the path (if it exists)
    // Note: realpath requires the file to exist in some implementations
    char resolved_path[PATH_MAX];
    char* result = realpath(input_path, resolved_path);
    
    if (result != NULL) {
        // Path exists and was resolved
        // Check if resolved path is still safe
        if (!is_safe_path(resolved_path)) {
            log_event_level(LOG_ERROR, "Path resolved to unsafe location");
            return -1;
        }
        
        if (strlen(resolved_path) >= output_size) {
            log_event_level(LOG_ERROR, "Resolved path too long for buffer");
            return -1;
        }
        
        strncpy(output_path, resolved_path, output_size - 1);
        output_path[output_size - 1] = '\0';
        return 0;
    }
    
    // If realpath failed (file doesn't exist), manually sanitize
    // Remove ./ and ../ components manually
    char temp_path[PATH_MAX];
    strncpy(temp_path, input_path, sizeof(temp_path) - 1);
    temp_path[sizeof(temp_path) - 1] = '\0';
    
    // Split path into components
    char* components[256];
    int component_count = 0;
    
    char* token = strtok(temp_path, "/");
    while (token != NULL && component_count < 256) {
        if (strcmp(token, ".") == 0) {
            // Skip current directory
        } else if (strcmp(token, "..") == 0) {
            // Go up one directory (but not beyond root)
            if (component_count > 0) {
                component_count--;
            } else {
                // Trying to go above root - security issue
                log_event_level(LOG_ERROR, "Path traversal above root directory");
                return -1;
            }
        } else if (strlen(token) > 0) {
            // Add component
            components[component_count] = strdup(token);
            component_count++;
        }
        token = strtok(NULL, "/");
    }
    
    // Reconstruct path
    output_path[0] = '\0';
    
    // If original path was absolute, start with /
    if (input_path[0] == '/') {
        strcat(output_path, "/");
    }
    
    for (int i = 0; i < component_count; i++) {
        if (i > 0) {
            strcat(output_path, "/");
        }
        strcat(output_path, components[i]);
        free(components[i]);
    }
    
    // Validate final path length
    if (strlen(output_path) >= output_size) {
        log_event_level(LOG_ERROR, "Sanitized path too long for buffer");
        return -1;
    }
    
    return 0;
}

// Secure file opening wrapper
FILE* fopen_safe(const char* filepath, const char* mode) {
    char sanitized_path[PATH_MAX];
    
    // Sanitize the path
    if (sanitize_path(filepath, sanitized_path, sizeof(sanitized_path)) != 0) {
        log_event_level(LOG_ERROR, "Failed to sanitize path for file opening");
        return NULL;
    }
    
    // Additional check: ensure we're not opening sensitive system files
    const char* forbidden_files[] = {
        "/etc/passwd",
        "/etc/shadow",
        "/etc/sudoers",
        "/root/",
        "/boot/",
        NULL
    };
    
    for (int i = 0; forbidden_files[i] != NULL; i++) {
        if (strstr(sanitized_path, forbidden_files[i]) != NULL) {
            log_event_level(LOG_ERROR, "Attempt to access forbidden system file");
            return NULL;
        }
    }
    
    // Open file with sanitized path
    FILE* fp = fopen(sanitized_path, mode);
    
    if (!fp) {
        char msg[512];
        snprintf(msg, sizeof(msg), "Failed to open file (sanitized): %s", sanitized_path);
        log_event_level(LOG_DEBUG, msg);
    }
    
    return fp;
}