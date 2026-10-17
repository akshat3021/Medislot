#include "auth.hpp"
#include <iostream>
#include <algorithm>
#include <vector>



bool authenticateUser(const std::string& username, const std::string& role) {
   
    std::vector<std::string> validRoles = {"admin", "doctor", "reception", "patient"};
    auto it = std::find(validRoles.begin(), validRoles.end(), role);
    return (it != validRoles.end());
}

bool authorizeRole(const std::string& currentRole, const std::string& requiredRole) {
    
    if (currentRole == "admin") {
        return true;
    }
    

    return (currentRole == requiredRole);
}
