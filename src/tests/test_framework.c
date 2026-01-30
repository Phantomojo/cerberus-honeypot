#include "include/test_framework.h"

#include "include/sandbox.h"
#include "include/security_utils.h"
#include "include/utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global test context
static test_context_t g_test_ctx;

// Initialize test framework
void test_framework_init(void) {
    memset(&g_test_ctx, 0, sizeof(test_context_t));
    printf("=== Cerberus Honeypot Test Framework ===\n");
}

// Cleanup test framework
void test_framework_cleanup(void) {
    printf("\n=== Test Framework Cleanup ===\n");
}

// Create test context
void create_test_context(test_context_t* ctx, const char* test_name, const char* module_name) {
    if (!ctx)
        return;

    strncpy(ctx->test_name, test_name, sizeof(ctx->test_name) - 1);
    strncpy(ctx->module_name, module_name, sizeof(ctx->module_name) - 1);
    ctx->tests_run = 0;
    ctx->tests_passed = 0;
    ctx->tests_failed = 0;
    ctx->tests_skipped = 0;
    ctx->execution_time_ms = 0.0;
    ctx->failure_message[0] = '\0';
}

// Start timer
void start_timer(test_context_t* ctx) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ctx->execution_time_ms = -(tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0);
}

// Stop timer
void stop_timer(test_context_t* ctx) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ctx->execution_time_ms += (tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0);
}

// Log test start
void log_test_start(const char* test_name) {
    printf("  Running: %s\n", test_name);
}

// Log test result
void log_test_result(const char* test_name, test_result_t result, double time_ms) {
    const char* result_str;
    switch (result) {
        case TEST_PASS:
            result_str = "PASS";
            break;
        case TEST_FAIL:
            result_str = "FAIL";
            break;
        case TEST_SKIP:
            result_str = "SKIP";
            break;
        default:
            result_str = "ERROR";
            break;
    }

    printf("    Result: %s (%.2fms)\n", result_str, time_ms);
}

// Log test failure
void log_test_failure(const char* test_name, const char* message) {
    printf("    FAILURE: %s\n", message);
}

// Print test summary
void print_test_summary(const test_context_t* ctx) {
    printf("\n  Test Summary for %s:\n", ctx->test_name);
    printf("    Tests run: %d\n", ctx->tests_run);
    printf("    Tests passed: %d\n", ctx->tests_passed);
    printf("    Tests failed: %d\n", ctx->tests_failed);
    printf("    Tests skipped: %d\n", ctx->tests_skipped);
    printf("    Success rate: %.1f%%\n",
           ctx->tests_run > 0 ? (100.0 * ctx->tests_passed / ctx->tests_run) : 0.0);
    printf("    Execution time: %.2fms\n", ctx->execution_time_ms);

    if (ctx->tests_failed > 0) {
        printf("    FAILURE DETAILS: %s\n", ctx->failure_message);
    }
}

// Print suite summary
void print_suite_summary(const test_suite_t* suite, const test_context_t* ctx) {
    printf("\n=== Suite Summary: %s ===\n", suite->suite_name);
    printf("Total tests: %zu\n", suite->test_count);
    printf("Total passed: %d\n", ctx->tests_passed);
    printf("Total failed: %d\n", ctx->tests_failed);
    printf("Total skipped: %d\n", ctx->tests_skipped);
    printf("Suite success rate: %.1f%%\n",
           ctx->tests_run > 0 ? (100.0 * ctx->tests_passed / ctx->tests_run) : 0.0);
    printf("Total execution time: %.2fms\n", ctx->execution_time_ms);
}

// Run single test
test_result_t run_single_test(test_context_t* ctx, test_function_t test_func) {
    if (!ctx || !test_func) {
        return TEST_ERROR;
    }

    ctx->tests_run++;
    start_timer(ctx);

    test_result_t result = test_func(ctx);

    stop_timer(ctx);

    if (result == TEST_PASS) {
        ctx->tests_passed++;
    } else if (result == TEST_FAIL) {
        ctx->tests_failed++;
    } else {
        ctx->tests_skipped++;
    }

    log_test_result(ctx->test_name, result, ctx->execution_time_ms);

    return result;
}

// Run test suite
test_result_t run_test_suite(const test_suite_t* suite) {
    if (!suite || !suite->tests) {
        printf("ERROR: Invalid test suite\n");
        return TEST_ERROR;
    }

    printf("\n=== Running Test Suite: %s ===\n", suite->suite_name);

    test_context_t suite_ctx;
    create_test_context(&suite_ctx, suite->suite_name, "framework");

    if (suite->setup) {
        suite->setup();
    }

    for (size_t i = 0; i < suite->test_count; i++) {
        if (!suite->tests[i]) {
            printf("WARNING: Test function %zu is NULL\n", i);
            continue;
        }

        create_test_context(&g_test_ctx, suite->suite_name, suite->suite_name);
        test_result_t result = run_single_test(&g_test_ctx, suite->tests[i]);

        if (result == TEST_FAIL) {
            suite_ctx.tests_failed++;
        } else if (result == TEST_PASS) {
            suite_ctx.tests_passed++;
        } else {
            suite_ctx.tests_skipped++;
        }

        suite_ctx.tests_run++;
    }

    if (suite->teardown) {
        suite->teardown();
    }

    print_suite_summary(suite, &suite_ctx);

    return (suite_ctx.tests_failed == 0) ? TEST_PASS : TEST_FAIL;
}

// Security test implementations
test_result_t test_input_validation(test_context_t* ctx) {
    log_test_start("Input Validation Tests");

    // Test 1: IP address validation
    const char* good_ip = "192.168.1.1";
    const char* bad_ip = "999.999.999.999";

    if (sec_validate_ip_address(good_ip) != SEC_VALID) {
        snprintf(ctx->failure_message,
                 sizeof(ctx->failure_message),
                 "Good IP validation failed: %s",
                 good_ip);
        return TEST_FAIL;
    }

    if (sec_validate_ip_address(bad_ip) == SEC_VALID) {
        snprintf(ctx->failure_message,
                 sizeof(ctx->failure_message),
                 "Bad IP validation passed: %s",
                 bad_ip);
        return TEST_FAIL;
    }

    // Test 2: Filename validation
    const char* good_filename = "config.txt";
    const char* bad_filename = "../../../etc/passwd";

    if (sec_validate_filename(good_filename) != SEC_VALID) {
        snprintf(ctx->failure_message,
                 sizeof(ctx->failure_message),
                 "Good filename validation failed: %s",
                 good_filename);
        return TEST_FAIL;
    }

    if (sec_validate_filename(bad_filename) == SEC_VALID) {
        snprintf(ctx->failure_message,
                 sizeof(ctx->failure_message),
                 "Bad filename validation passed: %s",
                 bad_filename);
        return TEST_FAIL;
    }

    // Test 3: String length validation
    char long_string[2000];
    memset(long_string, 'A', 1999);
    long_string[1999] = '\0';

    if (sec_validate_string(long_string, 100, false) != SEC_STRING_TOO_LONG) {
        snprintf(ctx->failure_message,
                 sizeof(ctx->failure_message),
                 "Long string validation should have failed");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

test_result_t test_memory_safety(test_context_t* ctx) {
    log_test_start("Memory Safety Tests");

    // Test 1: Safe string copy
    char dest[10];
    const char* src = "Hello, world!";

    if (sec_safe_strcpy(dest, src, sizeof(dest)) != SEC_VALID) {
        snprintf(ctx->failure_message,
                 sizeof(ctx->failure_message),
                 "Safe string copy failed unexpectedly");
        return TEST_FAIL;
    }

    // Test 2: Buffer overflow detection
    char large_src[50];
    memset(large_src, 'A', 49);
    large_src[49] = '\0';

    if (sec_safe_strcpy(dest, large_src, sizeof(dest)) == SEC_VALID) {
        snprintf(
            ctx->failure_message, sizeof(ctx->failure_message), "Buffer overflow NOT detected");
        return TEST_FAIL;
    }

    // Test 3: Safe arithmetic
    if (sec_is_safe_addition(SIZE_MAX / 2, SIZE_MAX / 2)) {
        snprintf(
            ctx->failure_message, sizeof(ctx->failure_message), "Unsafe addition NOT detected");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

test_result_t test_sandbox_isolation(test_context_t* ctx) {
    log_test_start("Sandbox Isolation Tests");

    // Test 1: Sandbox configuration validation
    sandbox_config_t config = get_cowrie_sandbox_config();

    if (!validate_sandbox_config(&config)) {
        snprintf(ctx->failure_message,
                 sizeof(ctx->failure_message),
                 "Cowrie sandbox config validation failed");
        return TEST_FAIL;
    }

    // Test 2: User validation
    if (!is_valid_sandbox_user("cowrie")) {
        snprintf(ctx->failure_message,
                 sizeof(ctx->failure_message),
                 "Valid sandbox user validation failed");
        return TEST_FAIL;
    }

    if (is_valid_sandbox_user("root")) {
        snprintf(ctx->failure_message,
                 sizeof(ctx->failure_message),
                 "Invalid sandbox user validation passed");
        return TEST_FAIL;
    }

    // Test 3: Chroot path validation
    if (!is_safe_chroot_path("/var/lib/cerberus/cowrie-chroot")) {
        snprintf(ctx->failure_message,
                 sizeof(ctx->failure_message),
                 "Safe chroot path validation failed");
        return TEST_FAIL;
    }

    if (is_safe_chroot_path("../../../etc/passwd")) {
        snprintf(ctx->failure_message,
                 sizeof(ctx->failure_message),
                 "Unsafe chroot path validation passed");
        return TEST_FAIL;
    }

    return TEST_PASS;
}

test_result_t test_encryption(test_context_t* ctx) {
    log_test_start("Encryption Tests");

    // This is a placeholder for now
    // TODO: Implement actual encryption tests

    log_test_failure(ctx->test_name, "Encryption tests not yet implemented");
    return TEST_SKIP;
}

test_result_t test_network_security(test_context_t* ctx) {
    log_test_start("Network Security Tests");

    // This would test network security functions
    // TODO: Implement actual network security tests

    log_test_failure(ctx->test_name, "Network security tests not yet implemented");
    return TEST_SKIP;
}

test_result_t test_integration(test_context_t* ctx) {
    log_test_start("Integration Tests");

    // This would test end-to-end security workflow
    // TODO: Implement actual integration tests

    log_test_failure(ctx->test_name, "Integration tests not yet implemented");
    return TEST_SKIP;
}
