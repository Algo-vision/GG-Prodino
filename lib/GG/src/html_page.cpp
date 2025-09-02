#include <api/String.h>

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
