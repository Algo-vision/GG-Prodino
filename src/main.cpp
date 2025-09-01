#include "../include/i2c_imu_gps.hpp"

// Define global GPS objects for linkage
TinyGPSPlus gps;
char gpsBuffer[GPS_BUFFER_LEN];


#include "KMPProDinoMKRZero.h"
#include "KMPCommon.h"
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include "../include/i2c_imu_gps.hpp"

#define LED_IO_PIN 6
// If in debug mode - print debug information in Serial. Comment in production code, this bring performance.
// This method is good for development and verification of results. But increases the amount of code and decreases productivity.

// Enter a MAC address and IP address for your controller below.
byte _mac[] = {0x00, 0x08, 0xDC, 0x53, 0x09, 0x72};
// The IP address will be dependent on your local network.
IPAddress _ip(192, 168, 1, 198);
const uint16_t LOCAL_PORT = 80;
const uint16_t LOCAL_PORT_WS = 81;

EthernetServer _server(LOCAL_PORT);
WebSocketsServer wsServer(LOCAL_PORT_WS);

EthernetClient _client;

// --- AUTH TOKEN ---
String authToken = "";

// --- DUMMY DATA ---

struct DeviceStatus
{
  bool relays_status[4] = {false, false, false, false};
  float imuX = 0;
  float imuY = 0;
  float imuZ = 0;
  float battery = 100;
  double gpsLat = 0;
  double gpsLng = 0;
  bool gpsValid = false;
} status;

// --- HARD-CODED LOGIN ---
const char *USERNAME = "admin";
const char *PASSWORD = "1234";

// --- HTML PAGE ---
const String htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>Prodino Dashboard</title>
<style>
body { font-family: Arial; padding: 20px; }
#dashboard { display: none; }
button { margin-left: 10px; }
    table { border-collapse: collapse; width: 300px; margin: 20px auto; }
    th, td { border: 1px solid #ccc; padding: 8px; text-align: center; }
    .on { background-color: lightgreen; }
    .off { background-color: lightcoral; }
</style>
</head>
<body>
<div id="loginDiv">
  <h2>Login</h2>
  <input id="user" placeholder="Username">
  <input id="pass" placeholder="Password" type="password">
  <button onclick="login()">Login</button>
</div>
<div id="dashboard">
  <h2>Prodino Dashboard</h2>
   <table id="relayTable">
    <thead>
      <tr><th>Relay</th><th>Status</th><th>Action</th></tr>
    </thead>
    <tbody></tbody>
  </table>

  <p>IMU: <span id="imu"></span></p>
  <p>Battery: <span id="battery"></span></p>
  <p>GPS: <span id="gps"></span></p>
  <p>
    <button onclick="sendCommand('led_internal','toggle')">Toggle Internal LED</button>
    <button onclick="sendCommand('led_io','toggle')">Toggle IO LED</button>
    <button onclick="sendCommand('pwm','128')">Set PWM 50%</button>
  </p>
</div>
<script>
let token = "";
let ws;
function login(){
  const user = document.getElementById('user').value;
  const pass = document.getElementById('pass').value;
  fetch('http://192.168.1.198/login',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({user:user, pass:pass})
  })
  .then(r=>r.json())
  .then(d=>{
    if(d.success){
      token = d.token;
      document.getElementById('loginDiv').style.display='none';
      document.getElementById('dashboard').style.display='block';
      openWebSocket();
    } else { alert('Login failed'); }
  })
  .catch(err=>alert("Connection error: "+err));
}
function openWebSocket(){
  ws = new WebSocket('ws://192.168.1.198:81/');
  ws.onopen = ()=>{ getStatus(); };
  ws.onmessage = (evt)=>{
    const msg = JSON.parse(evt.data);
    if(msg.type==="status"){
      document.getElementById('imu').innerText = msg.imuX.toFixed(2)+','+msg.imuY.toFixed(2)+','+msg.imuZ.toFixed(2);
      document.getElementById('battery').innerText = msg.battery.toFixed(1)+'%';
      if(msg.gpsValid){
        document.getElementById('gps').innerText = msg.gpsLat.toFixed(6)+','+msg.gpsLng.toFixed(6);
      } else {
        document.getElementById('gps').innerText = 'No Fix';
      }
      let relays = msg.relays_status;
      updateRelayTable(relays);
    }
  };
  setInterval(getStatus,1000);
}
function getStatus(){ if(ws && ws.readyState===1) ws.send(JSON.stringify({type:'get_status', token:token})); }
function sendCommand(cmd,value){ if(ws && ws.readyState===1) ws.send(JSON.stringify({type:'command', cmd:cmd, value:value, token:token})); }

function updateRelayTable(relays) {
  const tbody = document.querySelector("#relayTable tbody");
  tbody.innerHTML = ""; // clear old
  relays.forEach((state, i) => {
    let row = document.createElement("tr");
    row.className = state ? "on" : "off";

    let colName = document.createElement("td");
    colName.textContent = "Relay " + i;

    let colStatus = document.createElement("td");
    colStatus.textContent = state ? "ON" : "OFF";

    let colAction = document.createElement("td");
    let btn = document.createElement("button");
    btn.textContent = "Toggle";
    btn.onclick = () => {
    sendCommand("toggle_relay", i);
    };
    colAction.appendChild(btn);

    row.appendChild(colName);
    row.appendChild(colStatus);
    row.appendChild(colAction);
    tbody.appendChild(row);
  });
}

</script>
</body>
</html>
)rawliteral";

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

// --- WEBSOCKET EVENT HANDLER ---
void wsEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  if (type == WStype_TEXT)
  {
    String msg = String((char *)payload);
    JsonDocument doc;
    deserializeJson(doc, msg);
    Serial.println("WebSocket message: " + msg);
    String tokenRecv = doc["token"];
    if (tokenRecv != authToken)
    {
      wsServer.disconnect(num);
      return;
    }

    String msgType = doc["type"];
    if (msgType == "get_status")
    {
      // Read IMU
      float ax, ay, az;
      readAccelerometer(ax, ay, az);
      status.imuX = ax;
      status.imuY = ay;
      status.imuZ = az;

      // Read GPS
      double lat, lng;
      bool gpsOk = readGPSCoords(lat, lng);
      status.gpsValid = gpsOk;
      if (gpsOk) {
        status.gpsLat = lat;
        status.gpsLng = lng;
      }

      status.battery -= 0.05;
      if (status.battery < 0)
        status.battery = 100;

      JsonDocument resp;
      resp["type"] = "status";
      JsonArray relays_status = resp["relays_status"].to<JsonArray>();

      for (uint8_t i = 0; i < RELAY_COUNT; i++)
      {
        relays_status.add(KMPProDinoMKRZero.GetRelayState(i));
      }
      resp["imuX"] = status.imuX;
      resp["imuY"] = status.imuY;
      resp["imuZ"] = status.imuZ;
      resp["battery"] = status.battery;
      resp["gpsLat"] = status.gpsLat;
      resp["gpsLng"] = status.gpsLng;
      resp["gpsValid"] = status.gpsValid;
      String out;
      serializeJson(resp, out);
      wsServer.sendTXT(num, out);
    }

    if (msgType == "command")
    {
      String cmd = doc["cmd"];
      if (cmd == "led_internal") // Toggle internal LED
      {
        bool led_state = KMPProDinoMKRZero.GetStatusLed ();
        KMPProDinoMKRZero.SetStatusLed(!led_state);
      }
      else if (cmd == "led_io") // Toggle IO LED
      {
        digitalWrite(LED_IO_PIN, !digitalRead(LED_IO_PIN));
      }
      // else if (cmd == "pwm") // Set PWM value
      // {
      //   int pwmValue = doc["value"];
      //   KMPProDinoMKRZero.SetPWM(pwmValue);
      // }
      else if (cmd == "toggle_relay") // Toggle relay
      {
        uint8_t relay_num = doc["value"];
        if (relay_num < RELAY_COUNT)
        {
          bool newState = !KMPProDinoMKRZero.GetRelayState(relay_num);
          KMPProDinoMKRZero.SetRelayState(relay_num, newState);
          status.relays_status[relay_num] = newState;
          JsonDocument resp;
          resp["type"] = "status";
          JsonArray relays_status = resp.createNestedArray("relays_status");
          for (uint8_t i = 0; i < RELAY_COUNT; i++)
            relays_status.add(status.relays_status[i]);
          resp["imuX"] = status.imuX;
          resp["imuY"] = status.imuY;
          resp["imuZ"] = status.imuZ;
          resp["battery"] = status.battery;
          resp["gpsLat"] = status.gpsLat;
          resp["gpsLng"] = status.gpsLng;
          resp["gpsValid"] = status.gpsValid;
          String out;
          serializeJson(resp, out);
          wsServer.sendTXT(num, out);
        }
      }
    }
  }
}

void setup()
{
  delay(5000);
  Serial.begin(115200);

  // Init Dino board. Set pins, start W5500.
  KMPProDinoMKRZero.init(ProDino_MKR_Zero_Ethernet);

  // Start the Ethernet connection and the server.
  Ethernet.begin(_mac, _ip);
  _server.begin();
  wsServer.begin();
  wsServer.onEvent(wsEvent);

  // Init IMU and I2C
  Wire.begin();
  initIMU();

  Serial.println("The example WebRelay is started.");
  Serial.println("IPs:");
  Serial.println(Ethernet.localIP());
  Serial.println(Ethernet.gatewayIP());
  Serial.println(Ethernet.subnetMask());
}

void loop()
{
  EthernetClient client = _server.available();
  if (client)
  {
    String req = client.readStringUntil('\r');
    client.flush();

    if (req.startsWith("GET / "))
    {
      sendResponse(client, 200, htmlPage, "text/html");
    }
    else if (req.startsWith("POST /login"))
    {
      Serial.println("Login request received");
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
      String user = doc["user"];
      String pass = doc["pass"];

      serializeJson(doc, Serial);
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
      String out;
      serializeJson(resp, out);
      sendResponse(client, 200, out);
      Serial.println(resp["success"] ? "Client logged in" : "Client login failed");
    }

    delay(1);
    client.stop();
  }
  wsServer.loop();
}
