#include "encryption.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// Global encryption context
static crypt_context_t g_encrypt_ctx;
static crypt_context_t g_decrypt_ctx;

// Simple XOR-based encryption for demonstration
// In production, this would use proper OpenSSL encryption

void crypt_log_event(const char* event, const char* details) {
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "ENCRYPTION [%s]: %s", event, details ? details : "");
    log_event_level(LOG_INFO, log_msg);
}

void crypt_log_error(const char* function, const char* error) {
    char log_msg[512];
    snprintf(log_msg, sizeof(log_msg), "ENCRYPTION ERROR [%s]: %s", function, error);
    log_event_level(LOG_ERROR, log_msg);
}

bool crypt_is_safe_key_length(size_t len) {
    return (len >= 16 && len <= CRYPT_MAX_KEY_SIZE);
}

bool crypt_constant_time_compare(const uint8_t* a, size_t a_len, const uint8_t* b, size_t b_len) {
    if (a_len != b_len) return false;
    
    volatile uint8_t result = 0;
    for (size_t i = 0; i < a_len; i++) {
        result |= a[i] ^ b[i];
    }
    
    return result == 0;
}

// Check if OpenSSL supports algorithm
bool crypt_is_algorithm_supported(crypt_algorithm_t algorithm) {
    switch (algorithm) {
        case CRYPT_ALGO_AES256_GCM:
            return true;
        case CRYPT_ALGO_CHACHA20_POLY1305:
            return true;
        case CRYPT_ALGO_XCHACHA20_POLY1305:
            return true;
        default:
            return false;
    }
}

const char* crypt_get_algorithm_name(crypt_algorithm_t algorithm) {
    switch (algorithm) {
        case CRYPT_ALGO_AES256_GCM: return "AES-256-GCM";
        case CRYPT_ALGO_CHACHA20_POLY1305: return "ChaCha20-Poly1305";
        case CRYPT_ALGO_XCHACHA20_POLY1305: return "XChaCha20-Poly1305";
        default: return "Unknown";
    }
}

size_t crypt_get_key_size(crypt_algorithm_t algorithm) {
    switch (algorithm) {
        case CRYPT_ALGO_AES256_GCM: return 32;
        case CRYPT_ALGO_CHACHA20_POLY1305: return 32;
        case CRYPT_ALGO_XCHACHA20_POLY1305: return 32;
        default: return 0;
    }
}

size_t crypt_get_iv_size(crypt_algorithm_t algorithm) {
    switch (algorithm) {
        case CRYPT_ALGO_AES256_GCM: return 12;
        case CRYPT_ALGO_CHACHA20_POLY1305: return 12;
        case CRYPT_ALGO_XCHACHA20_POLY1305: return 24;
        default: return 0;
    }
}

size_t crypt_get_tag_size(crypt_algorithm_t algorithm) {
    switch (algorithm) {
        case CRYPT_ALGO_AES256_GCM: return 16;
        case CRYPT_ALGO_CHACHA20_POLY1305: return 16;
        case CRYPT_ALGO_XCHACHA20_POLY1305: return 16;
        default: return 0;
    }
}

// Initialize encryption context
crypt_result_t crypt_init(crypt_context_t* ctx, crypt_algorithm_t algorithm) {
    if (!ctx) {
        crypt_log_error("crypt_init", "NULL context");
        return CRYPT_ERROR_NULL_POINTER;
    }
    
    if (!crypt_is_algorithm_supported(algorithm)) {
        crypt_log_error("crypt_init", "Unsupported algorithm");
        return CRYPT_ERROR_INVALID_ALGORITHM;
    }
    
    // Clear context
    memset(ctx, 0, sizeof(crypt_context_t));
    
    ctx->algorithm = algorithm;
    ctx->key_set = false;
    ctx->initialized = false;
    
    crypt_log_event("crypt_init", "Encryption context initialized");
    return CRYPT_SUCCESS;
}

// Set encryption key
crypt_result_t crypt_set_key(crypt_context_t* ctx, const uint8_t* key, size_t key_len) {
    if (!ctx || !key) {
        crypt_log_error("crypt_set_key", "NULL context or key");
        return CRYPT_ERROR_NULL_POINTER;
    }
    
    if (!crypt_is_safe_key_length(key_len)) {
        crypt_log_error("crypt_set_key", "Invalid key length");
        return CRYPT_ERROR_INVALID_KEY;
    }
    
    if (key_len > CRYPT_MAX_KEY_SIZE) {
        crypt_log_error("crypt_set_key", "Key too long");
        return CRYPT_ERROR_INVALID_KEY;
    }
    
    memcpy(ctx->key, key, key_len);
    ctx->key_set = true;
    
    crypt_log_event("crypt_set_key", "Encryption key set successfully");
    return CRYPT_SUCCESS;
}

// Set additional authenticated data
crypt_result_t crypt_set_aad(crypt_context_t* ctx, const uint8_t* aad, size_t aad_len) {
    if (!ctx || !aad) {
        crypt_log_error("crypt_set_aad", "NULL context or AAD");
        return CRYPT_ERROR_NULL_POINTER;
    }
    
    if (aad_len > CRYPT_MAX_AAD_SIZE) {
        crypt_log_error("crypt_set_aad", "AAD too long");
        return CRYPT_ERROR_BUFFER_TOO_SMALL;
    }
    
    memcpy(ctx->aad, aad, aad_len);
    ctx->aad_len = aad_len;
    
    crypt_log_event("crypt_set_aad", "Additional authenticated data set");
    return CRYPT_SUCCESS;
}

// Simple XOR encryption for demonstration
// In production, this would use proper OpenSSL encryption
crypt_result_t crypt_encrypt(const crypt_context_t* ctx, const uint8_t* plaintext, size_t plaintext_len,
                        uint8_t* ciphertext, size_t* ciphertext_len) {
    if (!ctx || !plaintext || !ciphertext || !ciphertext_len) {
        crypt_log_error("crypt_encrypt", "NULL parameters");
        return CRYPT_ERROR_NULL_POINTER;
    }
    
    if (!ctx->key_set) {
        crypt_log_error("crypt_encrypt", "Key not set");
        return CRYPT_ERROR_INVALID_KEY;
    }
    
    if (plaintext_len > CRYPT_MAX_PLAINTEXT_SIZE) {
        crypt_log_error("crypt_encrypt", "Plaintext too long");
        return CRYPT_ERROR_BUFFER_TOO_SMALL;
    }
    
    // Generate random IV
    if (crypt_generate_random_bytes(ctx->iv, crypt_get_iv_size(ctx->algorithm)) != CRYPT_SUCCESS) {
        return CRYPT_ERROR_INVALID_IV;
    }
    
    // Simple XOR encryption (for demonstration)
    // In production, this would use proper OpenSSL encryption
    size_t key_len = crypt_get_key_size(ctx->algorithm);
    size_t iv_len = crypt_get_iv_size(ctx->algorithm);
    size_t tag_len = crypt_get_tag_size(ctx->algorithm);
    
    // Copy IV to beginning of ciphertext
    memcpy(ciphertext, ctx->iv, iv_len);
    
    // Encrypt plaintext with XOR (simplified)
    for (size_t i = 0; i < plaintext_len; i++) {
        ciphertext[iv_len + i] = plaintext[i] ^ ctx->key[i % key_len];
    }
    
    // Generate simple tag (XOR of all encrypted bytes)
    uint8_t tag = 0;
    for (size_t i = 0; i < plaintext_len; i++) {
        tag ^= ciphertext[iv_len + i];
    }
    
    // Append tag
    ciphertext[iv_len + plaintext_len] = tag;
    
    *ciphertext_len = iv_len + plaintext_len + tag_len;
    
    crypt_log_event("crypt_encrypt", "Encryption completed successfully");
    return CRYPT_SUCCESS;
}

// Simple XOR decryption for demonstration
crypt_result_t crypt_decrypt(const crypt_context_t* ctx, const uint8_t* ciphertext, size_t ciphertext_len,
                        uint8_t* plaintext, size_t* plaintext_len) {
    if (!ctx || !ciphertext || !plaintext || !plaintext_len) {
        crypt_log_error("crypt_decrypt", "NULL parameters");
        return CRYPT_ERROR_NULL_POINTER;
    }
    
    if (!ctx->key_set) {
        crypt_log_error("crypt_decrypt", "Key not set");
        return CRYPT_ERROR_INVALID_KEY;
    }
    
    size_t iv_len = crypt_get_iv_size(ctx->algorithm);
    size_t tag_len = crypt_get_tag_size(ctx->algorithm);
    
    if (ciphertext_len < iv_len + tag_len) {
        crypt_log_error("crypt_decrypt", "Ciphertext too short");
        return CRYPT_ERROR_DECRYPTION_FAILED;
    }
    
    // Extract IV from ciphertext
    uint8_t iv[32];
    memcpy(iv, ciphertext, iv_len);
    
    // Extract encrypted data
    size_t encrypted_len = ciphertext_len - iv_len - tag_len;
    const uint8_t* encrypted_data = ciphertext + iv_len;
    
    // Extract tag
    uint8_t expected_tag = ciphertext[ciphertext_len - 1];
    
    // Decrypt with XOR (simplified)
    size_t key_len = crypt_get_key_size(ctx->algorithm);
    for (size_t i = 0; i < encrypted_len; i++) {
        plaintext[i] = encrypted_data[i] ^ ctx->key[i % key_len];
    }
    
    // Verify tag
    uint8_t computed_tag = 0;
    for (size_t i = 0; i < encrypted_len; i++) {
        computed_tag ^= plaintext[i];
    }
    
    if (computed_tag != expected_tag) {
        crypt_log_error("crypt_decrypt", "Tag verification failed");
        return CRYPT_ERROR_DECRYPTION_FAILED;
    }
    
    *plaintext_len = encrypted_len;
    
    crypt_log_event("crypt_decrypt", "Decryption completed successfully");
    return CRYPT_SUCCESS;
}

// Derive key from password (simplified)
crypt_result_t crypt_derive_key(const uint8_t* password, size_t password_len,
                           const crypt_key_derivation_t* derivation,
                           uint8_t* key, size_t* key_len) {
    if (!password || !derivation || !key || !key_len) {
        crypt_log_error("crypt_derive_key", "NULL parameters");
        return CRYPT_ERROR_NULL_POINTER;
    }
    
    // Simple key derivation (for demonstration)
    // In production, this would use PBKDF2 with HMAC-SHA256
    size_t derived_len = crypt_get_key_size(CRYPT_ALGO_AES256_GCM);
    
    // Simple hash-based derivation
    for (size_t i = 0; i < derived_len; i++) {
        key[i] = password[i % password_len] ^ derivation->salt[i % 16];
        if (derivation->iterations > 0) {
            key[i] ^= (derivation->iterations >> (i % 8)) & 0xFF;
        }
    }
    
    *key_len = derived_len;
    
    crypt_log_event("crypt_derive_key", "Key derivation completed successfully");
    return CRYPT_SUCCESS;
}

// Generate random bytes
crypt_result_t crypt_generate_random_bytes(uint8_t* buffer, size_t len) {
    if (!buffer || len == 0) {
        crypt_log_error("crypt_generate_random_bytes", "Invalid parameters");
        return CRYPT_ERROR_NULL_POINTER;
    }
    
    // Simple random generation using time
    srand(time(NULL));
    for (size_t i = 0; i < len; i++) {
        buffer[i] = rand() % 256;
    }
    
    return CRYPT_SUCCESS;
}

// Securely zero memory
crypt_result_t crypt_secure_zero_memory(void* ptr, size_t len) {
    if (!ptr || len == 0) {
        return CRYPT_SUCCESS;
    }
    
    // Simple memory zeroing
    volatile uint8_t* p = (volatile uint8_t*)ptr;
    for (size_t i = 0; i < len; i++) {
        p[i] = 0;
    }
    
    return CRYPT_SUCCESS;
}

// Check for key derivation attacks
crypt_result_t crypt_check_key_derivation_security(const uint8_t* password, size_t password_len,
                                           const crypt_key_derivation_t* derivation) {
    // Check for common weak passwords
    const char* weak_passwords[] = {
        "password", "123456", "admin", "root", "cerberus", "honeypot"
    };
    
    for (size_t i = 0; i < sizeof(weak_passwords) / sizeof(weak_passwords[0]); i++) {
        if (password_len == strlen(weak_passwords[i]) &&
            memcmp(password, weak_passwords[i], password_len) == 0) {
            crypt_log_event("crypt_check_key_derivation_security", "Weak password detected");
            return CRYPT_ERROR_INVALID_KEY;
        }
    }
    
    // Check for insufficient iterations
    if (derivation->iterations < 10000) {
        crypt_log_event("crypt_check_key_derivation_security", "Insufficient PBKDF2 iterations");
        return CRYPT_ERROR_INVALID_KEY;
    }
    
    return CRYPT_SUCCESS;
}