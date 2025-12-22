const backendUrl = `${window.location.protocol}//${window.location.hostname}:5555`;
const socket = io(backendUrl);

// DOM Elements
const mqttStatus = document.getElementById('mqtt-status');
const deviceStatus = document.getElementById('device-status');
const lastSeenText = document.getElementById('last-seen');
const logContainer = document.getElementById('log-container');

// GPS Elements
const gpsLat = document.getElementById('gps-lat');
const gpsLng = document.getElementById('gps-lng');
const gpsAlt = document.getElementById('gps-alt');
const gpsSpeed = document.getElementById('gps-speed');
const gpsHeading = document.getElementById('gps-heading');
const gpsValid = document.getElementById('gps-valid');

// IMU Elements
const pitchVal = document.getElementById('pitch-val');
const rollVal = document.getElementById('roll-val');
const yawVal = document.getElementById('yaw-val');
const imuValid = document.getElementById('imu-valid');

// Raw Elements
const accelRaw = document.getElementById('accel-raw');
const gyroRaw = document.getElementById('gyro-raw');
const btnTech = document.getElementById('btn-tech');

// Map Elements
const mapStatus = document.getElementById('map-status');
let map, marker, pathLine;
let pathCoordinates = [];
const DEBUG_TEL_AVIV = false; // Set to true for debugging

// Initialize Map
function initMap() {
    try {
        map = L.map('map').setView([0, 0], 2);
        L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
            attribution: '&copy; OpenStreetMap contributors'
        }).addTo(map);

        marker = L.marker([0, 0]).addTo(map);
        pathLine = L.polyline([], { color: 'var(--accent-primary)', weight: 3 }).addTo(map);

        mapStatus.textContent = 'READY';
        mapStatus.classList.add('on');
        addLog('Map', 'Leaflet initialized successfully', 'accent-secondary');
    } catch (error) {
        console.error('Map initialization failed:', error);
        mapStatus.textContent = 'ERROR';
        mapStatus.classList.add('danger');
        addLog('Map', 'Initialization failed', 'danger');
    }
}

// Call initMap on load
window.addEventListener('load', initMap);

// Socket Events
socket.on('connect', () => {
    mqttStatus.classList.add('active');
    addLog('System', 'Connected to Mission Control Server', 'accent-secondary');
});

socket.on('disconnect', () => {
    mqttStatus.classList.remove('active');
    deviceStatus.classList.remove('active');
    addLog('System', 'Disconnected from Server', 'danger');
});

socket.on('device_update', (state) => {
    updateUI(state);
});

function updateUI(state) {
    // Connection Status
    if (state.connected) {
        deviceStatus.classList.add('active');
        const secondsAgo = Math.floor((Date.now() - state.lastSeen) / 1000);
        lastSeenText.textContent = `LAST SEEN: ${secondsAgo}S AGO`;

        if (secondsAgo > 5) deviceStatus.classList.remove('active');
    }

    // GPS
    let lat = state.gps.lat;
    let lng = state.gps.lng;

    if (DEBUG_TEL_AVIV) {
        lat = 32.0853;
        lng = 34.7818;
    }

    gpsLat.textContent = lat.toFixed(6);
    gpsLng.textContent = lng.toFixed(6);
    gpsAlt.textContent = `${state.gps.alt.toFixed(1)} m`;
    gpsSpeed.textContent = `${state.gps.speed.toFixed(1)} km/h`;
    gpsHeading.textContent = `${state.gps.heading.toFixed(1)}°`;

    if (state.gps.valid || DEBUG_TEL_AVIV) {
        gpsValid.textContent = DEBUG_TEL_AVIV ? 'DEBUG: TEL AVIV' : 'FIX ACQUIRED';
        gpsValid.classList.add('on');

        // Update Map
        const newPos = [lat, lng];
        if (marker) marker.setLatLng(newPos);
        if (pathLine) {
            pathCoordinates.push(newPos);
            pathLine.setLatLngs(pathCoordinates);
        }
        if (map && !map.getBounds().contains(newPos)) {
            map.panTo(newPos);
        }
    } else {
        gpsValid.textContent = 'NO FIX';
        gpsValid.classList.remove('on');
    }

    // IMU
    const { pitch, roll, yaw } = state.imu.orientation;
    pitchVal.textContent = `${pitch.toFixed(1)}°`;
    rollVal.textContent = `${roll.toFixed(1)}°`;
    yawVal.textContent = `${yaw.toFixed(1)}°`;

    if (state.imu.valid) {
        imuValid.textContent = 'STABLE';
        imuValid.classList.add('on');
    } else {
        imuValid.textContent = 'INVALID';
        imuValid.classList.remove('on');
    }

    // Raw Telemetry
    accelRaw.textContent = `${state.imu.accel.x.toFixed(2)} / ${state.imu.accel.y.toFixed(2)} / ${state.imu.accel.z.toFixed(2)}`;
    gyroRaw.textContent = `${state.imu.gyro.x.toFixed(2)} / ${state.imu.gyro.y.toFixed(2)} / ${state.imu.gyro.z.toFixed(2)}`;

    // Optos
    state.sensors.optos.forEach((val, i) => {
        const el = document.getElementById(`opto-${i}`);
        if (val) el.classList.add('on');
        else el.classList.remove('on');
    });

    // Button
    btnTech.textContent = state.sensors.button ? 'PRESSED' : 'RELEASED';
    if (state.sensors.button) btnTech.classList.add('on');
    else btnTech.classList.remove('on');

    // Relays
    state.relays.forEach((val, i) => {
        const el = document.getElementById(`relay-${i}`);
        el.textContent = val ? 'ACTIVE' : 'OFF';
        if (val) el.classList.add('on');
        else el.classList.remove('on');
    });

    // LEDs
    const ledInt = document.getElementById('led-int');
    ledInt.textContent = state.leds.internal ? 'ON' : 'OFF';
    if (state.leds.internal) ledInt.classList.add('on');
    else ledInt.classList.remove('on');

    const ledIo = document.getElementById('led-io');
    ledIo.textContent = state.leds.io;
    if (state.leds.io !== 'OFF') ledIo.classList.add('on');
    else ledIo.classList.remove('on');
}

function addLog(source, message, type = '') {
    const time = new Date().toLocaleTimeString([], { hour12: false, hour: '2-digit', minute: '2-digit', second: '2-digit' });
    const div = document.createElement('div');
    div.style.marginBottom = '4px';
    div.innerHTML = `<span style="color: #475569">[${time}]</span> <span style="color: var(--accent-primary)">[${source.toUpperCase()}]</span> <span class="${type}">${message}</span>`;
    logContainer.prepend(div);

    if (logContainer.childNodes.length > 50) {
        logContainer.removeChild(logContainer.lastChild);
    }
}

// Initial Log
addLog('System', 'Mission Control UI Loaded', 'accent-secondary');
