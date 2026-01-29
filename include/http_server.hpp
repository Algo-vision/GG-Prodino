/**
 * @file http_server.hpp
 * @brief HTTP Server Module
 * 
 * Handles HTTP API requests including:
 * - Login/authentication
 * - Device status queries
 * - Relay control
 * - LED control
 * - IP configuration
 * - Serial number management
 */

#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <Arduino.h>
#include <Ethernet.h>
#include <ArduinoJson.h>

// ============================================================================
// HTTP SERVER FUNCTIONS
// ============================================================================

/**
 * @brief Initialize HTTP server
 * @param server Reference to the EthernetServer instance
 * 
 * Call in setup() after Ethernet.begin().
 */
void httpServerInit(EthernetServer& server);

/**
 * @brief Process HTTP requests
 * 
 * Call in loop() to handle incoming HTTP requests.
 * This is the main HTTP request handling function.
 */
void httpServerLoop();

/**
 * @brief Read complete HTTP request
 * @param client Client connection
 * @return Complete HTTP request string
 */
String httpReadRequest(EthernetClient& client);

/**
 * @brief Extract body from HTTP request
 * @param request Complete HTTP request
 * @return Request body (after headers)
 */
String httpExtractBody(const String& request);

/**
 * @brief Send HTTP response
 * @param client Client connection
 * @param statusCode HTTP status code
 * @param content Response body
 * @param contentType Content type (default: application/json)
 */
void httpSendResponse(EthernetClient& client, int statusCode, 
                      const String& content, const String& contentType = "application/json");

/**
 * @brief Handle login request
 * @param doc Request JSON document
 * @return Response JSON document
 */
JsonDocument httpHandleLogin(JsonDocument& doc);

/**
 * @brief Check if user is currently connected
 * @return true if user connected recently
 */
bool httpIsUserConnected();

/**
 * @brief Get last user connection time
 * @return Last connection timestamp (millis)
 */
unsigned long httpGetLastUserConnectedTime();

#endif // HTTP_SERVER_HPP
