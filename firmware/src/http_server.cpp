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
#include <rest_detector.hpp>
#include "timing_probe.hpp"
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
            resp["code"] = "E-110";
            resp["message"] = "Invalid username";
        } else {
            resp["code"] = "E-111";
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
            String out = "{\"type\":\"error\",\"code\":\"E-103\",\"message\":\"IP not allowed\"}";
            httpSendResponse(client, 403, out);
            delay(1);
            client.stop();
            continue;  // Process next client
        }

        // A request from the OCU (.169) is proof it is alive and reachable -
        // this is what drives ocuConnected. See ocu_monitor.hpp.
        ocuMonitorNotifyActivity(remoteIP);

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
            httpSendResponse(client, 400, "{\"type\":\"error\",\"code\":\"E-100\",\"message\":\"Request timeout\"}");
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
                resp["code"] = "E-101";
                resp["message"] = "Token required.";
                httpStatusCode = 401;
            } else {
                String tokenRecv = doc["token"];
                tokenIsValid = authValidateToken(tokenRecv);
                
                if (tokenIsValid) {
                    authRefreshToken(tokenRecv);
                } else {
                    resp["type"] = "error";
                    resp["code"] = "E-102";
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
                        resp["code"] = "E-201";
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
                            resp["code"] = "E-202";
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
                        resp["code"] = "E-203";
                        resp["message"] = "Invalid IP address or whitelist entry provided.";
                    }
                }
                else if (msgType == "reset_led_control") {
                    ledControllerSetManualMode(false);
                    _gg_hal.set_indicator_led(OFF);
                    resp = statusGenerateJson(&doc);
                }
                else if (msgType == "get_config") {
                    resp["type"] = "config";
                    resp["firmwareVersion"] = statusGetFirmwareVersion();
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

                    resp["imuPitchAxis"]   = imuAxisToString(g_imuAxisMap.pitchAxis);
                    resp["imuRollAxis"]    = imuAxisToString(g_imuAxisMap.rollAxis);
                    resp["imuYawAxis"]     = imuAxisToString(g_imuAxisMap.yawAxis);
                    resp["imuPitchInvert"] = (bool)g_imuAxisMap.pitchInvert;
                    resp["imuRollInvert"]  = (bool)g_imuAxisMap.rollInvert;
                    resp["imuYawInvert"]   = (bool)g_imuAxisMap.yawInvert;
                }
                else if (msgType == "set_imu_axis_map") {
                    String pitchStr = doc["pitch_axis"];
                    String rollStr  = doc["roll_axis"];
                    String yawStr   = doc["yaw_axis"];

                    int pitchAxis = imuAxisFromString(pitchStr.c_str());
                    int rollAxis  = imuAxisFromString(rollStr.c_str());
                    int yawAxis   = imuAxisFromString(yawStr.c_str());

                    // Invert flags are optional - omitting one leaves that axis
                    // un-negated, so an older client keeps working unchanged.
                    bool pitchInvert = doc["pitch_invert"] | false;
                    bool rollInvert  = doc["roll_invert"]  | false;
                    bool yawInvert   = doc["yaw_invert"]   | false;

                    if (pitchAxis < 0 || rollAxis < 0 || yawAxis < 0) {
                        resp["type"] = "error";
                        resp["code"] = "E-204";
                        resp["message"] = "Invalid IMU axis (expected X, Y or Z)";
                    } else {
                        ImuAxisMap newMap = {(uint8_t)pitchAxis, (uint8_t)rollAxis, (uint8_t)yawAxis,
                                             (uint8_t)(pitchInvert ? 1 : 0),
                                             (uint8_t)(rollInvert ? 1 : 0),
                                             (uint8_t)(yawInvert ? 1 : 0)};

                        if (!imuAxisMapIsValid(newMap)) {
                            resp["type"] = "error";
                            resp["code"] = "E-205";
                            resp["message"] = "Pitch, roll and yaw must each use a different axis";
                        } else {
                            configSetImuAxisMap(newMap);
                            configSave();

                            String summary = "pitch=" + String(pitchInvert ? "-" : "") + pitchStr +
                                             " roll="  + String(rollInvert  ? "-" : "") + rollStr +
                                             " yaw="   + String(yawInvert   ? "-" : "") + yawStr;
                            resp["success"] = true;
                            resp["message"] = "IMU axis map set to: " + summary;
                            resp["reboot_required"] = true;

                            Serial.println("IMU axis map saved: " + summary);
                            Serial.println("Reboot required to recalibrate for the new axis map.");
                        }
                    }
                }
                else if (msgType == "get_overview") {
                    resp["type"] = "overview";

                    resp["powerConnected"] = g_status.powerConnected;
                    resp["powerSane"] = g_status.powerSane;
                    resp["systemVoltage"] = g_status.systemVoltage;
                    resp["systemCurrent_A"] = g_status.systemCurrent_A;

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
                }
                else if (msgType == "get_imu") {
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

                    resp["imuTemp"] = g_status.imuTemp;

                    // Same calibration state get_status carries, so a client
                    // polling either command sees it.
                    resp["zeroCalValid"] = g_zeroCalValid;
                    {
                        float mp = 0.0f, mr = 0.0f;
                        statusZeroCalAngles(mp, mr);
                        resp["mountPitch"] = mp;
                        resp["mountRoll"]  = mr;
                    }
                    resp["restSeconds"] = statusRestSeconds();
                    resp["atRest"] = statusRestSeconds() >= REST_SECONDS_FOR_CALIBRATION;
                }
                else if (msgType == "burn_zero_calibration") {
                    // The technician-GUI equivalent of burning a serial
                    // number: declares THIS attitude to be level for THIS
                    // installation and writes it to flash. Never happens on
                    // its own - a controller that has never been calibrated
                    // reports the sensor frame and says so.
                    const char *err = nullptr;
                    bool ok = statusBurnZeroCalibration(err);
                    resp["type"] = "zero_calibration_result";
                    resp["success"] = ok;
                    resp["restSeconds"] = statusRestSeconds();
                    if (ok) {
                        float p = 0.0f, r = 0.0f;
                        statusZeroCalAngles(p, r);
                        resp["mountPitch"] = p;
                        resp["mountRoll"] = r;
                    } else {
                        resp["code"] = "E-300";
                        resp["message"] = err ? err : "calibration refused";
                        httpStatusCode = 409;
                    }
                }
                else if (msgType == "calibrate_now") {
                    // Initiated calibration: throw away accumulated drift.
                    // Stores nothing, and is valid at any attitude.
                    const char *err = nullptr;
                    bool ok = statusInitiatedCalibration(err);
                    resp["type"] = "calibrate_now_result";
                    resp["success"] = ok;
                    resp["restSeconds"] = statusRestSeconds();
                    if (ok) {
                        resp["pitch"] = g_status.pitch;
                        resp["roll"] = g_status.roll;
                        resp["yaw"] = g_status.yaw;
                    } else {
                        resp["code"] = "E-301";
                        resp["message"] = err ? err : "calibration refused";
                        httpStatusCode = 409;
                    }
                }
                else if (msgType == "get_calibration") {
                    resp["type"] = "calibration";
                    resp["zeroCalValid"] = g_zeroCalValid;
                    float p = 0.0f, r = 0.0f;
                    statusZeroCalAngles(p, r);
                    resp["mountPitch"] = p;
                    resp["mountRoll"] = r;
                    resp["gyroBiasValid"] = g_gyroBiasValid;
                    JsonArray b1 = resp["gyroBias1"].to<JsonArray>();
                    JsonArray b2 = resp["gyroBias2"].to<JsonArray>();
                    for (int i = 0; i < 3; ++i) {
                        b1.add(g_imu1GyroBias[i]);
                        b2.add(g_imu2GyroBias[i]);
                    }
                    // So the GUI can grey out the calibrate button, and say
                    // WHY, instead of letting the request fail.
                    resp["restSeconds"] = statusRestSeconds();
                    resp["restRequiredS"] = REST_SECONDS_FOR_CALIBRATION;
                    resp["atRest"] = statusRestSeconds() >= REST_SECONDS_FOR_CALIBRATION;
                }
                else if (msgType == "get_gps") {
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
#if TIMING_PROBE
                // Bench instrumentation, present only in the timing build.
                // Arm, leave the board alone for the window, then read the
                // frozen result - so the readout is never part of what was
                // measured. See timing_probe.hpp.
                else if (msgType == "reset_timing") {
                    uint32_t windowS = doc["window_s"].is<uint32_t>()
                                           ? doc["window_s"].as<uint32_t>()
                                           : 60;
                    timingProbeArm(windowS);
                    timingProbeToJson(resp);
                }
                else if (msgType == "get_timing") {
                    timingProbeToJson(resp);
                }
#endif
                else {
                    resp["type"] = "error";
                    resp["code"] = "E-200";
                    resp["message"] = "Unknown request type";
                }
            }
        }
        
        
        // Send response (unless a handler already streamed its own raw body)
        if (!rawResponseSent) {
            String out;
            serializeJson(resp, out);
            httpSendResponse(client, httpStatusCode, out);
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
