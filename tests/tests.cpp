#include <iostream>
#include <cassert>
#include "../src/auth.hpp"

// Unit Tests for Module 2: Access Control & Security

void testAuthentication() {
    std::cout << "Running Authentication Tests...\n";
    
    // Valid role checks
    assert(authenticateUser("admin_user", "admin") == true);
    assert(authenticateUser("doc_smith", "doctor") == true);
    assert(authenticateUser("reception_jane", "reception") == true);
    assert(authenticateUser("patient_doe", "patient") == true);
    
    // Invalid role checks
    assert(authenticateUser("hacker", "superadmin") == false);
    
    std::cout << "Authentication Tests Passed!\n";
}

void testAuthorization() {
    std::cout << "Running Authorization Tests...\n";
    
    // Admin bypasses specific role requirements (Kernel Mode privilege equivalent)
    assert(authorizeRole("admin", "doctor") == true);
    assert(authorizeRole("admin", "patient") == true);
    
    // Matching role checks
    assert(authorizeRole("doctor", "doctor") == true);
    assert(authorizeRole("reception", "reception") == true);
    
    // Mismatched role checks (Process isolation boundary enforcement)
    assert(authorizeRole("patient", "doctor") == false);
    assert(authorizeRole("reception", "admin") == false);
    
    std::cout << "Authorization Tests Passed!\n";
}

int main() {
    std::cout << "=== STARTING MEDISLOT MODULE 2 TESTS ===\n";
    testAuthentication();
    testAuthorization();
    std::cout << "=== ALL MODULE 2 TESTS PASSED SUCCESSFULLY ===\n";
    return 0;
}
