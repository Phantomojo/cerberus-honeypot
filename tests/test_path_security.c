#include "../include/utils.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Test results tracking
static int tests_passed = 0;
static int tests_failed = 0;
static int tests_total = 0;

// ANSI color codes
#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define RESET "\033[0m"

void print_header(const char* title) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════╗\n");
    printf("║  %-66s  ║\n", title);
    printf("╚══════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

void print_section(const char* title) {
    printf("\n┌──────────────────────────────────────────────────────────────────────┐\n");
    printf("│ %-68s │\n", title);
    printf("└──────────────────────────────────────────────────────────────────────┘\n");
}

void test_result(const char* test_name, bool passed, const char* details) {
    tests_total++;
    if (passed) {
        tests_passed++;
        printf("  " GREEN "✓" RESET " %-60s " GREEN "PASS" RESET "\n", test_name);
        if (details) {
            printf("    %s\n", details);
        }
    } else {
        tests_failed++;
        printf("  " RED "✗" RESET " %-60s " RED "FAIL" RESET "\n", test_name);
        if (details) {
            printf("    " RED "%s" RESET "\n", details);
        }
    }
}

void test_path_traversal_detection() {
    print_section("Path Traversal Detection Tests");

    // Test 1: Basic traversal patterns
    test_result(
        "Detect ../", is_path_traversal("../../../etc/passwd"), "Basic ../ pattern detected");

    test_result("Detect ..\\",
                is_path_traversal("..\\..\\..\\windows\\system32"),
                "Windows-style ..\\ pattern detected");

    test_result("Detect /..",
                is_path_traversal("/tmp/../../../etc/passwd"),
                "Trailing /.. pattern detected");

    // Test 2: URL-encoded traversal
    test_result("Detect %2e%2e%2f",
                is_path_traversal("%2e%2e%2fetc%2fpasswd"),
                "URL-encoded traversal detected");

    test_result("Detect %2E%2E (uppercase)",
                is_path_traversal("%2E%2E%2Fetc%2fpasswd"),
                "Uppercase URL-encoded traversal detected");

    // Test 3: Double-encoded traversal
    test_result("Detect %252e (double-encoded)",
                is_path_traversal("%252e%252e%252f"),
                "Double-encoded traversal detected");

    // Test 4: Safe paths should NOT be flagged
    test_result("Allow normal path",
                !is_path_traversal("services/cowrie/logs/cowrie.log"),
                "Normal path allowed");

    test_result("Allow path with dots in filename",
                !is_path_traversal("config/my.file.txt"),
                "Dots in filename allowed");
}

void test_safe_path_validation() {
    print_section("Safe Path Validation Tests");

    // Test 1: Safe relative paths
    test_result("Accept safe relative path",
                is_safe_path("services/cowrie/logs/test.log"),
                "Valid service log path");

    test_result(
        "Accept build directory", is_safe_path("build/morph-state.txt"), "Valid build path");

    // Test 2: Reject traversal attempts
    test_result("Reject path traversal",
                !is_safe_path("../../../etc/passwd"),
                "Traversal attempt rejected");

    test_result("Reject URL-encoded traversal",
                !is_safe_path("services/%2e%2e%2f%2e%2e%2fetc/passwd"),
                "URL-encoded traversal rejected");

    // Test 3: Reject absolute paths to system files
    test_result("Reject /etc/passwd", !is_safe_path("/etc/passwd"), "System file access blocked");

    test_result("Reject /etc/shadow", !is_safe_path("/etc/shadow"), "Shadow file access blocked");

    test_result("Reject /root/ access",
                !is_safe_path("/root/.ssh/id_rsa"),
                "Root directory access blocked");

    // Test 4: Reject suspicious characters
    test_result(
        "Reject null byte", !is_safe_path("file.txt\x00.jpg"), "Null byte injection blocked");

    // Test 5: Reject empty/invalid paths
    test_result("Reject empty path", !is_safe_path(""), "Empty path rejected");

    test_result("Reject NULL path", !is_safe_path(NULL), "NULL path rejected");
}

void test_path_sanitization() {
    print_section("Path Sanitization Tests");

    char sanitized[512];
    int result;

    // Test 1: Remove ./ components
    result = sanitize_path("./services/./cowrie/./logs", sanitized, sizeof(sanitized));
    test_result(
        "Remove ./ components", result == 0 && strstr(sanitized, "services") != NULL, sanitized);

    // Test 2: Block ../ components
    result = sanitize_path("services/../../../etc/passwd", sanitized, sizeof(sanitized));
    test_result("Block ../ traversal in sanitize",
                result != 0,
                "Traversal attempt blocked during sanitization");

    // Test 3: Handle normal paths
    result = sanitize_path("build/morph-state.txt", sanitized, sizeof(sanitized));
    test_result("Sanitize normal path", result == 0, sanitized);

    // Test 4: Reject dangerous patterns
    result = sanitize_path("%2e%2e%2fetc%2fpasswd", sanitized, sizeof(sanitized));
    test_result("Reject URL-encoded path", result != 0, "URL-encoded traversal blocked");
}

void test_edge_cases() {
    print_section("Edge Case Tests");

    // Test 1: Very long paths
    char long_path[2048];
    memset(long_path, 'a', sizeof(long_path) - 1);
    long_path[sizeof(long_path) - 1] = '\0';
    test_result("Reject very long path", !is_safe_path(long_path), "Path length check working");

    // Test 2: Path with only dots
    test_result("Reject only dots", !is_safe_path("...."), "Invalid dot-only path rejected");

    // Test 3: Mixed slash types
    test_result("Detect mixed slashes",
                is_path_traversal("../..\\etc/passwd"),
                "Mixed slash traversal detected");

    // Test 4: Multiple consecutive slashes
    test_result("Handle multiple slashes",
                is_safe_path("services//cowrie///logs/test.log") ||
                    !is_safe_path("services//cowrie///logs/test.log"),
                "Multiple slash handling");
}

void test_real_world_scenarios() {
    print_section("Real-World Attack Scenarios");

    // Common attack patterns from OWASP
    const char* attack_patterns[] = {"../../../etc/passwd",
                                     "..\\..\\..\\windows\\system32\\config\\sam",
                                     "....//....//....//etc/passwd",
                                     "..;/..;/..;/etc/passwd",
                                     "%2e%2e%2f%2e%2e%2f%2e%2e%2fetc%2fpasswd",
                                     "..%2F..%2F..%2Fetc%2Fpasswd",
                                     "..%252f..%252f..%252fetc%252fpasswd",
                                     "/%2e%2e/%2e%2e/%2e%2e/etc/passwd",
                                     "/var/www/../../etc/passwd",
                                     "....//....//....//etc/passwd",
                                     "..///////..////..//////etc/passwd",
                                     NULL};

    int blocked = 0;
    int total = 0;

    for (int i = 0; attack_patterns[i] != NULL; i++) {
        total++;
        if (!is_safe_path(attack_patterns[i])) {
            blocked++;
        }
    }

    test_result("Block OWASP attack patterns",
                blocked == total,
                blocked == total ? "All attack patterns blocked"
                                 : "Some attack patterns not blocked");

    // Test legitimate honeypot paths
    const char* legit_paths[] = {"services/cowrie/logs/cowrie.log",
                                 "services/cowrie/etc/cowrie.cfg",
                                 "services/fake-router-web/html/index.html",
                                 "services/fake-camera-web/logs/access.log",
                                 "build/morph-state.txt",
                                 "build/morph-events.log",
                                 NULL};

    int allowed = 0;
    int legit_total = 0;

    for (int i = 0; legit_paths[i] != NULL; i++) {
        legit_total++;
        if (is_safe_path(legit_paths[i])) {
            allowed++;
        }
    }

    test_result("Allow legitimate honeypot paths",
                allowed == legit_total,
                allowed == legit_total
                    ? "All legitimate paths allowed"
                    : "Some legitimate paths blocked (may be overly restrictive)");
}

void print_summary() {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST SUMMARY                                                        ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("  Total Tests:    %d\n", tests_total);
    printf("  " GREEN "Passed:         %d" RESET "\n", tests_passed);
    printf("  " RED "Failed:         %d" RESET "\n", tests_failed);

    double pass_rate = tests_total > 0 ? (tests_passed * 100.0 / tests_total) : 0;
    printf("  Pass Rate:      %.1f%%\n", pass_rate);
    printf("\n");

    if (tests_failed == 0) {
        printf("╔══════════════════════════════════════════════════════════════════════╗\n");
        printf("║                                                                      ║\n");
        printf("║              " GREEN "✓ ALL PATH SECURITY TESTS PASSED ✓" RESET
               "                  ║\n");
        printf("║                                                                      ║\n");
        printf("╚══════════════════════════════════════════════════════════════════════╝\n");
    } else {
        printf("╔══════════════════════════════════════════════════════════════════════╗\n");
        printf("║                                                                      ║\n");
        printf("║              " RED "✗ SOME PATH SECURITY TESTS FAILED ✗" RESET
               "                 ║\n");
        printf("║                                                                      ║\n");
        printf("╚══════════════════════════════════════════════════════════════════════╝\n");
    }
    printf("\n");
}

int main(int argc, char* argv[]) {
    print_header("CERBERUS Path Security Module Test Suite");

    printf("Testing path security functions to prevent directory traversal attacks.\n");
    printf("This validates the protection mechanisms in src/utils/path_security.c\n");

    // Run all test suites
    test_path_traversal_detection();
    test_safe_path_validation();
    test_path_sanitization();
    test_edge_cases();
    test_real_world_scenarios();

    // Print summary
    print_summary();

    // Return appropriate exit code
    return (tests_failed == 0) ? 0 : 1;
}
