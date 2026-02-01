/**
 * @file http_server.cpp
 * @brief HTTP Server Module Implementation
 */

#include "http_server.hpp"
#include "config_manager.hpp"
#include "auth_manager.hpp"
#include "status_manager.hpp"
#include "led_controller.hpp"
#include "relay_controller.hpp"
#include "KMPProDinoMKRZero.h"
#include "gg_hal.hpp"
#include <Arduino_DebugUtils.h>  // For NVIC_SystemReset()

// ============================================================================
// CONFIGURATION
// ============================================================================

/** Auto-reset duration for relays 0 and 1 (5 seconds) */
constexpr unsigned long RELAY_AUTO_RESET_DURATION = 5000;

// ============================================================================
// INTERNAL STATE
// ============================================================================

/** Reference to the Ethernet server */
static EthernetServer* s_server = nullptr;

/** Last user connected timestamp */
static unsigned long s_lastUserConnectedTime = 0;

/** User connected flag */
static bool s_userConnected = false;

// Active IP Tracking - for UDP unicast optimization
// Only IPs that have recently connected via HTTP will receive UDP broadcasts
constexpr int MAX_ACTIVE_IPS = 10;
constexpr unsigned long ACTIVE_IP_TIMEOUT_MS = 30000; // 30 seconds

struct ActiveIP {
    IPAddress ip;
    unsigned long lastSeen;
    bool valid;
};

static ActiveIP s_activeIPs[MAX_ACTIVE_IPS];
static int s_activeIPCount = 0;

// External references
extern bool technician_mode;
extern GG_HAL _gg_hal;
extern LED_STATES manual_led_state;
extern bool manual_led_control_active;

// ============================================================================
// IMPLEMENTATION
// ============================================================================

void httpServerInit(EthernetServer& server) {
    s_server = &server;
    s_lastUserConnectedTime = 0;
    s_userConnected = false;
    
    // Initialize active IP tracking
    for (int i = 0; i < MAX_ACTIVE_IPS; i++) {
        s_activeIPs[i].valid = false;
        s_activeIPs[i].lastSeen = 0;
    }
    s_activeIPCount = 0;
    
    Serial.println("HTTP server initialized");
}

// Track an IP as active (called when HTTP request received)
static void trackActiveIP(const IPAddress& ip) {
    unsigned long now = millis();
    
    // First, check if this IP is already tracked
    for (int i = 0; i < MAX_ACTIVE_IPS; i++) {
        if (s_activeIPs[i].valid && s_activeIPs[i].ip == ip) {
            s_activeIPs[i].lastSeen = now;
            return;
        }
    }
    
    // Not found, add it to an empty slot or replace oldest
    int emptySlot = -1;
    int oldestSlot = 0;
    unsigned long oldestTime = now;
    
    for (int i = 0; i < MAX_ACTIVE_IPS; i++) {
        if (!s_activeIPs[i].valid) {
            emptySlot = i;
            break;
        }
        if (s_activeIPs[i].lastSeen < oldestTime) {
            oldestTime = s_activeIPs[i].lastSeen;
            oldestSlot = i;
        }
    }
    
    int slot = (emptySlot >= 0) ? emptySlot : oldestSlot;
    s_activeIPs[slot].ip = ip;
    s_activeIPs[slot].lastSeen = now;
    s_activeIPs[slot].valid = true;
    
    Serial.print("[Active IP] Added: ");
    Serial.println(ip);
}

// Get list of active IPs (not timed out)
int httpGetActiveIPs(IPAddress* outIPs, int maxCount) {
    unsigned long now = millis();
    int count = 0;
    
    for (int i = 0; i < MAX_ACTIVE_IPS && count < maxCount; i++) {
        if (s_activeIPs[i].valid) {
            // Check if timed out
            if (now - s_activeIPs[i].lastSeen > ACTIVE_IP_TIMEOUT_MS) {
                s_activeIPs[i].valid = false;  // Mark as expired
                continue;
            }
            outIPs[count++] = s_activeIPs[i].ip;
        }
    }
    
    return count;
}

String httpReadRequest(EthernetClient& client) {
    String req = "";
    unsigned long startTime = millis();
    int contentLength = -1;
    bool headersComplete = false;
    int bodyBytesRead = 0;
    
    // Read with 50ms idle timeout (stop if no data for 50ms)
    unsigned long lastDataTime = millis();
    while (client.connected() && (millis() - startTime < 500) && (millis() - lastDataTime < 50)) {
        if (client.available()) {
            char c = client.read();
            req += c;
            lastDataTime = millis();
            
            // Check for end of headers
            if (!headersComplete && req.endsWith("\r\n\r\n")) {
                headersComplete = true;
                
                // Parse Content-Length from headers
                int clIdx = req.indexOf("Content-Length:");
                if (clIdx != -1) {
                    int clEnd = req.indexOf("\r\n", clIdx);
                    String clValue = req.substring(clIdx + 15, clEnd);
                    clValue.trim();
                    contentLength = clValue.toInt();
                }
                bodyBytesRead = 0;
            }
            
            // Count body bytes after headers
            if (headersComplete) {
                bodyBytesRead++;
                // Stop when we've read the full body
                if (contentLength >= 0 && bodyBytesRead >= contentLength) {
                    break;  // Got all data, exit early!
                }
            }
        }
    }
    return req;
}

String httpExtractBody(const String& req) {
    int idx = req.indexOf("\r\n\r\n");
    if (idx != -1) {
        return req.substring(idx + 4);
    }
    return "";
}

void httpSendResponse(EthernetClient& client, int statusCode, 
                      const String& content, const String& contentType) {
    String statusText;
    switch (statusCode) {
        case 200: statusText = "OK"; break;
        case 401: statusText = "Unauthorized"; break;
        case 403: statusText = "Forbidden"; break;
        case 404: statusText = "Not Found"; break;
        case 500: statusText = "Internal Server Error"; break;
        default:  statusText = "Unknown"; break;
    }
    
    client.print("HTTP/1.1 ");
    client.print(statusCode);
    client.print(" ");
    client.println(statusText);
    client.print("Content-Type: ");
    client.println(contentType);
    client.println("Connection: close");
    client.print("Content-Length: ");
    client.println(content.length());
    client.println();
    client.print(content);
}

JsonDocument httpHandleLogin(JsonDocument& doc) {
    String user = doc["user"];
    String pass = doc["pass"];
    
    Serial.println();
    Serial.println("httpHandleLogin: Received User: " + user + ", Pass: " + pass);
    Serial.println("httpHandleLogin: Expected User: " + String(AUTH_USERNAME) + 
                   ", Pass: " + String(AUTH_PASSWORD));
    
    JsonDocument resp;
    resp["type"] = "login_result";
    
    if (!authCheckCredentials(user, pass)) {
        resp["success"] = false;
        if (user != AUTH_USERNAME) {
            resp["message"] = "Invalid username";
        } else {
            resp["message"] = "Invalid password";
        }
    } else {
        String newToken = authGenerateToken();
        authStoreToken(newToken);
        
        resp["success"] = true;
        resp["token"] = newToken;
        Serial.println("Login successful. Token assigned.");
    }
    
    return resp;
}

void httpServerLoop() {
    if (!s_server) return;
    
    EthernetClient client = s_server->available();
    if (!client) return;
    
    unsigned long httpStartTime = millis();  // TIMING DEBUG
    
    // Check IP whitelist
    IPAddress remoteIP = client.remoteIP();
    if (!authIsIPWhitelisted(remoteIP)) {
        String out = "{\"type\":\"error\",\"message\":\"IP not allowed\"}";
        httpSendResponse(client, 403, out);
        delay(1);
        client.stop();
        return;
    }
    
    Serial.print("[HTTP TIMING] t1 whitelist: "); Serial.println(millis() - httpStartTime);
    
    // Read first line of request
    String req = client.readStringUntil('\r');
    client.flush();
    
    Serial.print("[HTTP TIMING] t2 readLine: "); Serial.println(millis() - httpStartTime);
    
    if (req.startsWith("POST /")) {
        // Wait for data with timeout (prevents infinite blocking)
        unsigned long waitStart = millis();
        while (client.available() == 0 && (millis() - waitStart < 100)) {
            // Small delay to prevent busy-waiting
            delay(1);
        }
        
        Serial.print("[HTTP TIMING] t3 waitData: "); Serial.println(millis() - httpStartTime);
        
        // Check if we got data
        if (client.available() == 0) {
            // No data received within timeout
            httpSendResponse(client, 400, "{\"type\":\"error\",\"message\":\"Request timeout\"}");
            client.stop();
            return;
        }
        
        // Read full request
        String request = httpReadRequest(client);
        String body = httpExtractBody(request);
        
        Serial.print("[HTTP TIMING] t4 readBody: "); Serial.println(millis() - httpStartTime);
        
        Serial.println("Request body: " + body);
        
        // Parse JSON
        JsonDocument doc;
        deserializeJson(doc, body.c_str());
        
        Serial.println("Json content: " + body);
        Serial.println("Is Json null: " + String(doc.isNull()));
        
        String msgType = doc["type"];
        JsonDocument resp;
        int httpStatusCode = 200;
        
        if (msgType == "login") {
            Serial.println("Login type detected");
            resp = httpHandleLogin(doc);
        } else {
            // Not login - token required
            bool tokenIsValid = false;
            
            if (!doc["token"].is<String>()) {
                resp["type"] = "error";
                resp["message"] = "Token required.";
                httpStatusCode = 401;
            } else {
                String tokenRecv = doc["token"];
                tokenIsValid = authValidateToken(tokenRecv);
                
                if (tokenIsValid) {
                    authRefreshToken(tokenRecv);
                } else {
                    resp["type"] = "error";
                    resp["message"] = "Invalid or expired token.";
                    httpStatusCode = 401;
                }
            }
            
            if (tokenIsValid) {
                // Token is valid - process request
                s_lastUserConnectedTime = millis();
                s_userConnected = true;
                
                // Track this IP as active for UDP unicast
                IPAddress remoteIP = client.remoteIP();
                trackActiveIP(remoteIP);
                
                if (msgType == "get_status") {
                    resp = statusGenerateJson(&doc);
                }
                else if (msgType == "set_relay") {
                    uint8_t relayId = doc["relay_id"];
                    bool state = doc["state"];
                    
                    if (relayId < RELAY_COUNT) {
                        // Use auto-reset for relays 0 and 1
                        if ((relayId == 0 || relayId == 1) && state) {
                            relayControllerSet(relayId, state, RELAY_AUTO_RESET_DURATION);
                        } else {
                            relayControllerSet(relayId, state, 0);
                        }
                        resp = statusGenerateJson(&doc);
                    } else {
                        resp["type"] = "error";
                        resp["message"] = "Invalid relay number";
                    }
                }
                else if (msgType == "set_internal_led") {
                    bool state = doc["state"];
                    KMPProDinoMKRZero.SetStatusLed(state);
                    g_status.ledInternal = state;
                    resp = statusGenerateJson(&doc);
                }
                else if (msgType == "set_io_led") {
                    String color = doc["color"];
                    
                    if (color == "AUTO") {
                        ledControllerSetManualMode(false);
                        Serial.println("LED control returned to AUTO mode");
                        resp = statusGenerateJson(&doc);
                    } else {
                        LED_STATES colorVal;
                        bool validColor = true;
                        
                        if (color == "OFF") colorVal = OFF;
                        else if (color == "GREEN") colorVal = GREEN;
                        else if (color == "RED") colorVal = RED;
                        else if (color == "ORANGE") colorVal = ORANGE;
                        else {
                            validColor = false;
                            resp["type"] = "error";
                            resp["message"] = "Invalid LED color";
                        }
                        
                        if (validColor) {
                            ledControllerSetManualMode(true);
                            manual_led_state = colorVal;
                            _gg_hal.set_indicator_led(manual_led_state);
                            resp = statusGenerateJson(&doc);
                        }
                    }
                }
                else if (msgType == "set_serial_number") {
                    if (!technician_mode) {
                        resp["type"] = "error";
                        resp["message"] = "Technician mode required";
                        httpStatusCode = 403;
                    } else {
                        String newSN = doc["serial_number"];
                        // Note: serialNumberBurn will trigger NVIC_SystemReset on success
                        if (serialNumberBurn(newSN.c_str())) {
                            // This code won't execute - device reboots on success
                            resp["success"] = true;
                            resp["message"] = "Serial number set to: " + serialNumberGet();
                            resp["reboot_pending"] = true;
                        } else {
                            resp["type"] = "error";
                            if (serialNumberGet() != "UNCONFIGURED") {
                                resp["message"] = "Serial number already set and cannot be changed";
                            } else {
                                resp["message"] = "Invalid serial number (must be 2000-2999)";
                            }
                        }
                    }
                }
                else if (msgType == "get_serial_number") {
                    resp["type"] = "serial_number";
                    resp["serial_number"] = serialNumberGet();
                    resp["is_configured"] = (serialNumberGet() != "UNCONFIGURED");
                }
                else if (msgType == "set_ip_config") {
                    String controllerIPStr = doc["controller_ip"];
                    IPAddress newControllerIP;
                    bool ipValid = newControllerIP.fromString(controllerIPStr);
                    
                    JsonArray whitelistJson = doc["whitelist_ips"];
                    IPAddress newWhitelist[MAX_WHITELIST_IPS];
                    int newWhitelistCount = 0;
                    bool whitelistValid = true;
                    
                    for (JsonVariant ipVar : whitelistJson) {
                        IPAddress whitelistIP;
                        if (newWhitelistCount < MAX_WHITELIST_IPS && 
                            whitelistIP.fromString(ipVar.as<String>())) {
                            newWhitelist[newWhitelistCount++] = whitelistIP;
                        } else {
                            whitelistValid = false;
                            break;
                        }
                    }
                    
                    if (ipValid && whitelistValid) {
                        configSetControllerIP(newControllerIP);
                        configSetWhitelist(newWhitelist, newWhitelistCount);
                        configSave();
                        
                        resp["success"] = true;
                        resp["message"] = "IP configuration updated. Board will reboot.";
                        
                        // Send response before rebooting
                        String out;
                        serializeJson(resp, out);
                        httpSendResponse(client, 200, out);
                        client.stop();
                        
                        Serial.println("IP configuration saved. Initiating reboot in 2 seconds...");
                        delay(2000);
                        NVIC_SystemReset();
                    } else {
                        resp["type"] = "error";
                        resp["message"] = "Invalid IP address or whitelist entry provided.";
                    }
                }
                else if (msgType == "set_router_ip") {
                    String routerIPStr = doc["router_ip"];
                    IPAddress newRouterIP;
                    
                    if (newRouterIP.fromString(routerIPStr)) {
                        configSetRouterIP(newRouterIP);
                        configSave();
                        
                        resp["success"] = true;
                        resp["message"] = "Router IP updated to: " + g_routerIP.toString();
                        resp["reboot_required"] = true;
                        
                        Serial.println("Router IP saved: " + g_routerIP.toString());
                        Serial.println("Reboot required for MQTT to use new IP.");
                    } else {
                        resp["type"] = "error";
                        resp["message"] = "Invalid router IP address provided.";
                    }
                }
                else if (msgType == "get_router_ip") {
                    resp["type"] = "router_ip";
                    resp["router_ip"] = g_routerIP.toString();
                }
                else if (msgType == "reset_led_control") {
                    ledControllerSetManualMode(false);
                    _gg_hal.set_indicator_led(OFF);
                    resp = statusGenerateJson(&doc);
                }
                else {
                    resp["type"] = "error";
                    resp["message"] = "Unknown request type";
                }
            }
        }
        
        Serial.print("[HTTP TIMING] t5 processReq: "); Serial.println(millis() - httpStartTime);
        
        // Send response
        String out;
        serializeJson(resp, out);
        httpSendResponse(client, httpStatusCode, out);
        
        Serial.print("[HTTP TIMING] t6 sendResp: "); Serial.println(millis() - httpStartTime);
        
        if (msgType == "login") {
            Serial.println(resp["success"] ? "Client logged in" : "Client login failed");
        }
    }
    
    delay(1);
    client.stop();
    
    Serial.print("[HTTP TIMING] t7 TOTAL: "); Serial.println(millis() - httpStartTime);
}

bool httpIsUserConnected() {
    return s_userConnected;
}

unsigned long httpGetLastUserConnectedTime() {
    return s_lastUserConnectedTime;
}
