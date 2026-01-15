#define _GNU_SOURCE
#include "include/sandbox.h"
#include "include/utils.h"

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void test_privilege_dropping() {
    printf("Testing privilege dropping...\n");

    sandbox_config_t config = get_web_sandbox_config();
    // Use 'nobody' as a standard unprivileged user if 'honeypot' doesn't exist
    strncpy(config.user, "nobody", sizeof(config.user));
    strncpy(config.group, "nogroup", sizeof(config.group));

    pid_t pid = fork();
    if (pid == 0) {
        // Child: Try to drop privileges
        if (drop_privileges(&config) == SANDBOX_SUCCESS) {
            uid_t uid = getuid();
            printf("Child UID after dropping: %d\n", uid);
            if (uid != 0) {
                printf("Privilege drop: PASS\n");
                exit(0);
            }
        }
        printf("Privilege drop: FAIL\n");
        exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("test_privilege_dropping: PASS\n");
        } else {
            printf("test_privilege_dropping: FAIL (Check if running as root)\n");
        }
    }
}

void test_namespace_isolation() {
    printf("Testing namespace isolation...\n");

    sandbox_config_t config = get_web_sandbox_config();
    config.network_isolated = true;

    pid_t pid = fork();
    if (pid == 0) {
        // Child: Try to isolate network
        if (setup_network_namespace(&config) == SANDBOX_SUCCESS) {
            printf("Network isolated: PASS\n");
            exit(0);
        }
        printf("Network isolated: FAIL\n");
        exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("test_namespace_isolation: PASS\n");
        } else {
            printf("test_namespace_isolation: FAIL (Requires CAP_SYS_ADMIN)\n");
        }
    }
}

int main() {
    printf("Cerberus Sandbox Isolation Unit Tests\n");
    printf("======================================\n");

    if (getuid() != 0) {
        printf(
            "WARNING: Most sandbox tests require root privileges to run (for unshare/setuid).\n");
    }

    test_privilege_dropping();
    test_namespace_isolation();

    printf("Tests completed.\n");
    return 0;
}
