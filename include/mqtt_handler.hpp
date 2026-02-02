/*
 * mqtt_handler.hpp
 * 
 * MQTT publishing handler for Prodino IoT device
 * Publishes sensor data to MQTT broker on configured topics
 * Topics include device serial number for multi-device support
 */

#ifndef MQTT_HANDLER_HPP
#define MQTT_HANDLER_HPP

#include <Arduino.h>
#include <PubSubClient.h>
#include <Ethernet.h>
#include <ArduinoJson.h>

// MQTT Broker Configuration
#define MQTT_BROKER_PORT 1883
#define MQTT_USERNAME ""  // Empty for anonymous, set when authentication enabled
#define MQTT_PASSWORD ""  // Empty for anonymous
const unsigned long MQTT_PUBLISH_INTERVAL = 1000; // Publish every 1 second
const unsigned long MQTT_RECONNECT_INTERVAL = 30000; // Try reconnect every 30 seconds (was 5s)

class MQTTHandler {
private:
    EthernetClient ethClient;
    PubSubClient mqttClient;
    unsigned long lastPublishTime;
    unsigned long lastReconnectAttempt;
    bool connected;
    
    // Serial number and dynamic topic prefix
    String serialNumber;
    String topicPrefix;
    String clientId;
    
    // Dynamic broker IP
    IPAddress brokerIp;
    
    // Build a complete topic path
    String getTopic(const char* suffix) {
        return topicPrefix + String(suffix);
    }
    
public:
    MQTTHandler() : mqttClient(ethClient), lastPublishTime(0), lastReconnectAttempt(0), connected(false) {}
    
    // Begin with serial number and broker IP
    void begin(const String& sn, const IPAddress& brokerIpAddr) {
        serialNumber = sn;
        topicPrefix = "prodino/" + serialNumber + "/";
        clientId = "prodino_" + serialNumber;
        brokerIp = brokerIpAddr;
        
        // Set socket timeout to prevent blocking on connection failures
        // This reduces the delay when broker is unreachable from ~5s to ~500ms
        ethClient.setConnectionTimeout(500);  // 500ms timeout
        
        mqttClient.setServer(brokerIp, MQTT_BROKER_PORT);
        Serial.println("MQTT: Handler initialized for device: " + serialNumber);
        Serial.print("MQTT: Broker configured at ");
        Serial.print(brokerIp.toString());
        Serial.print(":");
        Serial.println(MQTT_BROKER_PORT);
        
        // Increase buffer size to handle full status JSON (~1040 bytes)
        // Default is 256 bytes. We use 2048 to be safe and allow for future growth.
        mqttClient.setBufferSize(2048);
        
        Serial.println("MQTT: Topic prefix: " + topicPrefix);
    }
    
    // Legacy begin() for backward compatibility - uses default IP
    void begin(const String& sn) {
        begin(sn, IPAddress(192, 168, 1, 1));  // Default Teltonika router IP
    }
    
    // Legacy begin() for backward compatibility
    void begin() {
        begin("DEFAULT", IPAddress(192, 168, 1, 1));
    }
    
    // Get current broker IP
    IPAddress getBrokerIp() {
        return brokerIp;
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
            return true;
        } else {
            Serial.print("failed, rc=");
            Serial.println(mqttClient.state());
            connected = false;
            return false;
        }
    }
    
    void loop() {
        if (!mqttClient.connected()) {
            connected = false;
            unsigned long now = millis();
            if (now - lastReconnectAttempt > MQTT_RECONNECT_INTERVAL) {
                lastReconnectAttempt = now;
                if (connectToMQTTBroker()) {
                    lastReconnectAttempt = 0;
                }
            }
        } else {
            mqttClient.loop(); // Maintain MQTT connection
        }
    }
    
    bool isConnected() {
        return connected && mqttClient.connected();
    }
    
    String getSerialNumber() {
        return serialNumber;
    }
    
    void publishStatus(JsonDocument& statusDoc) {
        if (!isConnected()) return;
        
        String jsonString;
        serializeJson(statusDoc, jsonString);
        
        if (mqttClient.publish(getTopic("status").c_str(), jsonString.c_str())) {
            Serial.println("MQTT: Published status");
        }
    }
    
    void publishGPS(double lat, double lng, double alt, 
                    float speedNorth, float speedEast, float speedDown, float groundSpeed,
                    float heading, bool valid, bool connected_status,
                    const char* timeStr, int satellites) {
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
    
    // Publish power monitoring data (INA219)
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

