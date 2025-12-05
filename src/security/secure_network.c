#include <network.h>
#include <security_utils.h>
#include <utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Forward declarations for functions from network.c
static void generate_ip_in_subnet(const char* base_ip, char* ip_out, size_t size);
static void get_subnet_mask(int prefix_len, char* mask_out, size_t size);
static char* get_random_interface_name(void);
static void randomize_interface_mtus(network_config_t* config);

// Secure version of network configuration creation
network_config_t* create_secure_network_config(const char* base_ip) {
    // Validate input parameters
    if (!base_ip) {
        log_event_level(LOG_ERROR, "create_secure_network_config: base_ip is NULL");
        return NULL;
    }
    
    if (sec_validate_ip_address(base_ip) != SEC_VALID) {
        log_event_level(LOG_ERROR, "create_secure_network_config: Invalid IP address format");
        return NULL;
    }
    
    // Allocate memory with safety check
    network_config_t* config = (network_config_t*)malloc(sizeof(network_config_t));
    if (!config) {
        log_event_level(LOG_ERROR, "create_secure_network_config: Memory allocation failed");
        return NULL;
    }

    // Initialize memory securely
    memset(config, 0, sizeof(network_config_t));

    // Primary interface setup with validation
    config->interface_count = 1;
    
    // Get and validate interface name
    const char* iface_name = get_random_interface_name();
    if (sec_validate_interface_name(iface_name) != SEC_VALID) {
        log_event_level(LOG_ERROR, "create_secure_network_config: Invalid interface name generated");
        free(config);
        return NULL;
    }
    
    // Safe string copy with bounds checking
    if (sec_safe_strcpy(config->interfaces[0].name, iface_name, MAX_INTERFACE_NAME) != SEC_VALID) {
        log_event_level(LOG_ERROR, "create_secure_network_config: Interface name copy failed");
        free(config);
        return NULL;
    }
    
    // Generate IP in subnet (already has validation)
    generate_ip_in_subnet(base_ip, config->interfaces[0].ip_address, MAX_IP_ADDR);
    
    // Get subnet mask
    get_subnet_mask(24, config->interfaces[0].netmask, MAX_IP_ADDR);
    
    // Safe gateway copy
    if (sec_safe_strcpy(config->interfaces[0].gateway, base_ip, MAX_IP_ADDR) != SEC_VALID) {
        log_event_level(LOG_ERROR, "create_secure_network_config: Gateway copy failed");
        free(config);
        return NULL;
    }
    
    // Set MTU with validation
    config->interfaces[0].mtu = 1500;
    if (sec_validate_numeric_range(config->interfaces[0].mtu, 576, 9000) != SEC_VALID) {
        log_event_level(LOG_ERROR, "create_secure_network_config: Invalid MTU value");
        free(config);
        return NULL;
    }
    
    config->interfaces[0].is_primary = true;

    // Optional secondary interface with validation
    if (rand() % 100 < 20) {
        config->interface_count = 2;
        
        const char* secondary_iface = get_random_interface_name();
        if (sec_validate_interface_name(secondary_iface) == SEC_VALID) {
            sec_safe_strcpy(config->interfaces[1].name, secondary_iface, MAX_INTERFACE_NAME);
        }
        
        // Generate safe secondary IP
        snprintf(config->interfaces[1].ip_address, MAX_IP_ADDR, "10.0.%d.1", rand() % 256);
        sec_safe_strcpy(config->interfaces[1].netmask, "255.255.255.0", MAX_IP_ADDR);
        sec_safe_strcpy(config->interfaces[1].gateway, "10.0.0.254", MAX_IP_ADDR);
        config->interfaces[1].mtu = 1500;
        config->interfaces[1].is_primary = false;
    }

    log_event_level(LOG_INFO, "create_secure_network_config: Network configuration created successfully");
    return config;
}

// Secure version of interface variations
void generate_secure_interface_variations(network_config_t* config) {
    if (!config) {
        log_event_level(LOG_ERROR, "generate_secure_interface_variations: config is NULL");
        return;
    }
    
    if (config->interface_count == 0) {
        log_event_level(LOG_WARN, "generate_secure_interface_variations: No interfaces to vary");
        return;
    }

    // Randomize MTU sizes with validation
    if (rand() % 100 < 30) {
        randomize_interface_mtus(config);
        
        // Validate all MTU values
        for (int i = 0; i < config->interface_count; i++) {
            if (sec_validate_numeric_range(config->interfaces[i].mtu, 576, 9000) != SEC_VALID) {
                log_event_level(LOG_WARN, "generate_secure_interface_variations: Invalid MTU detected, resetting to 1500");
                config->interfaces[i].mtu = 1500;
            }
        }
    }
}

// Secure version of file reading with validation
int read_file_secure(const char* filepath, char* buffer, size_t buffer_size) {
    // Validate inputs
    if (!filepath || !buffer) {
        log_event_level(LOG_ERROR, "read_file_secure: NULL parameters");
        return -1;
    }
    
    if (sec_validate_path(filepath, false) != SEC_VALID) {
        log_event_level(LOG_ERROR, "read_file_secure: Invalid file path");
        return -1;
    }
    
    if (sec_validate_buffer(buffer, buffer_size, SEC_SAFE_BUFFER_SIZE) != SEC_VALID) {
        log_event_level(LOG_ERROR, "read_file_secure: Invalid buffer");
        return -1;
    }
    
    FILE* f = fopen(filepath, "r");
    if (!f) {
        log_event_level(LOG_ERROR, "read_file_secure: Failed to open file");
        return -1;
    }
    
    // Safe read with bounds checking
    size_t bytes_read = fread(buffer, 1, buffer_size - 1, f);
    buffer[bytes_read] = '\0';  // Ensure null termination
    
    fclose(f);
    
    log_event_level(LOG_DEBUG, "read_file_secure: Successfully read file");
    return (int)bytes_read;
}

// Secure version of file writing with validation
int write_file_secure(const char* filepath, const char* content) {
    // Validate inputs
    if (!filepath || !content) {
        log_event_level(LOG_ERROR, "write_file_secure: NULL parameters");
        return -1;
    }
    
    if (sec_validate_path(filepath, false) != SEC_VALID) {
        log_event_level(LOG_ERROR, "write_file_secure: Invalid file path");
        return -1;
    }
    
    if (sec_validate_string(content, SEC_MAX_STRING_LENGTH, false) != SEC_VALID) {
        log_event_level(LOG_ERROR, "write_file_secure: Content too long or invalid");
        return -1;
    }
    
    FILE* f = fopen(filepath, "w");
    if (!f) {
        log_event_level(LOG_ERROR, "write_file_secure: Failed to open file for writing");
        return -1;
    }
    
    int result = fprintf(f, "%s", content);
    fclose(f);
    
    if (result >= 0) {
        log_event_level(LOG_DEBUG, "write_file_secure: Successfully wrote file");
        return 0;
    } else {
        log_event_level(LOG_ERROR, "write_file_secure: Failed to write file");
        return -1;
    }
}

// Secure string replacement function
int secure_replace_string(char* str, const char* old, const char* new_str, size_t max_length) {
    if (!str || !old || !new_str) {
        return -1;
    }
    
    if (sec_validate_string(str, max_length, false) != SEC_VALID ||
        sec_validate_string(old, max_length, false) != SEC_VALID ||
        sec_validate_string(new_str, max_length, false) != SEC_VALID) {
        return -1;
    }
    
    // Check for safe multiplication to prevent overflow
    size_t old_len = strlen(old);
    size_t new_len = strlen(new_str);
    
    if (!sec_is_safe_multiplication(old_len, 2) || !sec_is_safe_multiplication(new_len, 2)) {
        return -1;
    }
    
    // Simple implementation - in production, use more sophisticated approach
    char* pos = strstr(str, old);
    if (pos) {
        size_t remaining = max_length - (pos - str) - old_len;
        if (new_len >= remaining) {
            return -1;  // Would overflow
        }
        
        // Shift remaining characters
        memmove(pos + new_len, pos + old_len, strlen(pos + old_len) + 1);
        memcpy(pos, new_str, new_len);
    }
    
    return 0;
}