/**
 * @file auth_manager.cpp
 * @brief Authentication and Session Management Implementation
 */

#include "auth_manager.hpp"
#include "config_manager.hpp"

// ============================================================================
// CREDENTIALS
// ============================================================================

const char* AUTH_USERNAME = "admin";
const char* AUTH_PASSWORD = "1234";

// ============================================================================
// SESSION STORAGE
// ============================================================================

/** Active session tokens */
static String s_tokens[MAX_SESSIONS];

/** Last use timestamp for each token */
static unsigned long s_tokenLastUse[MAX_SESSIONS];

// ============================================================================
// IMPLEMENTATION
// ============================================================================

void authInit() {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        s_tokens[i] = "";
        s_tokenLastUse[i] = 0;
    }
    Serial.println("Auth system initialized");
}

String authGenerateToken() {
    String token = "";
    for (int i = 0; i < TOKEN_LENGTH; i++) {
        token += char('A' + random(0, 26));
    }
    return token;
}

bool authStoreToken(const String& token) {
    // First try to find an empty or expired slot
    authCleanupExpiredTokens();
    
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (s_tokens[i].length() == 0) {
            s_tokens[i] = token;
            s_tokenLastUse[i] = millis();
            return true;
        }
    }
    
    // No empty slots - find oldest token and replace it
    int oldestIndex = 0;
    unsigned long oldestTime = s_tokenLastUse[0];
    
    for (int i = 1; i < MAX_SESSIONS; i++) {
        if (s_tokenLastUse[i] < oldestTime) {
            oldestTime = s_tokenLastUse[i];
            oldestIndex = i;
        }
    }
    
    s_tokens[oldestIndex] = token;
    s_tokenLastUse[oldestIndex] = millis();
    return true;
}

bool authValidateToken(const String& token) {
    if (token.length() == 0) {
        return false;
    }
    
    unsigned long now = millis();
    
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (s_tokens[i] == token) {
            // Check if expired
            if (now - s_tokenLastUse[i] > TOKEN_TIMEOUT_MS) {
                s_tokens[i] = "";  // Clear expired token
                return false;
            }
            return true;
        }
    }
    
    return false;
}

void authRefreshToken(const String& token) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (s_tokens[i] == token) {
            s_tokenLastUse[i] = millis();
            return;
        }
    }
}

void authInvalidateToken(const String& token) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (s_tokens[i] == token) {
            s_tokens[i] = "";
            s_tokenLastUse[i] = 0;
            return;
        }
    }
}

bool authCheckCredentials(const String& username, const String& password) {
    return (username == AUTH_USERNAME && password == AUTH_PASSWORD);
}

bool authIsIPWhitelisted(const IPAddress& ip) {
    return configIsIPWhitelisted(ip);
}

void authCleanupExpiredTokens() {
    unsigned long now = millis();
    
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (s_tokens[i].length() > 0) {
            if (now - s_tokenLastUse[i] > TOKEN_TIMEOUT_MS) {
                s_tokens[i] = "";
                s_tokenLastUse[i] = 0;
            }
        }
    }
}

int authGetActiveSessionCount() {
    int count = 0;
    unsigned long now = millis();
    
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (s_tokens[i].length() > 0 && 
            (now - s_tokenLastUse[i]) <= TOKEN_TIMEOUT_MS) {
            count++;
        }
    }
    
    return count;
}
