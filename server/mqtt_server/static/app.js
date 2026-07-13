// Prodino IoT Dashboard - Real-time WebSocket Client

// Connect to WebSocket server
const socket = io('http://localhost:5000');

// Connection status
socket.on('connect', () => {
    console.log('Connected to server');
    updateConnectionStatus(true);
});

socket.on('disconnect', () => {
    console.log('Disconnected from server');
    updateConnectionStatus(false);
});

// Receive initial data on connection
socket.on('initial_data', (data) => {
    console.log('Initial data received:', data);
    updateDashboard(data);
});

// Receive real-time updates
socket.on('prodino_update', (update) => {
    console.log('Update received:', update.topic, update.data);
    handleUpdate(update.topic, update.data);
});

// Update connection status indicator
function updateConnectionStatus(connected) {
    const statusIndicator = document.getElementById('connectionStatus');
    const statusDot = statusIndicator.querySelector('.status-dot');
    const statusText = statusIndicator.querySelector('span');

    if (connected) {
        statusDot.classList.remove('offline');
        statusDot.classList.add('online');
        statusText.textContent = 'Connected';
    } else {
        statusDot.classList.remove('online');
        statusDot.classList.add('offline');
        statusText.textContent = 'Disconnected';
    }
}

// Handle incoming MQTT topic updates
function handleUpdate(topic, data) {
    switch (topic) {
        case 'prodino/gps/position':
            updateGPSPosition(data);
            break;
        case 'prodino/gps/velocity':
            updateGPSVelocity(data);
            break;
        case 'prodino/gps/heading':
            updateGPSHeading(data);
            break;
        case 'prodino/validity/gps':
            updateGPSValidity(data);
            break;
        case 'prodino/imu/accel':
            updateAccel(data);
            break;
        case 'prodino/imu/gyro':
            updateGyro(data);
            break;
        case 'prodino/imu/orientation':
            updateOrientation(data);
            break;
        case 'prodino/validity/imu':
            updateIMUValidity(data);
            break;
        case 'prodino/relays/state':
            updateRelays(data);
            break;
        case 'prodino/leds/internal':
            updateLEDInternal(data);
            break;
        case 'prodino/leds/io':
            updateLEDIO(data);
            break;
        case 'prodino/sensors/optos':
            updateOptos(data);
            break;
        case 'prodino/sensors/button_tech':
            updateButton(data);
            break;
    }

    // Update last update time
    updateLastUpdateTime();
}

// Update complete dashboard (initial load)
function updateDashboard(data) {
    // GPS
    if (data.gps && data.gps.position) {
        updateGPSPosition(data.gps.position);
    }
    if (data.gps && data.gps.velocity) {
        updateGPSVelocity(data.gps.velocity);
    }
    if (data.gps && data.gps.heading !== undefined) {
        updateGPSHeading(data.gps.heading);
    }
    if (data.validity && data.validity.gps) {
        updateGPSValidity(data.validity.gps);
    }

    // IMU
    if (data.imu && data.imu.accel) {
        updateAccel(data.imu.accel);
    }
    if (data.imu && data.imu.gyro) {
        updateGyro(data.imu.gyro);
    }
    if (data.imu && data.imu.orientation) {
        updateOrientation(data.imu.orientation);
    }
    if (data.validity && data.validity.imu !== undefined) {
        updateIMUValidity(data.validity.imu);
    }

    // Relays & LEDs
    if (data.relays) {
        updateRelays(data.relays);
    }
    if (data.leds) {
        if (data.leds.internal !== undefined) updateLEDInternal(data.leds.internal);
        if (data.leds.io) updateLEDIO(data.leds.io);
    }

    // Sensors
    if (data.sensors) {
        if (data.sensors.optos) updateOptos(data.sensors.optos);
        if (data.sensors.button_tech !== undefined) updateButton(data.sensors.button_tech);
    }

    updateLastUpdateTime();
}

// GPS Functions
function updateGPSPosition(position) {
    document.getElementById('gpsLat').textContent = position.lat ? position.lat.toFixed(6) + '°' : '--';
    document.getElementById('gpsLng').textContent = position.lng ? position.lng.toFixed(6) + '°' : '--';
    document.getElementById('gpsAlt').textContent = position.alt ? position.alt.toFixed(1) + ' m' : '--';
}

function updateGPSVelocity(velocity) {
    document.getElementById('gpsGroundSpeed').textContent = velocity.ground ? velocity.ground.toFixed(1) + ' km/h' : '--';
}

function updateGPSHeading(heading) {
    document.getElementById('gpsHeading').textContent = typeof heading === 'number' ? heading.toFixed(1) + '°' : heading + '°';
}

function updateGPSValidity(validity) {
    const badge = document.getElementById('gpsValidBadge');
    const connected = document.getElementById('gpsConnected');

    if (validity.valid) {
        badge.textContent = 'GPS Fix';
        badge.classList.remove('invalid');
        badge.classList.add('valid');
    } else {
        badge.textContent = 'No Fix';
        badge.classList.remove('valid');
        badge.classList.add('invalid');
    }

    connected.textContent = validity.connected ? 'Yes' : 'No';
}

// IMU Functions
function updateAccel(accel) {
    document.getElementById('accelX').textContent = accel.x.toFixed(2);
    document.getElementById('accelY').textContent = accel.y.toFixed(2);
    document.getElementById('accelZ').textContent = accel.z.toFixed(2);
}

function updateGyro(gyro) {
    document.getElementById('gyroX').textContent = gyro.gx.toFixed(2);
    document.getElementById('gyroY').textContent = gyro.gy.toFixed(2);
    document.getElementById('gyroZ').textContent = gyro.gz.toFixed(2);
}

function updateOrientation(orientation) {
    // Update values
    document.getElementById('pitch').textContent = orientation.pitch.toFixed(1) + '°';
    document.getElementById('roll').textContent = orientation.roll.toFixed(1) + '°';
    document.getElementById('yaw').textContent = orientation.yaw.toFixed(1) + '°';

    // Update bars (map -90 to 90 degrees to 0-100%)
    const pitchPercent = ((orientation.pitch + 90) / 180) * 100;
    const rollPercent = ((orientation.roll + 90) / 180) * 100;
    const yawPercent = (orientation.yaw / 360) * 100;

    document.getElementById('pitchBar').style.width = Math.max(0, Math.min(100, pitchPercent)) + '%';
    document.getElementById('rollBar').style.width = Math.max(0, Math.min(100, rollPercent)) + '%';
    document.getElementById('yawBar').style.width = Math.max(0, Math.min(100, yawPercent)) + '%';
}

function updateIMUValidity(valid) {
    const badge = document.getElementById('imuValidBadge');
    if (valid) {
        badge.textContent = 'Valid';
        badge.classList.remove('invalid');
        badge.classList.add('valid');
    } else {
        badge.textContent = 'Invalid';
        badge.classList.remove('valid');
        badge.classList.add('invalid');
    }
}

// Relay Functions
function updateRelays(relays) {
    if (Array.isArray(relays)) {
        relays.forEach((state, index) => {
            const relay = document.getElementById(`relay${index}`);
            if (relay) {
                const indicator = relay.querySelector('.relay-indicator');
                if (state) {
                    indicator.classList.remove('off');
                    indicator.classList.add('on');
                } else {
                    indicator.classList.remove('on');
                    indicator.classList.add('off');
                }
            }
        });
    }
}

// LED Functions
function updateLEDInternal(state) {
    const led = document.getElementById('ledInternal');
    if (state) {
        led.classList.remove('off');
        led.classList.add('green');
    } else {
        led.classList.remove('green');
        led.classList.add('off');
    }
}

function updateLEDIO(color) {
    const led = document.getElementById('ledIO');
    led.classList.remove('off', 'green', 'red', 'orange');

    if (typeof color === 'string') {
        led.classList.add(color.toLowerCase());
    } else {
        led.classList.add('off');
    }
}

// Sensor Functions
function updateOptos(optos) {
    if (Array.isArray(optos)) {
        optos.forEach((state, index) => {
            const opto = document.getElementById(`opto${index}`);
            if (opto) {
                opto.textContent = state ? 'ON' : 'OFF';
            }
        });
    }
}

function updateButton(state) {
    const button = document.getElementById('buttonTech');
    if (button) {
        button.textContent = state ? 'Pressed' : 'Released';
    }
}

// Update last update timestamp
function updateLastUpdateTime() {
    const lastUpdate = document.getElementById('lastUpdate');
    const now = new Date();
    lastUpdate.textContent = now.toLocaleTimeString();
}

// Initial data fetch via REST API (fallback)
async function fetchInitialData() {
    try {
        const response = await fetch('http://localhost:5000/api/status');
        const data = await response.json();
        updateDashboard(data);
    } catch (error) {
        console.error('Error fetching initial data:', error);
    }
}

// Fetch initial data on page load
fetchInitialData();
