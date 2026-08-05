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
#include "ocu_monitor.hpp"
#include "imu_mount_orientation.hpp"
#include "KMPProDinoMKRZero.h"
#include "gg_hal.hpp"
#include <Arduino_DebugUtils.h>  // For NVIC_SystemReset()

// ============================================================================
// CONFIGURATION
// ============================================================================

/** Auto-reset duration for relays 0 and 1 (5 seconds) */
constexpr unsigned long RELAY_AUTO_RESET_DURATION = 5000;

/** Max ms to wait for a client socket to close gracefully before force-closing.
 *  Kept small so a slow/lingering TCP close can't stall the main loop (and thus
 *  the next request). The default library value is 1000ms - far too long here. */
constexpr uint16_t HTTP_CLIENT_CLOSE_TIMEOUT_MS = 20;

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
    
    // Build the whole response (status line + headers + body) in one buffer and
    // send it with a SINGLE write, instead of ~10 small client.print() calls.
    // Each small write was a separate TCP segment that could stall on Nagle +
    // the client's delayed-ACK; coalescing helps most on larger responses
    // (measured: get_status p50 ~106ms -> ~61ms). The remaining per-request floor
    // (~40ms) is the connection-per-request model (setup + stop() teardown), which
    // is intentional for the 8-socket W5500 - see the accept loop below.
    String resp;
    resp.reserve(content.length() + 128);
    resp += "HTTP/1.1 ";
    resp += statusCode;
    resp += ' ';
    resp += statusText;
    resp += "\r\nContent-Type: ";
    resp += contentType;
    resp += "\r\nConnection: close\r\nContent-Length: ";
    resp += content.length();
    resp += "\r\n\r\n";
    resp += content;
    client.print(resp);
}

/**
 * @brief Send a JSON response with ZERO heap allocation (RAM-safe).
 *
 * The String-based path costs ~2 KB of transient heap for a big response
 * (serialize-to-String + header+body copy). With the TLS stack resident only
 * ~2.4 KB is free, so a large get_status response could collide and wedge the
 * board. This variant serializes into a STACK buffer (reclaimed on return, no
 * heap high-water) and sends header+body as one write. Oversized responses
 * fall back to streaming chunks directly into the socket.
 */
void httpStreamJsonResponse(EthernetClient& client, int statusCode, JsonDocument& doc) {
    const char* statusText = "OK";
    switch (statusCode) {
        case 200: statusText = "OK"; break;
        case 401: statusText = "Unauthorized"; break;
        case 403: statusText = "Forbidden"; break;
        case 404: statusText = "Not Found"; break;
        case 500: statusText = "Internal Server Error"; break;
        default:  statusText = "Unknown"; break;
    }
    size_t bodyLen = measureJson(doc);
    char buf[1408];                     // stack: freed on return, no heap growth
    int hdrLen = snprintf(buf, sizeof(buf),
        "HTTP/1.1 %d %s\r\nContent-Type: application/json\r\n"
        "Connection: close\r\nContent-Length: %u\r\n\r\n",
        statusCode, statusText, (unsigned)bodyLen);
    if (hdrLen + bodyLen < sizeof(buf)) {
        serializeJson(doc, buf + hdrLen, sizeof(buf) - hdrLen);
        client.write((const uint8_t*)buf, hdrLen + bodyLen);   // one segment
    } else {
        // response bigger than the buffer: send header, then stream the body
        client.write((const uint8_t*)buf, hdrLen);
        serializeJson(doc, client);
    }
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
    
    // Process ALL available clients (up to 4 per loop iteration)
    // This ensures we handle multiple simultaneous requests efficiently
    int clientsProcessed = 0;
    const int MAX_CLIENTS_PER_LOOP = 4;  // W5500 supports up to 4-8 sockets
    
    while (clientsProcessed < MAX_CLIENTS_PER_LOOP) {
        EthernetClient client = s_server->available();
        if (!client) break;  // No more clients waiting

        // Cap the socket-close wait. The Ethernet library's stop() blocks up to
        // _timeout ms (default 1000!) waiting for a graceful FIN handshake before
        // force-closing. Since every response is sent with "Connection: close" and
        // we stop() immediately, that full-second wait would freeze loop() - and
        // therefore delay the NEXT request's pickup by up to ~1s. A short wait then
        // force-close keeps request latency low. (See EthernetClient::stop().)
        client.setConnectionTimeout(HTTP_CLIENT_CLOSE_TIMEOUT_MS);

        clientsProcessed++;

        IPAddress remoteIP = client.remoteIP();

        // Check IP whitelist
        if (!authIsIPWhitelisted(remoteIP)) {
            String out = "{\"type\":\"error\",\"message\":\"IP not allowed\"}";
            httpSendResponse(client, 403, out);
            delay(1);
            client.stop();
            continue;  // Process next client
        }    
    // Read first line of request
    String req = client.readStringUntil('\r');
    client.flush();

    if (req.startsWith("POST /")) {
        // Wait for data with timeout (prevents infinite blocking)
        unsigned long waitStart = millis();
        while (client.available() == 0 && (millis() - waitStart < 100)) {
            // Small delay to prevent busy-waiting
            delay(1);
        }


        // Check if we got data
        if (client.available() == 0) {
            // No data received within timeout
            httpSendResponse(client, 400, "{\"type\":\"error\",\"message\":\"Request timeout\"}");
            client.stop();
            continue;  // Process next client
        }

        // Read full request
        String request = httpReadRequest(client);
        String body = httpExtractBody(request);

        // Parse JSON
        JsonDocument doc;
        deserializeJson(doc, body.c_str());

        String msgType = doc["type"];
        JsonDocument resp;
        int httpStatusCode = 200;
        bool rawResponseSent = false;  // set by handlers that stream their own body

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
                    // Buffered snapshot, NOT a fresh sensor read. loop() refreshes
                    // it in the background; serving from the buffer keeps a 20 Hz
                    // consumer from paying ~35 ms of blocking I2C per request.
                    resp = statusGenerateJsonNoRefresh();
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
                        resp = statusGenerateJsonNoRefresh();
                    } else {
                        resp["type"] = "error";
                        resp["message"] = "Invalid relay number";
                    }
                }
                else if (msgType == "set_internal_led") {
                    bool state = doc["state"];
                    KMPProDinoMKRZero.SetStatusLed(state);
                    g_status.ledInternal = state;
                    resp = statusGenerateJsonNoRefresh();
                }
                else if (msgType == "set_io_led") {
                    String color = doc["color"];
                    
                    if (color == "AUTO") {
                        ledControllerSetManualMode(false);
                        Serial.println("LED control returned to AUTO mode");
                        resp = statusGenerateJsonNoRefresh();
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
                            resp = statusGenerateJsonNoRefresh();
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
                        Serial.println("Reboot required for the new IP to take effect.");
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
                    resp = statusGenerateJsonNoRefresh();
                }
                else if (msgType == "get_config") {
                    statusUpdate();
                    resp["type"] = "config";
                    resp["firmwareVersion"] = statusGetFirmwareVersion();
                    resp["serialNumber"] = serialNumberGet();
                    resp["controllerIp"] = g_controllerIP.toString();

                    JsonArray whitelistArray = resp["whitelistIps"].to<JsonArray>();
                    for (int i = 0; i < g_whitelistCount; ++i) {
                        whitelistArray.add(g_whitelist[i].toString());
                    }

                    resp["technicianMode"] = technician_mode;
                    resp["burnedHours"] = round(configGetBurnedHoursFloat() * 100) / 100.0;
                    resp["sessionHours"] = round((millis() / 3600000.0) * 100) / 100.0;

                    switch (g_status.ledIo) {
                        case OFF:    resp["techLedColor"] = "OFF";    break;
                        case GREEN:  resp["techLedColor"] = "GREEN";  break;
                        case RED:    resp["techLedColor"] = "RED";    break;
                        case ORANGE: resp["techLedColor"] = "ORANGE"; break;
                        default:     resp["techLedColor"] = "OFF";    break;
                    }

                    resp["imuMountOrientation"] = imuMountOrientationToString(g_imuMountOrientation);
                }
                else if (msgType == "set_imu_mount_orientation") {
                    String orientationStr = doc["orientation"];
                    int newOrientation = imuMountOrientationFromString(orientationStr.c_str());

                    if (newOrientation >= 0) {
                        configSetImuMountOrientation((uint8_t)newOrientation);
                        configSave();

                        resp["success"] = true;
                        resp["message"] = "IMU mount orientation set to: " + orientationStr;
                        resp["reboot_required"] = true;

                        Serial.println("IMU mount orientation saved: " + orientationStr);
                        Serial.println("Reboot required to recalibrate for the new orientation.");
                    } else {
                        resp["type"] = "error";
                        resp["message"] = "Invalid IMU mount orientation";
                    }
                }
                else if (msgType == "get_overview") {
                    statusUpdate();
                    resp["type"] = "overview";

                    resp["powerConnected"] = g_status.powerConnected;
                    resp["powerSane"] = g_status.powerSane;
                    resp["busVoltage"] = g_status.busVoltage;
                    resp["busCurrent_mA"] = g_status.busCurrent_mA;

                    JsonArray relaysArray = resp["relays_status"].to<JsonArray>();
                    for (uint8_t i = 0; i < RELAY_COUNT; i++) {
                        relaysArray.add(KMPProDinoMKRZero.GetRelayState(i));
                    }
                    JsonArray optosArray = resp["optoin_status"].to<JsonArray>();
                    for (uint8_t i = 0; i < OPTOIN_COUNT; i++) {
                        optosArray.add(g_status.optos_status[i]);
                    }

                    resp["safetyMode"] = g_status.safetyMode;
                    resp["safetyModeDurationMs"] = g_status.safetyModeUnsafeDurationMs;

                    resp["ocuConnected"] = ocuMonitorIsConnected();
                    resp["ocuDisconnectedDurationMs"] = ocuMonitorDisconnectedDurationMs();

                    // No Jetson communication channel exists yet - this is the
                    // documented "no comm" fallback value, not a placeholder.
                    resp["jetsonCpuTemp"] = -1;
                }
                else if (msgType == "get_imu") {
                    statusUpdate();
                    resp["type"] = "imu";

                    resp["angleSane"] = g_status.angleSane;
                    resp["pitch"] = g_status.pitch;
                    resp["roll"] = g_status.roll;
                    resp["yaw"] = g_status.yaw;

                    resp["imuValid"] = g_status.imuValid;
                    resp["imu1Sane"] = g_status.imu1Sane;
                    resp["imuX"] = g_status.imuX;
                    resp["imuY"] = g_status.imuY;
                    resp["imuZ"] = g_status.imuZ;
                    resp["imuGx"] = g_status.imuGx;
                    resp["imuGy"] = g_status.imuGy;
                    resp["imuGz"] = g_status.imuGz;

                    resp["imu2Valid"] = g_status.imu2Valid;
                    resp["imu2Sane"] = g_status.imu2Sane;
                    resp["imu2X"] = g_status.imu2X;
                    resp["imu2Y"] = g_status.imu2Y;
                    resp["imu2Z"] = g_status.imu2Z;
                    resp["imu2Gx"] = g_status.imu2Gx;
                    resp["imu2Gy"] = g_status.imu2Gy;
                    resp["imu2Gz"] = g_status.imu2Gz;
                }
                else if (msgType == "get_gps") {
                    statusUpdate();
                    resp["type"] = "gps";

                    resp["gpsConnected"] = g_status.gpsConnected;
                    resp["gpsSane"] = g_status.gpsSane;
                    resp["gpsSatellites"] = g_status.gpsSatellites;
                    resp["gpsLat"] = g_status.gpsLat;
                    resp["gpsLng"] = g_status.gpsLng;
                    resp["gpsAlt"] = g_status.gpsAlt;
                    resp["gpsHeading"] = g_status.gpsHeading;
                    resp["gpsGroundSpeed"] = g_status.gpsGroundSpeed;
                    resp["gpsSpeedNorth"] = g_status.gpsSpeedNorth;
                    resp["gpsSpeedEast"] = g_status.gpsSpeedEast;
                    resp["gpsSpeedDown"] = g_status.gpsSpeedDown;
                    resp["gpsTime"] = g_status.gpsTime;
                    resp["lastGpsLat"] = g_status.lastGpsLat;
                    resp["lastGpsLng"] = g_status.lastGpsLng;
                    resp["lastGpsAlt"] = g_status.lastGpsAlt;
                }
                else {
                    resp["type"] = "error";
                    resp["message"] = "Unknown request type";
                }
            }
        }
        
        
        // Send response (unless a handler already streamed its own raw body).
        // RAM-safe path: no String copies - big responses (get_status ~950 B)
        // were colliding with the TLS heap and wedging the board.
        if (!rawResponseSent) {
            httpStreamJsonResponse(client, httpStatusCode, resp);
        }

        if (msgType == "login") {
            Serial.println(resp["success"] ? "Client logged in" : "Client login failed");
        }
    }

    delay(1);
    client.stop();

    }  // End of while loop
}

bool httpIsUserConnected() {
    return s_userConnected;
}

unsigned long httpGetLastUserConnectedTime() {
    return s_lastUserConnectedTime;
}
