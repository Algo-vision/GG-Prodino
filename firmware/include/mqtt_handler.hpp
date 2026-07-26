/*
 * mqtt_handler.hpp
 *
 * MQTT publishing handler for GRK IoT device
 * Publishes sensor data to MQTT broker on configured topics
 * Topics include device serial number for multi-device support
 *
 * Transport-agnostic: the network client (plain EthernetClient for a local
 * broker, or an SSLClient-based TLS client for AWS IoT) is supplied by the
 * caller, so this class only deals with MQTT itself.
 */

#ifndef MQTT_HANDLER_HPP
#define MQTT_HANDLER_HPP

#include <Arduino.h>
#include <PubSubClient.h>
#include <Client.h>
#include <ArduinoJson.h>

// MQTT Broker Configuration
#define MQTT_USERNAME ""  // Empty for anonymous, set when authentication enabled
#define MQTT_PASSWORD ""  // Empty for anonymous
const unsigned long MQTT_PUBLISH_INTERVAL = 1000; // Publish every 1 second
const unsigned long MQTT_RECONNECT_INTERVAL = 30000; // Try reconnect every 30 seconds (was 5s)

class MQTTHandler {
private:
    Client& netClient;         // underlying transport (kept for stop()/error reset)
    PubSubClient mqttClient;
    unsigned long lastPublishTime;
    unsigned long lastReconnectAttempt;
    bool firstAttemptDone;
    bool connected;

    // Serial number and dynamic topic prefix
    String serialNumber;
    String topicPrefix;
    String clientId;

    // Broker endpoint (hostname, e.g. the AWS IoT device data endpoint)
    String brokerHost;
    uint16_t brokerPort;

    // Build a complete topic path
    String getTopic(const char* suffix) {
        return topicPrefix + String(suffix);
    }

public:
    // netClient: the transport to use (e.g. TLS client for AWS IoT)
    MQTTHandler(Client& client)
        : netClient(client), mqttClient(client), lastPublishTime(0),
          lastReconnectAttempt(0), firstAttemptDone(false), connected(false),
          brokerPort(0) {}

    // Begin with serial number and broker hostname + port
    void begin(const String& sn, const char* host, uint16_t port) {
        serialNumber = sn;
        topicPrefix = "grk/" + serialNumber + "/";
        clientId = "grk_" + serialNumber;
        brokerHost = host;
        brokerPort = port;

        // NOTE: setServer stores the pointer, so keep the hostname in a
        // String member and hand PubSubClient its stable c_str().
        mqttClient.setServer(brokerHost.c_str(), brokerPort);
        Serial.println("MQTT: Handler initialized for device: " + serialNumber);
        Serial.println("MQTT: Broker configured at " + brokerHost + ":" + String(brokerPort));

        // RAM diet for the on-board TLS build: publishes are STREAMED via
        // beginPublish/serializeJson/endPublish, so the packet buffer only needs
        // to hold the CONNECT packet and publish headers - 256 B instead of 2048.
        // (Revert to 2048 + buffered publish for the gateway build.)
        mqttClient.setBufferSize(256);

        // Persistent-connection design: AWS IoT accepts keepalive up to 1200 s
        // and RESETS the timer on every PUBLISH - so a 10-minute publish cadence
        // keeps the connection alive with no pings and NO re-handshakes.
        mqttClient.setKeepAlive(1200);
        mqttClient.setSocketTimeout(15);

        Serial.println("MQTT: Topic prefix: " + topicPrefix);
    }

    bool connectToMQTTBroker() {
        Serial.print("MQTT: Attempting connection to broker as " + clientId + "... ");

        // Try to connect with device-specific client ID
        bool result;
        if (strlen(MQTT_USERNAME) > 0) {
            result = mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD);
        } else {
            result = mqttClient.connect(clientId.c_str());
        }

        if (result) {
            Serial.println("connected!");
            connected = true;
            // Settle: brief session service before the first publish (publishing
            // the instant CONNACK arrives is a known AWS insta-disconnect
            // pattern). Trimmed 1500 -> 300 ms to shorten the publish window.
            unsigned long settleStart = millis();
            while (millis() - settleStart < 300) {
                mqttClient.loop();
                delay(50);
            }
            return true;
        } else {
            Serial.print("failed, rc=");
            Serial.println(mqttClient.state());
            // Fully reset the transport. SSLClient latches a write-error flag
            // after any failure; until stop() clears it, every subsequent
            // connect()/connected() call fails instantly (and spams errors).
            netClient.stop();
            connected = false;
            return false;
        }
    }

    void loop() {
        unsigned long now = millis();

        if (!connected) {
            // Disconnected: retry on the interval (first attempt immediately).
            // Deliberately avoid calling mqttClient.connected() here - on a
            // failed TLS transport every call logs an error, flooding Serial.
            if (!firstAttemptDone || (now - lastReconnectAttempt > MQTT_RECONNECT_INTERVAL)) {
                firstAttemptDone = true;
                lastReconnectAttempt = now;
                connectToMQTTBroker();
            }
            return;
        }

        // Connected: service the MQTT client; detect connection loss once.
        if (!mqttClient.loop()) {
            Serial.println("MQTT: Connection lost");
            netClient.stop();     // reset transport for a clean reconnect
            connected = false;
            lastReconnectAttempt = now;
        }
    }

    // Fully tear down the connection + transport so its RAM is released
    // (used by the time-multiplex publish window).
    void forceDisconnect() {
        mqttClient.disconnect();
        netClient.stop();
        connected = false;
    }

    bool isConnected() {
        // Order matters: when the flag is false, short-circuit so we never
        // poke a failed TLS client (each call would log an error).
        return connected && mqttClient.connected();
    }
    
    String getSerialNumber() {
        return serialNumber;
    }
    
    void publishStatus(JsonDocument& statusDoc) {
        if (!isConnected()) return;

        // STREAMED publish: serialize the JSON directly into the (TLS) socket.
        // No ~1 KB String copy and no big packet buffer -> saves ~2 KB of heap
        // high-water, which on the 32 KB board is the difference between HTTP
        // surviving after a publish window (needs ~2 KB gap) or hard-faulting.
        size_t len = measureJson(statusDoc);
        if (mqttClient.beginPublish(getTopic("status").c_str(), len, false)) {
            serializeJson(statusDoc, mqttClient);   // PubSubClient is a Print
            if (mqttClient.endPublish()) {
                Serial.println("MQTT: Published status (streamed)");
            }
        }
    }
    
    void publishGPS(double lat, double lng, double alt, 
                    float speedNorth, float speedEast, float speedDown, float groundSpeed,
                    float heading, bool valid, bool connected_status,
                    const char* timeStr, int satellites,
                    float hAcc, float vAcc, double altEllipsoid) {
        if (!isConnected()) return;
        
        // GPS Position
        JsonDocument posDoc;
        posDoc["lat"] = lat;
        posDoc["lng"] = lng;
        posDoc["alt"] = alt;
        String posJson;
        serializeJson(posDoc, posJson);
        mqttClient.publish(getTopic("gps/position").c_str(), posJson.c_str());
        
        // GPS Velocity
        JsonDocument velDoc;
        velDoc["north"] = speedNorth;
        velDoc["east"] = speedEast;
        velDoc["down"] = speedDown;
        velDoc["ground"] = groundSpeed;
        String velJson;
        serializeJson(velDoc, velJson);
        mqttClient.publish(getTopic("gps/velocity").c_str(), velJson.c_str());
        
        // GPS Heading
        String headingStr = String(heading, 2);
        mqttClient.publish(getTopic("gps/heading").c_str(), headingStr.c_str());
        
        // GPS Validity
        JsonDocument validDoc;
        validDoc["valid"] = valid;
        validDoc["connected"] = connected_status;
        String validJson;
        serializeJson(validDoc, validJson);
        mqttClient.publish(getTopic("validity/gps").c_str(), validJson.c_str());
        
        // GPS Time
        if (timeStr != nullptr && strlen(timeStr) > 0) {
            mqttClient.publish(getTopic("gps/time").c_str(), timeStr);
        }
        
        // GPS Satellites
        mqttClient.publish(getTopic("gps/satellites").c_str(), String(satellites).c_str());
        
        // GPS Accuracy (from UBX NAV-PVT)
        JsonDocument accDoc;
        accDoc["hAcc"] = hAcc;
        accDoc["vAcc"] = vAcc;
        accDoc["altEllipsoid"] = altEllipsoid;
        String accJson;
        serializeJson(accDoc, accJson);
        mqttClient.publish(getTopic("gps/accuracy").c_str(), accJson.c_str());
    }
    
    void publishIMU(float ax, float ay, float az,
                    float gx, float gy, float gz,
                    float pitch, float roll, float yaw,
                    bool valid) {
        if (!isConnected()) return;
        
        // IMU Accelerometer
        JsonDocument accelDoc;
        accelDoc["x"] = ax;
        accelDoc["y"] = ay;
        accelDoc["z"] = az;
        String accelJson;
        serializeJson(accelDoc, accelJson);
        mqttClient.publish(getTopic("imu/accel").c_str(), accelJson.c_str());
        
        // IMU Gyroscope
        JsonDocument gyroDoc;
        gyroDoc["gx"] = gx;
        gyroDoc["gy"] = gy;
        gyroDoc["gz"] = gz;
        String gyroJson;
        serializeJson(gyroDoc, gyroJson);
        mqttClient.publish(getTopic("imu/gyro").c_str(), gyroJson.c_str());
        
        // IMU Orientation
        JsonDocument orientDoc;
        orientDoc["pitch"] = pitch;
        orientDoc["roll"] = roll;
        orientDoc["yaw"] = yaw;
        String orientJson;
        serializeJson(orientDoc, orientJson);
        mqttClient.publish(getTopic("imu/orientation").c_str(), orientJson.c_str());
        
        // IMU Validity
        String validStr = valid ? "true" : "false";
        mqttClient.publish(getTopic("validity/imu").c_str(), validStr.c_str());
    }
    
    void publishRelays(bool r0, bool r1, bool r2, bool r3) {
        if (!isConnected()) return;
        
        JsonDocument doc;
        JsonArray relays = doc.to<JsonArray>();
        relays.add(r0);
        relays.add(r1);
        relays.add(r2);
        relays.add(r3);
        
        String json;
        serializeJson(doc, json);
        mqttClient.publish(getTopic("relays/state").c_str(), json.c_str());
    }
    
    void publishLEDs(bool internal, const char* ioColor) {
        if (!isConnected()) return;
        
        // Internal LED
        String internalStr = internal ? "true" : "false";
        mqttClient.publish(getTopic("leds/internal").c_str(), internalStr.c_str());
        
        // IO LED
        mqttClient.publish(getTopic("leds/io").c_str(), ioColor);
    }
    
    void publishSensors(bool opto0, bool opto1, bool opto2, bool opto3, bool button) {
        if (!isConnected()) return;
        
        // Opto-isolators
        JsonDocument doc;
        JsonArray optos = doc.to<JsonArray>();
        optos.add(opto0);
        optos.add(opto1);
        optos.add(opto2);
        optos.add(opto3);
        
        String json;
        serializeJson(doc, json);
        mqttClient.publish(getTopic("sensors/optos").c_str(), json.c_str());
        
        // Button
        String buttonStr = button ? "true" : "false";
        mqttClient.publish(getTopic("sensors/button_tech").c_str(), buttonStr.c_str());
    }
    
    // Publish power monitoring data
    void publishPower(bool connected, float busVoltage) {
        if (!isConnected()) return;
        
        // Connection status
        String connStr = connected ? "true" : "false";
        mqttClient.publish(getTopic("power/connected").c_str(), connStr.c_str());
        
        // Bus voltage (only if connected)
        if (connected) {
            String voltageStr = String(busVoltage, 2);
            mqttClient.publish(getTopic("power/bus_voltage").c_str(), voltageStr.c_str());
        }
    }
    
    unsigned long getLastPublishTime() {
        return lastPublishTime;
    }
    
    void setLastPublishTime(unsigned long time) {
        lastPublishTime = time;
    }
};

#endif // MQTT_HANDLER_HPP

