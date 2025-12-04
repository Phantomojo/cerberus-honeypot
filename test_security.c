#include "include/security_utils.h"
#include "include/utils.h"
#include <stdio.h>
#include <string.h>

int main() {
    printf("Testing security utilities...\n");
    
    // Test IP validation
    const char* good_ip = "192.168.1.1";
    const char* bad_ip = "999.999.999.999";
    
    sec_result_t result = sec_validate_ip_address(good_ip);
    printf("Good IP validation: %s\n", result == SEC_VALID ? "PASS" : "FAIL");
    
    result = sec_validate_ip_address(bad_ip);
    printf("Bad IP validation: %s\n", result != SEC_VALID ? "PASS" : "FAIL");
    
    // Test filename validation
    const char* good_filename = "config.txt";
    const char* bad_filename = "../../../etc/passwd";
    
    result = sec_validate_filename(good_filename);
    printf("Good filename validation: %s\n", result == SEC_VALID ? "PASS" : "FAIL");
    
    result = sec_validate_filename(bad_filename);
    printf("Bad filename validation: %s\n", result != SEC_VALID ? "PASS" : "FAIL");
    
    // Test safe string copy
    char dest[50];
    result = sec_safe_strcpy(dest, "Hello, world!", sizeof(dest));
    printf("Safe string copy: %s\n", result == SEC_VALID ? "PASS" : "FAIL");
    printf("Copied string: %s\n", dest);
    
    // Test buffer overflow protection
    char small_dest[5];
    result = sec_safe_strcpy(small_dest, "This is too long", sizeof(small_dest));
    printf("Buffer overflow protection: %s\n", result == SEC_BUFFER_OVERFLOW ? "PASS" : "FAIL");
    
    printf("Security utilities test completed!\n");
    return 0;
}