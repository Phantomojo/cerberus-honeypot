#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Encryption result codes
typedef enum {
    CRYPT_SUCCESS = 0,
    CRYPT_ERROR_NULL_POINTER = -1,
    CRYPT_ERROR_INVALID_KEY = -2,
    CRYPT_ERROR_INVALID_IV = -3,
    CRYPT_ERROR_BUFFER_TOO_SMALL = -4,
    CRYPT_ERROR_ENCRYPTION_FAILED = -5,
    CRYPT_ERROR_DECRYPTION_FAILED = -6,
    CRYPT_ERROR_KEY_DERIVATION_FAILED = -7,
    CRYPT_ERROR_INVALID_ALGORITHM = -8
} crypt_result_t;

// Encryption algorithms
typedef enum {
    CRYPT_ALGO_AES256_GCM = 0,
    CRYPT_ALGO_CHACHA20_POLY1305 = 1,
    CRYPT_ALGO_XCHACHA20_POLY1305 = 2
} crypt_algorithm_t;

// Encryption context
typedef struct {
    crypt_algorithm_t algorithm;
    uint8_t key[32];  // Encryption key
    uint8_t iv[12];   // Initialization vector
    uint8_t aad[64];  // Additional authenticated data
    size_t aad_len;   // Length of AAD
    bool key_set;     // Whether key is set
    bool initialized; // Whether context is initialized
} crypt_context_t;

// Key derivation
typedef struct {
    uint8_t salt[16];    // Salt for key derivation
    uint32_t iterations; // PBKDF2 iterations
    uint8_t info[32];    // Context info for key derivation
} crypt_key_derivation_t;

// Encryption constants
#define CRYPT_MAX_KEY_SIZE 32
#define CRYPT_MAX_IV_SIZE 12
#define CRYPT_MAX_AAD_SIZE 64
#define CRYPT_MAX_PLAINTEXT_SIZE 4096
#define CRYPT_MAX_CIPHERTEXT_SIZE (CRYPT_MAX_PLAINTEXT_SIZE + 16) // +16 for GCM tag
#define CRYPT_PBKDF2_ITERATIONS 100000
#define CRYPT_KEY_DERIVATION_INFO "cerberus-honeypot-v1"

// Function declarations
crypt_result_t crypt_init(crypt_context_t* ctx, crypt_algorithm_t algorithm);
crypt_result_t crypt_set_key(crypt_context_t* ctx, const uint8_t* key, size_t key_len);
crypt_result_t crypt_set_aad(crypt_context_t* ctx, const uint8_t* aad, size_t aad_len);
crypt_result_t crypt_encrypt(const crypt_context_t* ctx,
                             const uint8_t* plaintext,
                             size_t plaintext_len,
                             uint8_t* ciphertext,
                             size_t* ciphertext_len);
crypt_result_t crypt_decrypt(const crypt_context_t* ctx,
                             const uint8_t* ciphertext,
                             size_t ciphertext_len,
                             uint8_t* plaintext,
                             size_t* plaintext_len);
crypt_result_t crypt_derive_key(const uint8_t* password,
                                size_t password_len,
                                const crypt_key_derivation_t* derivation,
                                uint8_t* key,
                                size_t* key_len);
crypt_result_t crypt_generate_random_bytes(uint8_t* buffer, size_t len);
crypt_result_t crypt_secure_zero_memory(void* ptr, size_t len);
bool crypt_is_algorithm_supported(crypt_algorithm_t algorithm);
const char* crypt_get_algorithm_name(crypt_algorithm_t algorithm);
size_t crypt_get_key_size(crypt_algorithm_t algorithm);
size_t crypt_get_iv_size(crypt_algorithm_t algorithm);
size_t crypt_get_tag_size(crypt_algorithm_t algorithm);

// Utility functions
void crypt_log_event(const char* event, const char* details);
void crypt_log_error(const char* function, const char* error);
bool crypt_is_safe_key_length(size_t len);
bool crypt_constant_time_compare(const uint8_t* a, size_t a_len, const uint8_t* b, size_t b_len);

#endif // ENCRYPTION_H
