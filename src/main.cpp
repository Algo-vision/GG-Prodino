#include "KMPProDinoMKRZero.h"
#include "KMPCommon.h"
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include <i2c_imu_gps.hpp>
#include "gg_hal.hpp"
#include "calculations.hpp"
#include <FlashStorage.h> // Include for FlashStorage
#include <Arduino_DebugUtils.h> // Required for NVIC_SystemReset()

// Define a struct to hold the configuration data
struct Config {
  byte controller_ip_bytes[4];
  byte whitelist_ip_bytes[10][4]; // Assuming max 10 whitelist IPs
  int whitelist_count;

  // Constructor to initialize with default values
  Config() : whitelist_count(0) {
    // Default IP: 192.168.1.198
    controller_ip_bytes[0] = 192;
    controller_ip_bytes[1] = 168;
    controller_ip_bytes[2] = 1;
    controller_ip_bytes[3] = 198;

    // Default whitelist IPs: 192.168.1.20, 192.168.1.169
    whitelist_ip_bytes[0][0] = 192; whitelist_ip_bytes[0][1] = 168; whitelist_ip_bytes[0][2] = 1; whitelist_ip_bytes[0][3] = 20;
    whitelist_ip_bytes[1][0] = 192; whitelist_ip_bytes[1][1] = 168; whitelist_ip_bytes[1][2] = 1; whitelist_ip_bytes[1][3] = 169;
    whitelist_count = 2;
  }
};

// Declare a FlashStorage object
FlashStorage(config_store, Config);

IPAddress current_ip; // Initialized by loadConfig()
IPAddress current_whitelist[10]; // C-style array for whitelist
int current_whitelist_count = 0; // Number of IPs in the whitelist

// --- IP CONFIGURATION PERSISTENCE ---
void saveConfig() {
  Config config_data;

  // Save controller IP
  config_data.controller_ip_bytes[0] = current_ip[0];
  config_data.controller_ip_bytes[1] = current_ip[1];
  config_data.controller_ip_bytes[2] = current_ip[2];
  config_data.controller_ip_bytes[3] = current_ip[3];

  // Save whitelist IPs
  config_data.whitelist_count = current_whitelist_count;
  for (int i = 0; i < current_whitelist_count; ++i) {
    config_data.whitelist_ip_bytes[i][0] = current_whitelist[i][0];
    config_data.whitelist_ip_bytes[i][1] = current_whitelist[i][1];
    config_data.whitelist_ip_bytes[i][2] = current_whitelist[i][2];
    config_data.whitelist_ip_bytes[i][3] = current_whitelist[i][3];
  }

  config_store.write(config_data);
  Serial.println("Configuration saved to FlashStorage.");
}

void loadConfig() {
  Config config_data = config_store.read();

  // Check if the loaded IP is all zeros OR if whitelist_count is 0 (indicating uninitialized/empty flash)
  if ((config_data.controller_ip_bytes[0] == 0 &&
       config_data.controller_ip_bytes[1] == 0 &&
       config_data.controller_ip_bytes[2] == 0 &&
       config_data.controller_ip_bytes[3] == 0) ||
      config_data.whitelist_count == 0) { // Added check for empty whitelist
    Serial.println("FlashStorage uninitialized, corrupted, or whitelist empty. Setting default configuration.");
    // Re-initialize config_data with default values using its constructor
    Config default_config; // This calls the constructor with default IPs and whitelist
    config_data = default_config;
    // We need to manually populate the global variables from the default config
    current_ip = IPAddress(default_config.controller_ip_bytes[0], default_config.controller_ip_bytes[1], default_config.controller_ip_bytes[2], default_config.controller_ip_bytes[3]);
    current_whitelist_count = default_config.whitelist_count;
    for (int i = 0; i < current_whitelist_count; ++i) {
        current_whitelist[i] = IPAddress(default_config.whitelist_ip_bytes[i][0], default_config.whitelist_ip_bytes[i][1], default_config.whitelist_ip_bytes[i][2], default_config.whitelist_ip_bytes[i][3]);
    }
    saveConfig(); // Save the default configuration to flash
  } else {
    // Load controller IP
    current_ip = IPAddress(config_data.controller_ip_bytes[0],
                           config_data.controller_ip_bytes[1],
                           config_data.controller_ip_bytes[2],
                           config_data.controller_ip_bytes[3]);

    // Load whitelist IPs
    current_whitelist_count = config_data.whitelist_count;
    for (int i = 0; i < current_whitelist_count; ++i) {
      current_whitelist[i] = IPAddress(config_data.whitelist_ip_bytes[i][0],
                                            config_data.whitelist_ip_bytes[i][1],
                                            config_data.whitelist_ip_bytes[i][2],
                                            config_data.whitelist_ip_bytes[i][3]);
    }
  }
  Serial.println("Configuration loaded from FlashStorage.");
}

// OTA support
#include <ArduinoOTA.h>

#define LED_IO_PIN 6
unsigned long last_user_connected_time = 0;
bool user_connected = false;
bool all_devices_connected = false;
unsigned long led_last_change_time = 0;
unsigned long last_update_time = 0;
float imuX_offset = 0.0;
float imuY_offset = 0.0;
float imuZ_offset = 0.0;
float imuGx_offset = 0.0;
float imuGy_offset = 0.0;
float imuGz_offset = 0.0;
bool technician_mode = false;
bool ota_in_progress = false; // Flag to indicate OTA update is running
const String FIRMWARE_VERSION = "1.2.1"; // Added firmware version constant
// If in debug mode - print debug information in Serial. Comment in production code, this bring performance.
// This method is good for development and verification of results. But increases the amount of code and decreases productivity.

// Enter a MAC address and IP address for your controller below.



// Enter a MAC address and IP address for your controller below.
byte _mac[] = {0x00, 0x08, 0xDC, 0x53, 0x09, 0x72};
// The IP address will be dependent on your local network.

const uint16_t LOCAL_PORT = 80;

EthernetServer _server(LOCAL_PORT);

EthernetClient _client;
GG_HAL _gg_hal;
LED_STATES manual_led_state = OFF; // Stores the manually set LED state
bool manual_led_control_active = false; // Flag to indicate if manual LED control is active
// --- AUTH TOKEN ---
String authToken = "";

// --- IP WHITELIST ---
// WHITELIST is now current_whitelist

bool is_ip_whitelisted(const IPAddress& ip) {
  for (int i = 0; i < current_whitelist_count; ++i) {
    if (ip == current_whitelist[i]) return true;
  }
  return false;
}



// --- DUMMY DATA ---

struct DeviceStatus
{
  bool relays_status[4] = {false, false, false, false};
  bool optos_status[4] = {false, false, false, false};
  float imuX = 0;
  float imuY = 0;
  float imuZ = 0;
  float imuGx = 0;
  float imuGy = 0;
  float imuGz = 0;
  float pitch = 0;
  float roll = 0;
  float yaw = 0; // Added yaw
  bool imuValid = false;
  // float battery = 100;
  double gpsLat = 0;
  double gpsLng = 0;
  double gpsAlt = 0;
  char gpsTime[20] = ""; // YYYY-MM-DD hh:mm:ss
  float gpsSpeedNorth = 0; // km/hr
  float gpsSpeedEast = 0;  // km/hr
  float gpsSpeedDown = 0;  // km/hr
  float gpsGroundSpeed = 0; // km/hr
  float gpsHeading = 0;     // degrees
  bool gpsValid = false;
  bool gpsConnected = false;
  bool ledInternal = false;
  LED_STATES ledIo = OFF;
  bool button_tech = false;
  bool technicianMode = false;

} status;

// Variables for GPS-derived vertical speed
double previous_gps_altitude = 0.0;
unsigned long previous_gps_time = 0;
float filtered_gpsSpeedDown = 0.0; // New variable for filtered speed_down
const float SPEED_DOWN_FILTER_ALPHA = 0.2; // Smoothing factor for low-pass filter (0.0 to 1.0, smaller is more smooth)
// --- HARD-CODED LOGIN ---
const char *USERNAME = "admin";
const char *PASSWORD = "1234";

String generateToken()

{
  String t = "";
  for (int i = 0; i < 16; i++)
    t += char('A' + random(0, 26));
  return t;
}
String readHttpRequest(EthernetClient &client)
{
  String req = "";
  unsigned long timeout = millis();
  while (client.connected() && millis() - timeout < 1000)
  {
    while (client.available())
    {
      char c = client.read();
      req += c;
      timeout = millis();
    }
  }
  return req;
}

String extractHttpBody(const String &req)
{
  int pos = req.indexOf("\r\n\r\n"); // end of headers
  if (pos == -1)
    return "";                   // no body found
  return req.substring(pos + 4); // body starts after \r\n\r\n
}
// --- SEND HTTP RESPONSE ---
void sendResponse(EthernetClient &client, int code, const String content, String type = "application/json")
{
  client.println("HTTP/1.1 " + String(code) + " OK");
  client.println("Content-Type: " + type);
  client.println("Connection: close");
  client.println();
  const size_t CHUNK_SIZE = 128; // keep < Ethernet buffer (2k total, 512 safe per socket)
  for (size_t i = 0; i < content.length(); i += CHUNK_SIZE)
  {
    client.print(content.substring(i, i + CHUNK_SIZE));
    delay(1); // let W5500 flush
  }
}

void update_hw_status()
{
  // Read IMU
  float ax, ay, az;
  bool imu_valid = readAccelerometer(ax, ay, az);
  status.imuX = ax;
  status.imuY = ay;
  status.imuZ = az;

  // Read Gyroscope
  float gx, gy, gz;
  _gg_hal.get_gyro_data(gx, gy, gz);
  status.imuGx = gx - imuGx_offset; // Apply gyroscope offset
  status.imuGy = gy - imuGy_offset; // Apply gyroscope offset
  status.imuGz = gz - imuGz_offset; // Apply gyroscope offset

  status.imuValid = imu_valid; // imuValid should also consider gyroscope data validity.

  // Calculate vertical speed from IMU and Pitch/Roll
  unsigned long current_time = millis();
  float dt = (current_time - last_update_time) / 1000.0; // Time difference in seconds
  last_update_time = current_time;

  if (status.imuValid) {
    calculatePitchRoll(status.pitch, status.roll, status.imuX, status.imuY, status.imuZ, status.imuGx, status.imuGy, dt, imuX_offset, imuY_offset);
    calculateYaw(status.yaw, status.imuGz, dt); // Calculate yaw
  }

  // Read GPS
  gps_data current_gps_data;
  _gg_hal.get_gps_data(current_gps_data);
  status.gpsValid = current_gps_data.valid;
  status.gpsConnected = gps_conncted; // gps_conncted is a global from i2c_imu_gps.cpp
  if (status.gpsValid)
  {
    status.gpsLat = current_gps_data.latitude;
    status.gpsLng = current_gps_data.longitude;
    status.gpsAlt = current_gps_data.altitude; // Use raw GPS altitude

    // Calculate vertical speed from GPS altitude changes
    if (previous_gps_time > 0 && dt > 0.0) {
        // speed_down is positive when going down, so (previous_altitude - current_altitude)
        float vertical_speed_ms = (previous_gps_altitude - current_gps_data.altitude) / dt;
        float current_gpsSpeedDown = vertical_speed_ms * 3.6; // Convert m/s to km/hr
        // Apply EMA filter
        filtered_gpsSpeedDown = (SPEED_DOWN_FILTER_ALPHA * current_gpsSpeedDown) + ((1.0 - SPEED_DOWN_FILTER_ALPHA) * filtered_gpsSpeedDown);
        status.gpsSpeedDown = filtered_gpsSpeedDown;
    } else {
        status.gpsSpeedDown = 0.0; // No previous data or dt is zero
        filtered_gpsSpeedDown = 0.0; // Reset filter if no valid data
    }
    previous_gps_altitude = current_gps_data.altitude;
    previous_gps_time = current_time;

    strcpy(status.gpsTime, current_gps_data.time_str);
    status.gpsSpeedNorth = current_gps_data.speed_north;
    status.gpsSpeedEast = current_gps_data.speed_east;
    status.gpsGroundSpeed = current_gps_data.ground_speed;
    status.gpsHeading = current_gps_data.heading;
  } else {
    // Clear GPS data if not valid
    status.gpsLat = 0;
    status.gpsLng = 0;
    status.gpsAlt = 0; // Clear altitude if GPS is not valid
    strcpy(status.gpsTime, "");
    status.gpsSpeedNorth = 0;
    status.gpsSpeedEast = 0;
    status.gpsSpeedDown = 0; // Reset vertical speed if GPS is not valid
    status.gpsGroundSpeed = 0;
    status.gpsHeading = 0;
    previous_gps_altitude = 0.0; // Reset previous altitude
    previous_gps_time = 0;       // Reset previous time
  }
  status.button_tech = _gg_hal.get_button_tech_state();
  status.ledIo = _gg_hal.get_indicator_led_state();
  all_devices_connected = gps_conncted && imu_valid;
  for (uint8_t i = 0; i < RELAY_COUNT; i++)
  {
    status.relays_status[i] = KMPProDinoMKRZero.GetRelayState(i);
  }
  for (uint8_t i = 0; i < OPTOIN_COUNT; i++)
  {
    status.optos_status[i] = _gg_hal.get_optoin_state(i);
  }
  Serial.println("Technician mode status in update_hw_status: " + String(technician_mode ? "true" : "false"));
  if(technician_mode)
  {
    status.technicianMode = true;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(60000);

  // while (!Serial);

  loadConfig(); // Load configuration from LittleFS
  KMPProDinoMKRZero.init(ProDino_MKR_Zero_Ethernet);
  // Start the Ethernet connection and the server.
  Ethernet.begin(_mac, current_ip);
  _server.begin();
  _gg_hal.init();
  // Check for technician mode: button_tech held for 5 seconds during startup
  unsigned long tech_start = millis();
  bool tech_button_held = false;
  Serial.println("Hold button 1 to enter technician mode...");
  Serial.println("Button state is: ");
  Serial.println(_gg_hal.get_button_tech_state() ? "PRESSED" : "RELEASED");
  while ((millis() - tech_start) < 5000)
  {
    bool button_state = _gg_hal.get_button_tech_state();
    Serial.println("Button state is: ");
    Serial.println(button_state ? "PRESSED" : "RELEASED");
    if (button_state)
    {
      tech_button_held = true;
    }
    else
    {
      tech_button_held = false;
      break;
    }
    delay(10);
  }
  if (tech_button_held)
  {
    technician_mode = true;
    // Initialize OTA only in technician mode
    ArduinoOTA.onStart([]() {
      ota_in_progress = true;
      Serial.println("OTA update started.");
    });
    // Note: onEnd is not supported by this library version, but the process reboots on success anyway.
    ArduinoOTA.onError([](int error, const char* msg) {
      ota_in_progress = false;
      Serial.print("OTA Error[");
      Serial.print(error);
      Serial.print("]: ");
      Serial.println(msg);
    });
    ArduinoOTA.begin(Ethernet.localIP(), "grk", "", InternalStorage);
    Serial.println("OTA update enabled. Use Arduino IDE or compatible tool to upload firmware over network.");
  }
  Serial.println(technician_mode  ? "Technician mode enabled." : "Normal mode.");



  // Calibrate IMU by taking 100 readings and averaging them
  Serial.println("Calibrating IMU... Keep the device flat and still.");
  float ax_sum = 0.0;
  float ay_sum = 0.0;
  float az_sum = 0.0;
  for (int i = 0; i < 100; i++) {
    float ax, ay, az;
    if (readAccelerometer(ax, ay, az)) {
      ax_sum += ax;
      ay_sum += ay;
      az_sum += az;
    }
    delay(10);
  }
  imuX_offset = ax_sum / 100.0;
  imuY_offset = ay_sum / 100.0;
  imuZ_offset = az_sum / 100.0;
  Serial.println("Accelerometer calibration complete.");
  Serial.print("Accel Offsets: X="); Serial.print(imuX_offset);
  Serial.print(", Y="); Serial.print(imuY_offset);
  Serial.print(", Z="); Serial.println(imuZ_offset);

  // Calibrate Gyroscope by taking 100 readings and averaging them
  Serial.println("Calibrating Gyroscope... Keep the device flat and still.");
  float gx_sum = 0.0;
  float gy_sum = 0.0;
  float gz_sum = 0.0;
  for (int i = 0; i < 100; i++) {
    float gx, gy, gz;
    _gg_hal.get_gyro_data(gx, gy, gz); // Assuming this reads raw gyro data
    gx_sum += gx;
    gy_sum += gy;
    gz_sum += gz;
    delay(10);
  }
  imuGx_offset = gx_sum / 100.0;
  imuGy_offset = gy_sum / 100.0;
  imuGz_offset = gz_sum / 100.0;
  Serial.println("Gyroscope calibration complete.");
  Serial.print("Gyro Offsets: X="); Serial.print(imuGx_offset);
  Serial.print(", Y="); Serial.print(imuGy_offset);
  Serial.print(", Z="); Serial.println(imuGz_offset);

  last_update_time = millis();
  filtered_gpsSpeedDown = 0.0; // Initialize filtered vertical speed


  Serial.println("Starting up...");
  Serial.println("The example WebRelay is started.");
  Serial.println("IPs:");
  Serial.println(Ethernet.localIP());
  Serial.println(Ethernet.gatewayIP());
  Serial.println(Ethernet.subnetMask());
}

JsonDocument handle_login_request(JsonDocument &doc)
{
  String user = doc["user"];
  String pass = doc["pass"];
  Serial.println();
  Serial.println("handle_login_request: Received User: " + user + ", Pass: " + pass);
  Serial.println("handle_login_request: Expected User: " + String(USERNAME) + ", Pass: " + String(PASSWORD));
  JsonDocument resp;
  resp["type"] = "login_result";
  if (user == USERNAME && pass == PASSWORD)
  {
    authToken = generateToken();
    resp["success"] = true;
    resp["token"] = authToken;
  }
  else
  {
    resp["success"] = false;
    resp["token"] = "";
  }
  return resp;
}
JsonDocument generate_status_msg(JsonDocument &doc)
{
  update_hw_status();
  JsonDocument resp;
  resp["type"] = "status";
  resp["firmwareVersion"] = FIRMWARE_VERSION; // Include firmware version
  JsonArray relays_status = resp["relays_status"].to<JsonArray>();

  for (uint8_t i = 0; i < RELAY_COUNT; i++)
  {
    relays_status.add(KMPProDinoMKRZero.GetRelayState(i));
  }
  resp["imuX"] = status.imuX;
  resp["imuY"] = status.imuY;
  resp["imuZ"] = status.imuZ;
  resp["imuGx"] = status.imuGx;
  resp["imuGy"] = status.imuGy;
  resp["imuGz"] = status.imuGz;
  resp["pitch"] = status.pitch; // Include pitch
  resp["roll"] = status.roll;   // Include roll
  resp["yaw"] = status.yaw;     // Include yaw
  resp["gpsLat"] = status.gpsLat;
  resp["gpsLng"] = status.gpsLng;
  resp["gpsAlt"] = status.gpsAlt;
  resp["gpsTime"] = status.gpsTime;
  resp["gpsSpeedNorth"] = status.gpsSpeedNorth;
  resp["gpsSpeedEast"] = status.gpsSpeedEast;
  resp["gpsSpeedDown"] = status.gpsSpeedDown;
  resp["gpsGroundSpeed"] = status.gpsGroundSpeed;
  resp["gpsHeading"] = status.gpsHeading;
  resp["ledInternal"] = status.ledInternal;
  if (status.ledIo == OFF)
    resp["ledIo"] = "OFF";
  else if (status.ledIo == GREEN)
    resp["ledIo"] = "GREEN";
  else if (status.ledIo == RED)
    resp["ledIo"] = "RED";
  else if (status.ledIo == ORANGE)
    resp["ledIo"] = "ORANGE";
  resp["gpsValid"] = status.gpsValid;
  resp["button_tech"] = status.button_tech;
  resp["imuValid"] = status.imuValid;
  resp["GPSConnected"] = status.gpsConnected;
  resp["technicianMode"] = status.technicianMode;
  JsonArray optoin_status = resp["optoin_status"].to<JsonArray>();

  for (uint8_t i = 0; i < OPTOIN_COUNT; i++)
  {
    optoin_status.add( status.optos_status[i]);
  }
  
  // Add current IP configuration
  resp["controllerIp"] = current_ip.toString();
  JsonArray whitelist_ips_json = resp["whitelistIps"].to<JsonArray>();
  for (int i = 0; i < current_whitelist_count; ++i) {
    whitelist_ips_json.add(current_whitelist[i].toString());
  }

  return resp;
}

void write_status_to_serial()
{
  Serial.print("Relays: ");
  for (uint8_t i = 0; i < RELAY_COUNT; i++)
  {
    Serial.print(KMPProDinoMKRZero.GetRelayState(i) ? "1" : "0");
    if (i < RELAY_COUNT - 1)
      Serial.print(", ");
  }
  Serial.print("OptoIn: ");
  for (uint8_t i = 0; i < RELAY_COUNT; i++)
  {
    Serial.print(status.optos_status[i] ? "1" : "0");
    if (i < RELAY_COUNT - 1)
      Serial.print(", ");
  }
  Serial.print(" | IMU Accel: ");
  Serial.print(status.imuX, 2);
  Serial.print(", ");
  Serial.print(status.imuY, 2);
  Serial.print(", ");
  Serial.print(status.imuZ, 2);
  Serial.print(" | IMU Gyro: ");
  Serial.print(status.imuGx, 2);
  Serial.print(", ");
  Serial.print(status.imuGy, 2);
  Serial.print(", ");
  Serial.print(status.imuGz, 2);
  Serial.print(" | Pitch: ");
  Serial.print(status.pitch, 2);
  Serial.print(" | Roll: ");
  Serial.print(status.roll, 2);
  Serial.print(" | Yaw: ");
  Serial.print(status.yaw, 2);
  Serial.print(" | IMU Valid: ");
  Serial.print(status.imuValid ? "Yes" : "No");
  Serial.print(" | GPS: ");
  if (status.gpsValid)
  {
    Serial.print(status.gpsLat, 6);
    Serial.print(", ");
    Serial.print(status.gpsLng, 6);
    Serial.print(", ");
    Serial.print(status.gpsAlt, 2);
    Serial.print(" | Time: ");
    Serial.print(status.gpsTime);
    Serial.print(" | Spd N/E/D: ");
    Serial.print(status.gpsSpeedNorth, 2);
    Serial.print(", ");
    Serial.print(status.gpsSpeedEast, 2);
    Serial.print(", ");
    Serial.print(status.gpsSpeedDown, 2);
    Serial.print(" | Gnd Spd: ");
    Serial.print(status.gpsGroundSpeed, 2);
    Serial.print(" | Heading: ");
    Serial.print(status.gpsHeading, 2);
  }
  else
  {
    Serial.print("No fix");
  }
  Serial.print(" | GPS Connected: ");
  Serial.print(status.gpsConnected ? "Yes" : "No");
  Serial.print(" | button_tech: ");
  Serial.print(status.button_tech ? "Pressed" : "Released");
  Serial.print(" | LED Internal: ");
  Serial.print(status.ledInternal ? "ON" : "OFF");
  Serial.print(" | LED IO: ");
  if (status.ledIo == OFF)
    Serial.print("OFF");
  else if (status.ledIo == GREEN)
    Serial.print("GREEN");
  else if (status.ledIo == RED)
    Serial.print("RED");
  else if (status.ledIo == ORANGE)
    Serial.print("ORANGE");

  Serial.print(" | IP: ");
  Serial.println(Ethernet.localIP());

  Serial.println();
}
void http_loop()
{

  EthernetClient client = _server.available();
  if (client)
  {
    IPAddress remote_ip = client.remoteIP();
    if (!is_ip_whitelisted(remote_ip)) {
      // Reject connection if not whitelisted
      String out = "{\"type\":\"error\",\"message\":\"IP not allowed\"}";
      sendResponse(client, 403, out);
      delay(1);
      client.stop();
      return;
    }

    String req = client.readStringUntil('\r');
    client.flush();

    if (req.startsWith("POST /"))
    {
      while (client.available() == 0)
        ;

      String request = readHttpRequest(client);
      // Extract body only
      String body = extractHttpBody(request);
      Serial.println("Request body: " + body);
      JsonDocument doc;
      Serial.println("Json content: " + body);
      deserializeJson(doc, body.c_str());
      Serial.println("Is Json null: " + String(doc.isNull()));

      String msg_type = doc["type"];
      JsonDocument resp;
      int http_status_code = 200; // Default to 200 OK

      if (msg_type == "login")
      {
        Serial.println("Login type detected");
        resp = handle_login_request(doc);
      }
      else // Not a login request, token is required
      {
        String tokenRecv = doc["token"];
        bool valid_token = (tokenRecv == authToken && authToken != "");

        if (!valid_token)
        {
          resp["type"] = "error";
          resp["message"] = "Invalid token";
          http_status_code = 401; // Set status to 401 Unauthorized
        }
        else // Token is valid
        {
          last_user_connected_time = millis();
          user_connected = true;
          if (msg_type == "get_status")
          {
            resp = generate_status_msg(doc);
          }
          else if (msg_type == "set_relay")
          {
            uint8_t relay_id = doc["relay_id"];
            bool state = doc["state"];
            if (relay_id < RELAY_COUNT)
            {
              KMPProDinoMKRZero.SetRelayState(relay_id, state);
              resp = generate_status_msg(doc);
            }
            else
            {
              resp["type"] = "error";
              resp["message"] = "Invalid relay number";
            }
          }
          else if (msg_type == "set_internal_led")
          {
            bool state = doc["state"];
            KMPProDinoMKRZero.SetStatusLed(state);
            status.ledInternal = state;
            resp = generate_status_msg(doc);
          }
          else if (msg_type == "set_io_led")
          {
            String color = doc["color"];
            LED_STATES color_val;
            if (color == "OFF")
            {
              color_val = OFF;
            }
            else if (color == "GREEN")
            {
              color_val = GREEN;
            }
            else if (color == "RED")
            {
              color_val = RED;
            }
            else if (color == "ORANGE")
            {
              color_val = ORANGE;
            }
            else
            {
              resp["type"] = "error";
              resp["message"] = "Invalid LED color";
            }
            manual_led_control_active = true; // Activate manual control
            manual_led_state = color_val;     // Store the desired state
            _gg_hal.set_indicator_led(manual_led_state); // Apply the manual setting
            resp = generate_status_msg(doc);
          }
          else if (msg_type == "set_ip_config")
          {
            String controller_ip_str = doc["controller_ip"];
            IPAddress new_controller_ip; // Use a temporary variable
            bool ip_valid = new_controller_ip.fromString(controller_ip_str);

            JsonArray whitelist_ips_json = doc["whitelist_ips"];
            IPAddress new_whitelist[10];
            int new_whitelist_count = 0;
            bool whitelist_valid = true;
            for (JsonVariant ip_str_variant : whitelist_ips_json) {
              IPAddress whitelist_ip;
              if (new_whitelist_count < 10 && whitelist_ip.fromString(ip_str_variant.as<String>())) {
                new_whitelist[new_whitelist_count++] = whitelist_ip;
              } else {
                whitelist_valid = false;
                break;
              }
            }

            if (ip_valid && whitelist_valid) {
              // Update global variables
              current_ip = new_controller_ip;
              current_whitelist_count = new_whitelist_count;
              for(int i=0; i < new_whitelist_count; ++i) {
                current_whitelist[i] = new_whitelist[i];
              }

              saveConfig(); // Save the new configuration to FlashStorage

              resp["success"] = true;
              resp["message"] = "IP configuration updated. Board will reboot.";
              
              // Send response immediately before rebooting
              String out;
              serializeJson(resp, out);
              sendResponse(client, 200, out);
              client.stop(); // Close the client connection

              Serial.println("IP configuration saved. Initiating reboot in 2 seconds...");
              delay(2000); // Give client time to receive response
              NVIC_SystemReset(); // Perform software reset
            } else {
              resp["type"] = "error";
              resp["message"] = "Invalid IP address or whitelist entry provided.";
            }
          }
          else if (msg_type == "reset_led_control")
          {
            manual_led_control_active = false; // Deactivate manual control
            _gg_hal.set_indicator_led(OFF); // Turn off LED immediately or revert to last auto state
            resp = generate_status_msg(doc);
          }
          else
          {
            resp["type"] = "error";
            resp["message"] = "Unknown request type";
          }
        }
      }
      String out;
      serializeJson(resp, out);
      sendResponse(client, http_status_code, out);

      if (msg_type == "login") {
        Serial.println(resp["success"] ? "Client logged in" : "Client login failed");
      }
    }

    delay(1);
    client.stop();
  }
}

void status_led_blink()
{
  if (!manual_led_control_active) { // Only run automatic blinking if manual control is not active
    bool is_safe_state = status.optos_status[0] && status.optos_status[1];
    bool imu_connected = status.imuValid; // Assuming imuValid implies IMU connected
    bool gps_connected = status.gpsConnected; // Assuming gpsConnected implies GPS connected
    bool all_sensors_connected = imu_connected && gps_connected;

    if (technician_mode)
    {
      if (is_safe_state)
      {
        // Solid Orange: Technician mode, voltage to optocouplers (safe)
        _gg_hal.set_indicator_led(ORANGE);
      }
      else
      {
        // Blinking Orange: Technician mode, no voltage to optocouplers (unsafe)
        if ((millis() - led_last_change_time) > 500)
        {
          if (_gg_hal.get_indicator_led_state() == OFF)
            _gg_hal.set_indicator_led(ORANGE);
          else
            _gg_hal.set_indicator_led(OFF);
          led_last_change_time = millis();
        }
      }
    }
    else // Normal mode
    {
      if (is_safe_state)
      {
        if (all_sensors_connected)
        {
          // Solid Green: GPS & IMU connected, voltage to optocouplers (safe)
          _gg_hal.set_indicator_led(GREEN);
        }
        else
        {
          // Solid Red: IMU disconnected (or GPS), voltage to optocouplers (safe)
          _gg_hal.set_indicator_led(RED);
        }
      }
      else // Unsafe state (no voltage to optocouplers)
      {
        if (all_sensors_connected)
        {
          // Blinking Green: GPS & IMU connected, no voltage to optocouplers (unsafe)
          if ((millis() - led_last_change_time) > 500)
          {
            if (_gg_hal.get_indicator_led_state() == OFF)
              _gg_hal.set_indicator_led(GREEN);
            else
              _gg_hal.set_indicator_led(OFF);
            led_last_change_time = millis();
          }
        }
        else
        {
          // Blinking Red: IMU disconnected (or GPS), no voltage to optocouplers (unsafe)
          if ((millis() - led_last_change_time) > 500)
          {
            if (_gg_hal.get_indicator_led_state() == OFF)
              _gg_hal.set_indicator_led(RED);
            else
              _gg_hal.set_indicator_led(OFF);
            led_last_change_time = millis();
          }
        }
      }
    }
  }
}
void loop()
{
  if (technician_mode) {
    // Handle OTA updates in technician mode
    ArduinoOTA.handle();
  }
  http_loop();
  update_hw_status();
  write_status_to_serial();
  if (millis() - last_user_connected_time > 5000 )
  {
    user_connected = false;
  }
  status_led_blink();
  // delay(1000);
}
