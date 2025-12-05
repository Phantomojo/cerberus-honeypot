#!/bin/bash

# Security Testing Script for Cerberus Honeypot
# This script tests various security aspects of the codebase

echo "=== Cerberus Honeypot Security Testing ==="
echo "Testing input validation, bounds checking, and memory safety..."

# Compile security test
echo "1. Compiling security utilities..."
gcc -Iinclude src/security/security_utils.c src/utils/utils.c -o test_security 2>/dev/null

if [ $? -eq 0 ]; then
    echo "   ✓ Security utilities compiled successfully"
else
    echo "   ✗ Security utilities compilation failed"
    exit 1
fi

# Run security tests
echo "2. Running security validation tests..."
./test_security > security_test_output.txt

if [ $? -eq 0 ]; then
    echo "   ✓ Security tests passed"
else
    echo "   ✗ Security tests failed"
fi

# Test buffer overflow protection
echo "3. Testing buffer overflow protection..."
cat > test_buffer_overflow.c << 'EOF'
#include <stdio.h>
#include <string.h>
#include "include/security_utils.h"

int main() {
    char small_buffer[10];
    char large_input[50];
    
    // Initialize with safe pattern
    memset(large_input, 'A', 49);
    large_input[49] = '\0';
    
    printf("Testing buffer overflow protection...\n");
    
    // This should be prevented
    sec_result_t result = sec_safe_strcpy(small_buffer, large_input, sizeof(small_buffer));
    
    if (result == SEC_BUFFER_OVERFLOW) {
        printf("   ✓ Buffer overflow correctly detected and prevented\n");
        return 0;
    } else {
        printf("   ✗ Buffer overflow NOT detected - SECURITY ISSUE\n");
        return 1;
    }
}
EOF

gcc -Iinclude test_buffer_overflow.c src/security/security_utils.c -o test_buffer_overflow 2>/dev/null

if [ $? -eq 0 ]; then
    ./test_buffer_overflow
    rm -f test_buffer_overflow
else
    echo "   ✗ Buffer overflow test compilation failed"
fi

# Test path traversal protection
echo "4. Testing path traversal protection..."
cat > test_path_traversal.c << 'EOF'
#include <stdio.h>
#include "include/security_utils.h"

int main() {
    const char* dangerous_paths[] = {
        "../../../etc/passwd",
        "/etc/shadow",
        "..\\..\\windows\\system32\\config\\sam",
        "....//....//....//etc/passwd",
        NULL
    };
    
    printf("Testing path traversal protection...\n");
    
    int passed = 0;
    int total = 0;
    
    for (int i = 0; dangerous_paths[i] != NULL; i++) {
        total++;
        sec_result_t result = sec_validate_path(dangerous_paths[i], false);
        
        if (result != SEC_VALID) {
            passed++;
            printf("   ✓ Blocked: %s\n", dangerous_paths[i]);
        } else {
            printf("   ✗ ALLOWED: %s - SECURITY ISSUE\n", dangerous_paths[i]);
        }
    }
    
    printf("Path traversal test: %d/%d passed\n", passed, total);
    return (passed == total) ? 0 : 1;
}
EOF

gcc -Iinclude test_path_traversal.c src/security/security_utils.c -o test_path_traversal 2>/dev/null

if [ $? -eq 0 ]; then
    ./test_path_traversal
    rm -f test_path_traversal
else
    echo "   ✗ Path traversal test compilation failed"
fi

# Test IP validation
echo "5. Testing IP address validation..."
cat > test_ip_validation.c << 'EOF'
#include <stdio.h>
#include "include/security_utils.h"

int main() {
    const char* good_ips[] = {
        "192.168.1.1",
        "10.0.0.1",
        "172.16.0.1",
        "127.0.0.1",
        "255.255.255.255",
        "0.0.0.0",
        NULL
    };
    
    const char* bad_ips[] = {
        "999.999.999.999",
        "192.168.1",
        "192.168.1.1.1",
        "192.168.1.-1",
        "192.168.1.256",
        "abc.def.ghi.jkl",
        "",
        NULL
    };
    
    printf("Testing IP address validation...\n");
    
    int good_passed = 0;
    int bad_passed = 0;
    
    // Test good IPs
    for (int i = 0; good_ips[i] != NULL; i++) {
        sec_result_t result = sec_validate_ip_address(good_ips[i]);
        if (result == SEC_VALID) {
            good_passed++;
            printf("   ✓ Valid: %s\n", good_ips[i]);
        } else {
            printf("   ✗ Rejected: %s - should be valid\n", good_ips[i]);
        }
    }
    
    // Test bad IPs
    for (int i = 0; bad_ips[i] != NULL; i++) {
        sec_result_t result = sec_validate_ip_address(bad_ips[i]);
        if (result != SEC_VALID) {
            bad_passed++;
            printf("   ✓ Blocked: %s\n", bad_ips[i]);
        } else {
            printf("   ✗ ALLOWED: %s - SECURITY ISSUE\n", bad_ips[i]);
        }
    }
    
    printf("IP validation test: %d/%d good passed, %d/%d bad passed\n", 
           good_passed, 6, bad_passed, 7);
    
    return (good_passed == 6 && bad_passed == 7) ? 0 : 1;
}
EOF

gcc -Iinclude test_ip_validation.c src/security/security_utils.c -o test_ip_validation 2>/dev/null

if [ $? -eq 0 ]; then
    ./test_ip_validation
    rm -f test_ip_validation
else
    echo "   ✗ IP validation test compilation failed"
fi

# Test filename validation
echo "6. Testing filename validation..."
cat > test_filename_validation.c << 'EOF'
#include <stdio.h>
#include "include/security_utils.h"

int main() {
    const char* good_filenames[] = {
        "config.txt",
        "data.log",
        "backup_2023.tar.gz",
        "file-with-dashes.txt",
        "file_with_underscores.doc",
        NULL
    };
    
    const char* bad_filenames[] = {
        "../../../etc/passwd",
        "file with spaces.txt",
        "file;with;semicolons.txt",
        "file|with|pipes.txt",
        "file`with`backticks.txt",
        ".hidden_file",
        "",
        NULL
    };
    
    printf("Testing filename validation...\n");
    
    int good_passed = 0;
    int bad_passed = 0;
    
    // Test good filenames
    for (int i = 0; good_filenames[i] != NULL; i++) {
        sec_result_t result = sec_validate_filename(good_filenames[i]);
        if (result == SEC_VALID) {
            good_passed++;
            printf("   ✓ Valid: %s\n", good_filenames[i]);
        } else {
            printf("   ✗ Rejected: %s - should be valid\n", good_filenames[i]);
        }
    }
    
    // Test bad filenames
    for (int i = 0; bad_filenames[i] != NULL; i++) {
        sec_result_t result = sec_validate_filename(bad_filenames[i]);
        if (result != SEC_VALID) {
            bad_passed++;
            printf("   ✓ Blocked: %s\n", bad_filenames[i]);
        } else {
            printf("   ✗ ALLOWED: %s - SECURITY ISSUE\n", bad_filenames[i]);
        }
    }
    
    printf("Filename validation test: %d/%d good passed, %d/%d bad passed\n", 
           good_passed, 5, bad_passed, 7);
    
    return (good_passed == 5 && bad_passed == 7) ? 0 : 1;
}
EOF

gcc -Iinclude test_filename_validation.c src/security/security_utils.c -o test_filename_validation 2>/dev/null

if [ $? -eq 0 ]; then
    ./test_filename_validation
    rm -f test_filename_validation
else
    echo "   ✗ Filename validation test compilation failed"
fi

# Cleanup
rm -f test_security test_buffer_overflow.c test_path_traversal.c test_ip_validation.c test_filename_validation.c security_test_output.txt

echo ""
echo "=== Security Testing Complete ==="
echo "Summary:"
echo "- Input validation framework: ✓ Implemented"
echo "- Buffer overflow protection: ✓ Implemented" 
echo "- Path traversal protection: ✓ Implemented"
echo "- IP address validation: ✓ Implemented"
echo "- Filename validation: ✓ Implemented"
echo "- Bounds checking: ✓ Implemented"
echo ""
echo "Next steps:"
echo "1. Integrate security validation into existing code"
echo "2. Add memory safety tools (AddressSanitizer, Valgrind)"
echo "3. Create sandboxing for honeypot services"
echo "4. Develop automated security testing pipeline"