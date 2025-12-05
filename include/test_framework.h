#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdbool.h>
#include <stddef.h>

// Test result codes
typedef enum { TEST_PASS = 0, TEST_FAIL = 1, TEST_SKIP = 2, TEST_ERROR = 3 } test_result_t;

// Test context structure
typedef struct {
    char test_name[128];
    char module_name[64];
    int tests_run;
    int tests_passed;
    int tests_failed;
    int tests_skipped;
    double execution_time_ms;
    char failure_message[512];
} test_context_t;

// Test function pointer type
typedef test_result_t (*test_function_t)(test_context_t* ctx);

// Test suite structure
typedef struct {
    char suite_name[128];
    test_function_t* tests;
    size_t test_count;
    void (*setup)(void);
    void (*teardown)(void);
} test_suite_t;

// Test assertion macros
#define ASSERT_TRUE(ctx, condition)                                                                \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            snprintf((ctx)->failure_message,                                                       \
                     sizeof((ctx)->failure_message),                                               \
                     "ASSERT_TRUE failed: %s",                                                     \
                     #condition);                                                                  \
            return TEST_FAIL;                                                                      \
        }                                                                                          \
    } while (0)

#define ASSERT_FALSE(ctx, condition)                                                               \
    do {                                                                                           \
        if (condition) {                                                                           \
            snprintf((ctx)->failure_message,                                                       \
                     sizeof((ctx)->failure_message),                                               \
                     "ASSERT_FALSE failed: %s",                                                    \
                     #condition);                                                                  \
            return TEST_FAIL;                                                                      \
        }                                                                                          \
    } while (0)

#define ASSERT_EQ(ctx, expected, actual)                                                           \
    do {                                                                                           \
        if ((expected) != (actual)) {                                                              \
            snprintf((ctx)->failure_message,                                                       \
                     sizeof((ctx)->failure_message),                                               \
                     "ASSERT_EQ failed: expected %ld, got %ld",                                    \
                     (long)(expected),                                                             \
                     (long)(actual));                                                              \
            return TEST_FAIL;                                                                      \
        }                                                                                          \
    } while (0)

#define ASSERT_NE(ctx, expected, actual)                                                           \
    do {                                                                                           \
        if ((expected) == (actual)) {                                                              \
            snprintf((ctx)->failure_message,                                                       \
                     sizeof((ctx)->failure_message),                                               \
                     "ASSERT_NE failed: expected not equal to %ld",                                \
                     (long)(expected));                                                            \
            return TEST_FAIL;                                                                      \
        }                                                                                          \
    } while (0)

#define ASSERT_NULL(ctx, ptr)                                                                      \
    do {                                                                                           \
        if ((ptr) != NULL) {                                                                       \
            snprintf((ctx)->failure_message,                                                       \
                     sizeof((ctx)->failure_message),                                               \
                     "ASSERT_NULL failed: pointer is not NULL");                                   \
            return TEST_FAIL;                                                                      \
        }                                                                                          \
    } while (0)

#define ASSERT_NOT_NULL(ctx, ptr)                                                                  \
    do {                                                                                           \
        if ((ptr) == NULL) {                                                                       \
            snprintf((ctx)->failure_message,                                                       \
                     sizeof((ctx)->failure_message),                                               \
                     "ASSERT_NOT_NULL failed: pointer is NULL");                                   \
            return TEST_FAIL;                                                                      \
        }                                                                                          \
    } while (0)

#define ASSERT_GT(ctx, a, b)                                                                       \
    do {                                                                                           \
        if (!((a) > (b))) {                                                                        \
            snprintf((ctx)->failure_message,                                                       \
                     sizeof((ctx)->failure_message),                                               \
                     "ASSERT_GT failed: %ld is not greater than %ld",                              \
                     (long)(a),                                                                    \
                     (long)(b));                                                                   \
            return TEST_FAIL;                                                                      \
        }                                                                                          \
    } while (0)

#define ASSERT_LT(ctx, a, b)                                                                       \
    do {                                                                                           \
        if (!((a) < (b))) {                                                                        \
            snprintf((ctx)->failure_message,                                                       \
                     sizeof((ctx)->failure_message),                                               \
                     "ASSERT_LT failed: %ld is not less than %ld",                                 \
                     (long)(a),                                                                    \
                     (long)(b));                                                                   \
            return TEST_FAIL;                                                                      \
        }                                                                                          \
    } while (0)

// Security-specific assertions
#define ASSERT_SECURE_INPUT(ctx, input, max_length)                                                \
    do {                                                                                           \
        if (sec_validate_string((input), (max_length), false) != SEC_VALID) {                      \
            snprintf((ctx)->failure_message,                                                       \
                     sizeof((ctx)->failure_message),                                               \
                     "ASSERT_SECURE_INPUT failed: input is invalid or too long");                  \
            return TEST_FAIL;                                                                      \
        }                                                                                          \
    } while (0)

#define ASSERT_NO_BUFFER_OVERFLOW(ctx, buffer, size)                                               \
    do {                                                                                           \
        if (sec_validate_buffer((buffer), (size), (size)) != SEC_VALID) {                          \
            snprintf((ctx)->failure_message,                                                       \
                     sizeof((ctx)->failure_message),                                               \
                     "ASSERT_NO_BUFFER_OVERFLOW failed: buffer validation failed");                \
            return TEST_FAIL;                                                                      \
        }                                                                                          \
    } while (0)

// Test framework functions
void test_framework_init(void);
void test_framework_cleanup(void);
test_result_t run_test_suite(const test_suite_t* suite);
test_result_t run_single_test(test_context_t* ctx, test_function_t test_func);
void print_test_summary(const test_context_t* ctx);
void print_suite_summary(const test_suite_t* suite, const test_context_t* ctx);

// Timing functions
void start_timer(test_context_t* ctx);
void stop_timer(test_context_t* ctx);

// Security testing functions
test_result_t test_input_validation(test_context_t* ctx);
test_result_t test_memory_safety(test_context_t* ctx);
test_result_t test_sandbox_isolation(test_context_t* ctx);
test_result_t test_encryption(test_context_t* ctx);
test_result_t test_network_security(test_context_t* ctx);
test_result_t test_integration(test_context_t* ctx);

// Utility functions
void create_test_context(test_context_t* ctx, const char* test_name, const char* module_name);
void log_test_start(const char* test_name);
void log_test_result(const char* test_name, test_result_t result, double time_ms);
void log_test_failure(const char* test_name, const char* message);

// Test constants
#define MAX_TEST_SUITES 20
#define MAX_TESTS_PER_SUITE 50
#define TEST_TIMEOUT_MS 30000 // 30 seconds

#endif // TEST_FRAMEWORK_H
