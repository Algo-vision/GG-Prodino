/*
 * mqtt_handler.hpp
 * 
 * MQTT publishing handler for Prodino IoT device
 * Publishes sensor data to MQTT broker on configured topics
 */

#ifndef MQTT_HANDLER_HPP
#define MQTT_HANDLER_HPP

#include <Arduino.h>
#include <PubSubClient.h>
#include <Ethernet.h>
#include <ArduinoJson.h>

// MQTT Broker Configuration
#define MQTT_BROKER_IP "192.168.100.131"  // Your PC IP (change to "192.168.1.1" for RUTX12)
#define MQTT_BROKER_PORT 1883
#define MQTT_CLIENT_ID "prodino_001"
#define MQTT_USERNAME ""  // Empty for anonymous, set when authentication enabled
#define MQTT_PASSWORD ""  // Empty for anonymous
const unsigned long MQTT_PUBLISH_INTERVAL = 1000; // Publish every 1 second
const unsigned long MQTT_RECONNECT_INTERVAL = 5000; // Try reconnect every 5 seconds

// MQTT Topics
#define TOPIC_STATUS "prodino/status"
#define TOPIC_GPS_POSITION "prodino/gps/position"
#define TOPIC_GPS_VELOCITY "prodino/gps/velocity"
#define TOPIC_GPS_HEADING "prodino/gps/heading"
#define TOPIC_IMU_ACCEL "prodino/imu/accel"
#define TOPIC_IMU_GYRO "prodino/imu/gyro"
#define TOPIC_IMU_ORIENTATION "prodino/imu/orientation"
#define TOPIC_RELAYS_STATE "prodino/relays/state"
#define TOPIC_LEDS_INTERNAL "prodino/leds/internal"
#define TOPIC_LEDS_IO "prodino/leds/io"
#define TOPIC_SENSORS_OPTOS "prodino/sensors/optos"
#define TOPIC_SENSORS_BUTTON "prodino/sensors/button_tech"
#define TOPIC_VALIDITY_GPS "prodino/validity/gps"
#define TOPIC_VALIDITY_IMU "prodino/validity/imu"

class MQTTHandler {
private:
    EthernetClient ethClient;
    PubSubClient mqttClient;
    unsigned long lastPublishTime;
    unsigned long lastReconnectAttempt;
    bool connected;
    
public:
    MQTTHandler() : mqttClient(ethClient), lastPublishTime(0), lastReconnectAttempt(0), connected(false) {}
    
    void begin() {
        mqttClient.setServer(MQTT_BROKER_IP, MQTT_BROKER_PORT);
        Serial.println("MQTT: Handler initialized");
        Serial.print("MQTT: Broker configured at ");
        Serial.print(MQTT_BROKER_IP);
        Serial.print(":");
        Serial.println(MQTT_BROKER_PORT);
    }
    
    bool connectToMQTTBroker() {
        Serial.print("MQTT: Attempting connection to broker... ");
        
        // Try to connect
        bool result;
        if (strlen(MQTT_USERNAME) > 0) {
            result = mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD);
        } else {
            result = mqttClient.connect(MQTT_CLIENT_ID);
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
    
    void publishStatus(JsonDocument& statusDoc) {
        if (!isConnected()) return;
        
        String jsonString;
        serializeJson(statusDoc, jsonString);
        
        if (mqttClient.publish(TOPIC_STATUS, jsonString.c_str())) {
            Serial.println("MQTT: Published status");
        }
    }
    
    void publishGPS(double lat, double lng, double alt, 
                    float speedNorth, float speedEast, float speedDown, float groundSpeed,
                    float heading, bool valid, bool connected_status) {
        if (!isConnected()) return;
        
        // GPS Position
        JsonDocument posDoc;
        posDoc["lat"] = lat;
        posDoc["lng"] = lng;
        posDoc["alt"] = alt;
        String posJson;
        serializeJson(posDoc, posJson);
        mqttClient.publish(TOPIC_GPS_POSITION, posJson.c_str());
        
        // GPS Velocity
        JsonDocument velDoc;
        velDoc["north"] = speedNorth;
        velDoc["east"] = speedEast;
        velDoc["down"] = speedDown;
        velDoc["ground"] = groundSpeed;
        String velJson;
        serializeJson(velDoc, velJson);
        mqttClient.publish(TOPIC_GPS_VELOCITY, velJson.c_str());
        
        // GPS Heading
        String headingStr = String(heading, 2);
        mqttClient.publish(TOPIC_GPS_HEADING, headingStr.c_str());
        
        // GPS Validity
        JsonDocument validDoc;
        validDoc["valid"] = valid;
        validDoc["connected"] = connected_status;
        String validJson;
        serializeJson(validDoc, validJson);
        mqttClient.publish(TOPIC_VALIDITY_GPS, validJson.c_str());
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
        mqttClient.publish(TOPIC_IMU_ACCEL, accelJson.c_str());
        
        // IMU Gyroscope
        JsonDocument gyroDoc;
        gyroDoc["gx"] = gx;
        gyroDoc["gy"] = gy;
        gyroDoc["gz"] = gz;
        String gyroJson;
        serializeJson(gyroDoc, gyroJson);
        mqttClient.publish(TOPIC_IMU_GYRO, gyroJson.c_str());
        
        // IMU Orientation
        JsonDocument orientDoc;
        orientDoc["pitch"] = pitch;
        orientDoc["roll"] = roll;
        orientDoc["yaw"] = yaw;
        String orientJson;
        serializeJson(orientDoc, orientJson);
        mqttClient.publish(TOPIC_IMU_ORIENTATION, orientJson.c_str());
        
        // IMU Validity
        String validStr = valid ? "true" : "false";
        mqttClient.publish(TOPIC_VALIDITY_IMU, validStr.c_str());
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
        mqttClient.publish(TOPIC_RELAYS_STATE, json.c_str());
    }
    
    void publishLEDs(bool internal, const char* ioColor) {
        if (!isConnected()) return;
        
        // Internal LED
        String internalStr = internal ? "true" : "false";
        mqttClient.publish(TOPIC_LEDS_INTERNAL, internalStr.c_str());
        
        // IO LED
        mqttClient.publish(TOPIC_LEDS_IO, ioColor);
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
        mqttClient.publish(TOPIC_SENSORS_OPTOS, json.c_str());
        
        // Button
        String buttonStr = button ? "true" : "false";
        mqttClient.publish(TOPIC_SENSORS_BUTTON, buttonStr.c_str());
    }
    
    unsigned long getLastPublishTime() {
        return lastPublishTime;
    }
    
    void setLastPublishTime(unsigned long time) {
        lastPublishTime = time;
    }
};

#endif // MQTT_HANDLER_HPP
