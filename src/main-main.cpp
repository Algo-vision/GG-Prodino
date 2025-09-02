

#include "KMPProDinoMKRZero.h"
#include "KMPCommon.h"
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include <i2c_imu_gps.hpp>
#include "gg_hal.hpp"
// OTA support
#include <ArduinoOTA.h>
TinyGPSPlus gps;
char gpsBuffer[GPS_BUFFER_LEN];
#define LED_IO_PIN 6
unsigned long last_user_connected_time = 0;
bool user_connected = false;
bool all_devices_connected = false;
unsigned long led_last_change_time = 0;
bool technician_mode = false;
// If in debug mode - print debug information in Serial. Comment in production code, this bring performance.
// This method is good for development and verification of results. But increases the amount of code and decreases productivity.

// Enter a MAC address and IP address for your controller below.
byte _mac[] = {0x00, 0x08, 0xDC, 0x53, 0x09, 0x72};
// The IP address will be dependent on your local network.
IPAddress _ip(192, 168, 1, 198);
const uint16_t LOCAL_PORT = 80;

EthernetServer _server(LOCAL_PORT);

EthernetClient _client;
GG_HAL _gg_hal;
// --- AUTH TOKEN ---
String authToken = "";

// --- DUMMY DATA ---

struct DeviceStatus
{
  bool relays_status[4] = {false, false, false, false};
  float imuX = 0;
  float imuY = 0;
  float imuZ = 0;
  bool imuValid = false;
  // float battery = 100;
  double gpsLat = 0;
  double gpsLng = 0;
  double gpsAlt = 0;
  bool gpsValid = false;
  bool gpsConnected = false;
  bool ledInternal = false;
  LED_STATES ledIo = OFF;
  bool button1 = false;
} status;

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
  status.imuValid = imu_valid;

  // Read GPS
  double lat, lng, alt;
  bool gpsOk = readGPSCoords(lat, lng, alt);
  status.gpsValid = gpsOk;
  status.gpsConnected = gps_conncted;
  if (gpsOk)
  {
    status.gpsLat = lat;
    status.gpsLng = lng;
    status.gpsAlt = alt;
  }
  status.button1 = _gg_hal.get_button1_state();
  status.ledIo = _gg_hal.get_indicator_led_state();
  all_devices_connected = gps_conncted && imu_valid;
}

void setup()
{
  // Check for technician mode: button1 held for 5 seconds during startup
  unsigned long tech_start = millis();
  bool tech_button_held = false;
  delay(3000);
  Serial.begin(115200);
  while (!Serial)
    ;

  KMPProDinoMKRZero.init(ProDino_MKR_Zero_Ethernet);

  // Start the Ethernet connection and the server.
  Ethernet.begin(_mac, _ip);
  _server.begin();

  _gg_hal.init();
  Serial.println("Starting up...");
  Serial.println("Hold button 1 to enter technician mode...");
  Serial.println("Button state is: ");
  Serial.println(_gg_hal.get_button1_state() ? "PRESSED" : "RELEASED");
  while ((millis() - tech_start) < 5000)
  {
    bool button_state = _gg_hal.get_button1_state();
    Serial.println("Button state is: ");
  Serial.println(button_state? "PRESSED" : "RELEASED");
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
  ArduinoOTA.begin(Ethernet.localIP(), "prodino", "", InternalStorage);
  Serial.println("OTA update enabled. Use Arduino IDE or compatible tool to upload firmware over network.");
  }
  Serial.println(technician_mode ? "Technician mode enabled." : "Normal mode.");
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
  Serial.println("User: " + user + ", Pass: " + pass);
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
  JsonArray relays_status = resp.createNestedArray("relays_status");

  for (uint8_t i = 0; i < RELAY_COUNT; i++)
  {
    relays_status.add(KMPProDinoMKRZero.GetRelayState(i));
  }
  resp["imuX"] = status.imuX;
  resp["imuY"] = status.imuY;
  resp["imuZ"] = status.imuZ;
  resp["gpsLat"] = status.gpsLat;
  resp["gpsLng"] = status.gpsLng;
  resp["gpsAlt"] = status.gpsAlt;
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
  resp["button1"] = status.button1;
  resp["imuValid"] = status.imuValid;
  resp["GPSConnected"] = status.gpsConnected;
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
  Serial.print(" | IMU: ");
  Serial.print(status.imuX, 2);
  Serial.print(", ");
  Serial.print(status.imuY, 2);
  Serial.print(", ");
  Serial.print(status.imuZ, 2);
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
  }
  else
  {
    Serial.print("No fix");
  }
  Serial.print(" | GPS Connected: ");
  Serial.print(status.gpsConnected ? "Yes" : "No");
  Serial.print(" | Button1: ");
  Serial.print(status.button1 ? "Pressed" : "Released");
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

  Serial.println();
}
void http_loop()
{

  EthernetClient client = _server.available();
  if (client)
  {
    String req = client.readStringUntil('\r');
    client.flush();

    if (req.startsWith("POST /"))
    {
      while (client.available() == 0)
        ;

      String request = readHttpRequest(client);
      // Extract body only
      String body = extractHttpBody(request);
      // String body = client.readString();
      Serial.println("Request body: " + body);
      JsonDocument doc;
      Serial.println("Json content: " + body);
      deserializeJson(doc, body.c_str());
      Serial.println("Is Json null: " + String(doc.isNull()));

      String msg_type = doc["type"];
      JsonDocument resp;
      if (msg_type == "login")
      {
        Serial.println("Login type detected");
        resp = handle_login_request(doc);
      }
      if (msg_type == "")
      {
        resp["type"] = "error";
        resp["message"] = "Unknown request type";
      }
      String tokenRecv = doc["token"];
      bool valid_token = (tokenRecv == authToken);
      if (msg_type != "login" && valid_token)
      {
        resp["type"] = "error";
        resp["message"] = "Invalid token";
      }
      if (valid_token)
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
          _gg_hal.set_indicator_led(color_val);
          resp = generate_status_msg(doc);
        }
      }
      String out;
      serializeJson(resp, out);
      sendResponse(client, 200, out);

      Serial.println(resp["success"] ? "Client logged in" : "Client login failed");
    }

    delay(1);
    client.stop();
  }
}

void status_led_blink()
{
  // Technician mode (example: set by a global flag, here as a placeholder)
  extern bool technician_mode;
  if (technician_mode)
  {
    _gg_hal.set_indicator_led(ORANGE);
    return;
  }

  if (all_devices_connected && user_connected)
  {
    // Solid green: client connected and all sensors OK
    _gg_hal.set_indicator_led(GREEN);
  }
  else if (all_devices_connected && !user_connected)
  {
    // Blinking green: all sensors OK, no client
    Serial.println("Last change time: " + String(led_last_change_time));
     Serial.println("Current time: " + String(millis()));
     Serial.println("Time diff: " + String(millis() - led_last_change_time));
     Serial.println("Current LED state: " + String(_gg_hal.get_indicator_led_state() == OFF ? "OFF" : "ON"));
     Serial.println("In blinking green mode");
    if ((millis() - led_last_change_time) > 500)
    {
      Serial.println("Blinking green");
      Serial.println(_gg_hal.get_indicator_led_state() == OFF ? "Currently OFF" : "Currently ON");
      if (_gg_hal.get_indicator_led_state() == OFF)
        _gg_hal.set_indicator_led(GREEN);
      else
        _gg_hal.set_indicator_led(OFF);
      led_last_change_time = millis();
    }
  }
  else if (!all_devices_connected && user_connected)
  {
    // Solid red: client connected, but at least one sensor not OK
    _gg_hal.set_indicator_led(RED);
  }
  else
  {
    // Blinking red: no client, at least one sensor not OK
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
void loop()
{
  if (technician_mode) {
    // Handle OTA updates in technician mode
    ArduinoOTA.handle();
  }
  http_loop();
  update_hw_status();
  // write_status_to_serial();
  if (millis() - last_user_connected_time > 5000 )
  {
    user_connected = false;
  }
  status_led_blink();
  // delay(1000);
}
