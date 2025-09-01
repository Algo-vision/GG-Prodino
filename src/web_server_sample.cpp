#include <SPI.h>
#include <Ethernet.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// --- NETWORK SETTINGS ---
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ip(192,168,1,198);

// --- HTTP SERVER ---
EthernetServer httpServer(80);

// --- WEBSOCKET SERVER ---
WebSocketsServer wsServer(81);

// --- HARD-CODED LOGIN ---
const char* USERNAME = "admin";
const char* PASSWORD = "1234";

// --- AUTH TOKEN ---
String authToken = "";

// --- DUMMY DATA ---
struct DeviceStatus {
  bool relay1 = false;
  bool relay2 = false;
  float imuX = 0;
  float imuY = 0;
  float imuZ = 0;
  float battery = 100;
} status;

// --- HTML PAGE ---
const char htmlPage[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>Prodino Dashboard</title>
<style>
body { font-family: Arial; padding: 20px; }
#dashboard { display: none; }
button { margin-left: 10px; }
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
  <p>Relay1: <span id="relay1"></span>
     <button onclick="sendCommand('relay1','toggle')">Toggle</button>
  </p>
  <p>Relay2: <span id="relay2"></span>
     <button onclick="sendCommand('relay2','toggle')">Toggle</button>
  </p>
  <p>IMU: <span id="imu"></span></p>
  <p>Battery: <span id="battery"></span></p>
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
      document.getElementById('relay1').innerText = msg.relay1;
      document.getElementById('relay2').innerText = msg.relay2;
      document.getElementById('imu').innerText = msg.imuX.toFixed(2)+','+msg.imuY.toFixed(2)+','+msg.imuZ.toFixed(2);
      document.getElementById('battery').innerText = msg.battery.toFixed(1)+'%';
    }
  };
  setInterval(getStatus,1000);
}
function getStatus(){ if(ws && ws.readyState===1) ws.send(JSON.stringify({type:'get_status', token:token})); }
function sendCommand(cmd,value){ if(ws && ws.readyState===1) ws.send(JSON.stringify({type:'command', cmd:cmd, value:value, token:token})); }
</script>
</body>
</html>
)rawliteral";

// --- HELPER: generate simple token ---
String generateToken() {
  String t = "";
  for(int i=0;i<16;i++) t += char('A'+random(0,26));
  return t;
}

// --- SEND HTTP RESPONSE ---
void sendResponse(EthernetClient &client, int code, const char* content, String type="application/json"){
  client.println("HTTP/1.1 " + String(code) + " OK");
  client.println("Content-Type: "+type);
  client.println("Connection: close");
  client.println();
  client.println(content);
}

// --- WEBSOCKET EVENT HANDLER ---
void wsEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length){
  if(type == WStype_TEXT){
    String msg = String((char*)payload);
    DynamicJsonDocument doc(256);
    deserializeJson(doc,msg);

    String tokenRecv = doc["token"];
    if(tokenRecv != authToken){
      wsServer.disconnect(num);
      return;
    }

    String msgType = doc["type"];
    if(msgType=="get_status"){
      status.imuX += 0.1; status.imuY += 0.2; status.imuZ += 0.3;
      status.battery -= 0.05; if(status.battery<0) status.battery=100;

      DynamicJsonDocument resp(256);
      resp["type"]="status";
      resp["relay1"]=status.relay1;
      resp["relay2"]=status.relay2;
      resp["imuX"]=status.imuX;
      resp["imuY"]=status.imuY;
      resp["imuZ"]=status.imuZ;
      resp["battery"]=status.battery;
      String out; serializeJson(resp,out);
      wsServer.sendTXT(num, out);
    }

    if(msgType=="command"){
      String cmd = doc["cmd"];
      String value = doc["value"];
      Serial.print("Command received: "); Serial.print(cmd); Serial.print(" -> "); Serial.println(value);
      if(cmd=="relay1" && value=="toggle") status.relay1 = !status.relay1;
      if(cmd=="relay2" && value=="toggle") status.relay2 = !status.relay2;
    }
  }
}

// --- SETUP ---
void setup(){
  Serial.begin(115200);
  while(!Serial);

  Ethernet.begin(mac, ip);
  httpServer.begin();
  wsServer.begin();
  wsServer.onEvent(wsEvent);

  Serial.print("HTTP server running at http://");
  Serial.println(Ethernet.localIP());
  Serial.println("WebSocket server running at ws://192.168.1.198:81/");
}

// --- LOOP ---
void loop(){
  // HTTP serving
  EthernetClient client = httpServer.available();
  if(client){
    String req = client.readStringUntil('\r');
    client.flush();

    if(req.startsWith("GET / ")){
      sendResponse(client,200,htmlPage,"text/html");
    } 
    else if(req.startsWith("POST /login")){
      while(client.available()==0);
      String body = client.readString();
      DynamicJsonDocument doc(256);
      deserializeJson(doc,body);
      String user = doc["user"];
      String pass = doc["pass"];

      DynamicJsonDocument resp(128);
      resp["type"]="login_result";
      if(user==USERNAME && pass==PASSWORD){
        authToken = generateToken();
        resp["success"]=true;
        resp["token"]=authToken;
      } else {
        resp["success"]=false;
        resp["token"]="";
      }
      String out; serializeJson(resp,out);
      sendResponse(client,200,out.c_str());
      Serial.println(resp["success"] ? "Client logged in" : "Client login failed");
    }

    delay(1);
    client.stop();
  }

  // WebSocket loop
  wsServer.loop();
}
