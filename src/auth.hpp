#ifndef AUTH_HPP
#define AUTH_HPP

#include <string>

// Module 2: Access Control & Security - Header Declarations

bool authenticateUser(const std::string& username, const std::string& role);
bool authorizeRole(const std::string& currentRole, const std::string& requiredRole);

#endif // AUTH_HPP
