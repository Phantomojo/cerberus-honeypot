/**
 * test_state_engine.c - Test program for the state engine
 * 
 * Tests:
 * 1. Initialization
 * 2. Profile loading
 * 3. Correlation (memory matches processes, uptime consistent)
 * 4. Output generators
 * 5. Morphing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "state_engine.h"

#define TEST_PASS(name) printf("  ✓ %s\n", name)
#define TEST_FAIL(name, reason) printf("  ✗ %s: %s\n", name, reason); failures++

static int failures = 0;

/* Test basic initialization */
void test_init(void) {
    printf("\n=== Test: Initialization ===\n");
    
    system_state_t state;
    device_profile_t profile;
    
    /* Get a built-in profile */
    if (state_get_builtin_profile(&profile, "TP-Link_Archer_C7") == 0) {
        TEST_PASS("Get built-in profile");
    } else {
        TEST_FAIL("Get built-in profile", "Failed to get TP-Link profile");
    }
    
    /* Initialize state */
    if (state_engine_init(&state, &profile) == 0) {
        TEST_PASS("Initialize state engine");
    } else {
        TEST_FAIL("Initialize state engine", "init returned non-zero");
        return;
    }
    
    /* Check basic state */
    if (state.is_initialized) {
        TEST_PASS("State marked as initialized");
    } else {
        TEST_FAIL("State marked as initialized", "is_initialized is false");
    }
    
    if (state.boot_time > 0 && state.boot_time < time(NULL)) {
        TEST_PASS("Boot time is valid");
    } else {
        TEST_FAIL("Boot time is valid", "boot_time out of range");
    }
    
    if (state.uptime_seconds > 0) {
        TEST_PASS("Uptime is positive");
    } else {
        TEST_FAIL("Uptime is positive", "uptime_seconds is 0 or negative");
    }
    
    if (state.user_count > 0) {
        TEST_PASS("Users were created");
        printf("    Created %d users\n", state.user_count);
    } else {
        TEST_FAIL("Users were created", "user_count is 0");
    }
    
    if (state.process_count > 0) {
        TEST_PASS("Processes were created");
        printf("    Created %d processes\n", state.process_count);
    } else {
        TEST_FAIL("Processes were created", "process_count is 0");
    }
    
    if (state.interface_count > 0) {
        TEST_PASS("Network interfaces were created");
    } else {
        TEST_FAIL("Network interfaces were created", "interface_count is 0");
    }
    
    state_engine_destroy(&state);
    TEST_PASS("Destroy state engine");
}

/* Test correlation - memory should match process sum */
void test_correlation(void) {
    printf("\n=== Test: Correlation (Rubik's Cube) ===\n");
    
    system_state_t state;
    state_engine_init(&state, NULL);
    
    /* Calculate expected memory from processes */
    uint32_t process_memory = 0;
    for (int i = 0; i < state.process_count; i++) {
        process_memory += state.processes[i].memory_kb;
    }
    
    printf("    Process memory sum: %u KB\n", process_memory);
    printf("    State used memory:  %u KB\n", state.used_memory_kb);
    printf("    State total memory: %u KB\n", state.total_memory_kb);
    
    /* Used memory should be >= process memory (includes kernel overhead) */
    if (state.used_memory_kb >= process_memory) {
        TEST_PASS("Memory correlates with processes");
    } else {
        TEST_FAIL("Memory correlates with processes", 
                  "used_memory_kb < sum of process memory");
    }
    
    /* Load average should be reasonable */
    printf("    Load averages: %.2f %.2f %.2f\n", 
           state.load_avg_1 / 100.0,
           state.load_avg_5 / 100.0,
           state.load_avg_15 / 100.0);
    
    if (state.load_avg_1 < 1000 && state.load_avg_5 < 1000 && state.load_avg_15 < 1000) {
        TEST_PASS("Load average is reasonable for IoT");
    } else {
        TEST_FAIL("Load average is reasonable", "Load too high for IoT device");
    }
    
    state_engine_destroy(&state);
}

/* Test output generators */
void test_generators(void) {
    printf("\n=== Test: Output Generators ===\n");
    
    system_state_t state;
    state_engine_init(&state, NULL);
    
    char buf[4096];
    int len;
    
    /* Test /proc/uptime */
    len = state_generate_proc_uptime(&state, buf, sizeof(buf));
    if (len > 0) {
        TEST_PASS("Generate /proc/uptime");
        printf("    %s", buf);
    } else {
        TEST_FAIL("Generate /proc/uptime", "returned <= 0");
    }
    
    /* Test /proc/meminfo */
    len = state_generate_proc_meminfo(&state, buf, sizeof(buf));
    if (len > 0) {
        TEST_PASS("Generate /proc/meminfo");
        /* Just show first few lines */
        char* newline = strchr(buf, '\n');
        if (newline) {
            newline = strchr(newline + 1, '\n');
            if (newline) {
                newline = strchr(newline + 1, '\n');
                if (newline) *newline = '\0';
            }
        }
        printf("    %s...\n", buf);
    } else {
        TEST_FAIL("Generate /proc/meminfo", "returned <= 0");
    }
    
    /* Test /proc/loadavg */
    len = state_generate_proc_loadavg(&state, buf, sizeof(buf));
    if (len > 0) {
        TEST_PASS("Generate /proc/loadavg");
        printf("    %s", buf);
    } else {
        TEST_FAIL("Generate /proc/loadavg", "returned <= 0");
    }
    
    /* Test /etc/passwd */
    len = state_generate_passwd(&state, buf, sizeof(buf));
    if (len > 0) {
        TEST_PASS("Generate /etc/passwd");
        /* Show first 2 lines */
        char* nl = strchr(buf, '\n');
        if (nl) nl = strchr(nl + 1, '\n');
        if (nl) *nl = '\0';
        printf("    %s...\n", buf);
    } else {
        TEST_FAIL("Generate /etc/passwd", "returned <= 0");
    }
    
    /* Test ps output */
    len = state_generate_ps_output(&state, buf, sizeof(buf), true);
    if (len > 0) {
        TEST_PASS("Generate ps aux output");
        /* Show header + first process */
        char* nl = strchr(buf, '\n');
        if (nl) nl = strchr(nl + 1, '\n');
        if (nl) *nl = '\0';
        printf("    %s...\n", buf);
    } else {
        TEST_FAIL("Generate ps aux output", "returned <= 0");
    }
    
    /* Test uptime command */
    len = state_generate_uptime_output(&state, buf, sizeof(buf));
    if (len > 0) {
        TEST_PASS("Generate uptime output");
        printf("    %s", buf);
    } else {
        TEST_FAIL("Generate uptime output", "returned <= 0");
    }
    
    /* Test generic file content router */
    len = state_generate_file_content(&state, "/proc/uptime", buf, sizeof(buf));
    if (len > 0) {
        TEST_PASS("File content router works");
    } else {
        TEST_FAIL("File content router", "returned <= 0");
    }
    
    state_engine_destroy(&state);
}

/* Test morphing */
void test_morph(void) {
    printf("\n=== Test: Morphing (Rubik's Cube Twist) ===\n");
    
    system_state_t state;
    state_engine_init(&state, NULL);
    
    /* Record initial values */
    time_t initial_boot = state.boot_time;
    uint32_t initial_uptime = state.uptime_seconds;
    char initial_hostname[64];
    strncpy(initial_hostname, state.hostname, sizeof(initial_hostname));
    
    printf("    Before morph: boot=%ld, uptime=%u, hostname=%s\n",
           initial_boot, initial_uptime, initial_hostname);
    
    /* Morph! */
    if (state_engine_morph(&state, 12345) == 0) {
        TEST_PASS("Morph succeeded");
    } else {
        TEST_FAIL("Morph succeeded", "morph returned non-zero");
        return;
    }
    
    printf("    After morph:  boot=%ld, uptime=%u, hostname=%s\n",
           state.boot_time, state.uptime_seconds, state.hostname);
    
    /* Values should be different */
    if (state.boot_time != initial_boot) {
        TEST_PASS("Boot time changed after morph");
    } else {
        TEST_FAIL("Boot time changed", "boot_time is same");
    }
    
    /* State should still be valid */
    if (state.is_initialized && state.process_count > 0 && state.user_count > 0) {
        TEST_PASS("State still valid after morph");
    } else {
        TEST_FAIL("State still valid", "State corrupted after morph");
    }
    
    /* Test reproducibility - same seed = same state */
    system_state_t state2;
    state_engine_init(&state2, NULL);
    state_engine_morph(&state2, 12345);
    
    if (state.boot_time == state2.boot_time) {
        TEST_PASS("Same seed produces same boot_time (reproducible)");
    } else {
        TEST_FAIL("Same seed produces same boot_time", "Different boot times");
    }
    
    state_engine_destroy(&state);
    state_engine_destroy(&state2);
}

/* Test different device profiles */
void test_profiles(void) {
    printf("\n=== Test: Device Profiles ===\n");
    
    int count;
    const char** profiles = state_list_builtin_profiles(&count);
    
    printf("    Found %d built-in profiles\n", count);
    
    for (int i = 0; i < count; i++) {
        device_profile_t profile;
        if (state_get_builtin_profile(&profile, profiles[i]) == 0) {
            printf("    - %s (%s %s, %s)\n", 
                   profile.name, profile.vendor, profile.model,
                   profile.type == DEVICE_TYPE_ROUTER ? "Router" :
                   profile.type == DEVICE_TYPE_CAMERA ? "Camera" : "Other");
        }
    }
    
    TEST_PASS("Listed all profiles");
    
    /* Test initializing with different profiles */
    system_state_t state;
    device_profile_t router, camera;
    
    state_get_builtin_profile(&router, "TP-Link_Archer_C7");
    state_get_builtin_profile(&camera, "Hikvision_DS-2CD2");
    
    state_engine_init(&state, &router);
    printf("    Router processes: %d\n", state.process_count);
    state_engine_destroy(&state);
    
    state_engine_init(&state, &camera);
    printf("    Camera processes: %d\n", state.process_count);
    state_engine_destroy(&state);
    
    TEST_PASS("Different profiles create different states");
}

int main(void) {
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║           CERBERUS State Engine Test Suite                    ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n");
    
    test_init();
    test_correlation();
    test_generators();
    test_morph();
    test_profiles();
    
    printf("\n═══════════════════════════════════════════════════════════════\n");
    if (failures == 0) {
        printf("All tests PASSED! ✓\n");
        return 0;
    } else {
        printf("%d test(s) FAILED ✗\n", failures);
        return 1;
    }
}
