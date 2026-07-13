/**
 * @file auth_manager.hpp
 * @brief Authentication and Session Management Module
 * 
 * Handles:
 * - Session token generation and validation
 * - Token timeout/expiration
 * - IP whitelist checking (uses config_manager)
 * - Login credential verification
 */

#ifndef AUTH_MANAGER_HPP
#define AUTH_MANAGER_HPP

#include <Arduino.h>
#include <IPAddress.h>

// ============================================================================
// CONSTANTS
// ============================================================================

/** Maximum concurrent sessions */
constexpr int MAX_SESSIONS = 5;

/** Token timeout in milliseconds (5 minutes) */
constexpr unsigned long TOKEN_TIMEOUT_MS = 300000;

/** Token length in characters */
constexpr int TOKEN_LENGTH = 16;

// ============================================================================
// CREDENTIALS (configurable)
// ============================================================================

/** Default username for login */
extern const char* AUTH_USERNAME;

/** Default password for login */
extern const char* AUTH_PASSWORD;

// ============================================================================
// AUTHENTICATION FUNCTIONS
// ============================================================================

/**
 * @brief Initialize authentication system
 * 
 * Clears all session tokens. Call in setup().
 */
void authInit();

/**
 * @brief Generate a new session token
 * @return Randomly generated 16-character token
 */
String authGenerateToken();

/**
 * @brief Store a token in the session list
 * @param token Token to store
 * @return true if stored successfully, false if no slots available
 */
bool authStoreToken(const String& token);

/**
 * @brief Validate a token
 * @param token Token to validate
 * @return true if token is valid and not expired
 */
bool authValidateToken(const String& token);

/**
 * @brief Refresh token's last-use timestamp
 * @param token Token to refresh
 * 
 * Call this when a valid request is made to prevent timeout.
 */
void authRefreshToken(const String& token);

/**
 * @brief Invalidate a specific token (logout)
 * @param token Token to invalidate
 */
void authInvalidateToken(const String& token);

/**
 * @brief Check login credentials
 * @param username Provided username
 * @param password Provided password
 * @return true if credentials match
 */
bool authCheckCredentials(const String& username, const String& password);

/**
 * @brief Check if an IP address is whitelisted
 * @param ip IP address to check
 * @return true if IP is in whitelist
 * 
 * @note This is a convenience wrapper around configIsIPWhitelisted()
 */
bool authIsIPWhitelisted(const IPAddress& ip);

/**
 * @brief Clean up expired tokens
 * 
 * Removes tokens that haven't been used within TOKEN_TIMEOUT_MS.
 * Called automatically during token validation.
 */
void authCleanupExpiredTokens();

/**
 * @brief Get number of active sessions
 * @return Count of valid, non-expired tokens
 */
int authGetActiveSessionCount();

#endif // AUTH_MANAGER_HPP
