const backendUrl = `${window.location.protocol}//${window.location.hostname}:5555`;
let socket = null; // Initialize after auth check

// Auth State
let currentUser = null;
let allowedDevices = null; // null = all devices, array = restricted

// State Management
let devices = new Map(); // Key: serialNumber, Value: deviceState
let selectedDevice = null; // Currently selected device serial number
let deviceMarkers = new Map(); // Key: serialNumber, Value: Leaflet marker

// DOM Elements
const mqttStatus = document.getElementById('mqtt-status');
const deviceCountEl = document.getElementById('device-count');
const deviceTotalEl = document.getElementById('device-total');
const lastSeenText = document.getElementById('last-seen');
const logContainer = document.getElementById('log-container');
const deviceGrid = document.getElementById('device-grid');
const fleetOverview = document.getElementById('fleet-overview');
const dashboardGrid = document.getElementById('dashboard-grid');
const selectedDeviceBanner = document.getElementById('selected-device-banner');
const selectedDeviceSN = document.getElementById('selected-device-sn');
const selectedDeviceStatus = document.getElementById('selected-device-status');
const btnBackToFleet = document.getElementById('btn-back-to-fleet');

// GPS Elements
const gpsLat = document.getElementById('gps-lat');
const gpsLng = document.getElementById('gps-lng');
const gpsAlt = document.getElementById('gps-alt');
const gpsSpeed = document.getElementById('gps-speed');
const gpsVelocity = document.getElementById('gps-velocity');
const gpsHeading = document.getElementById('gps-heading');
const gpsValid = document.getElementById('gps-valid');
const gpsConnected = document.getElementById('gps-connected');
const gpsSatellites = document.getElementById('gps-satellites');
const gpsHAcc = document.getElementById('gps-hacc');
const gpsVAcc = document.getElementById('gps-vacc');
const gpsAltEllipsoid = document.getElementById('gps-alt-ellipsoid');
const gpsTime = document.getElementById('gps-time');

// Device Info Elements
const deviceSerial = document.getElementById('device-serial');
const motorWorkHours = document.getElementById('motor-work-hours');
const firmwareVersion = document.getElementById('firmware-version');
const controllerIp = document.getElementById('controller-ip');
const routerIp = document.getElementById('router-ip');
const busVoltage = document.getElementById('bus-voltage');

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
let map;
let pathLines = new Map(); // Key: serialNumber, Value: polyline
const DEBUG_TEL_AVIV = false;

// Colors for different devices on map
const deviceColors = ['#4f46e5', '#06b6d4', '#10b981', '#f59e0b', '#ef4444', '#8b5cf6'];

// Initialize Map
function initMap() {
    try {
        map = L.map('map').setView([32.0, 34.8], 8); // Default view: Israel
        L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
            attribution: '&copy; OpenStreetMap contributors'
        }).addTo(map);

        mapStatus.textContent = 'READY';
        mapStatus.classList.add('on');
        addLog('Map', 'Leaflet initialized for multi-device tracking', 'accent-secondary');
    } catch (error) {
        console.error('Map initialization failed:', error);
        mapStatus.textContent = 'ERROR';
        mapStatus.classList.add('danger');
        addLog('Map', 'Initialization failed', 'danger');
    }
}

// Get color for device (cycle through colors)
function getDeviceColor(index) {
    return deviceColors[index % deviceColors.length];
}

// Create or update device marker on map
function updateDeviceMarker(serialNumber, lat, lng, isValid) {
    if (!map || !isValid) return;

    const deviceIndex = Array.from(devices.keys()).indexOf(serialNumber);
    const color = getDeviceColor(deviceIndex);

    if (deviceMarkers.has(serialNumber)) {
        deviceMarkers.get(serialNumber).setLatLng([lat, lng]);
    } else {
        // Create custom icon
        const icon = L.divIcon({
            className: 'device-map-marker',
            html: `<div style="background: ${color}; width: 12px; height: 12px; border-radius: 50%; border: 2px solid white; box-shadow: 0 2px 6px rgba(0,0,0,0.3);"></div>`,
            iconSize: [16, 16],
            iconAnchor: [8, 8]
        });

        const marker = L.marker([lat, lng], { icon }).addTo(map);
        marker.bindPopup(`<strong>${serialNumber}</strong>`);
        deviceMarkers.set(serialNumber, marker);

        addLog('Map', `Added marker for ${serialNumber}`, 'accent-secondary');
    }

    // Update path line
    if (!pathLines.has(serialNumber)) {
        const polyline = L.polyline([], { color: color, weight: 2, opacity: 0.7 }).addTo(map);
        pathLines.set(serialNumber, { line: polyline, coords: [] });
    }

    const pathData = pathLines.get(serialNumber);
    pathData.coords.push([lat, lng]);
    if (pathData.coords.length > 100) pathData.coords.shift(); // Limit path length
    pathData.line.setLatLngs(pathData.coords);
}

// Render device grid
function renderDeviceGrid(deviceList) {
    if (!deviceList || deviceList.length === 0) {
        deviceGrid.innerHTML = `
            <div class="device-card placeholder">
                <div class="device-card-icon">📡</div>
                <p>Waiting for devices...</p>
            </div>
        `;
        return;
    }

    const isAdmin = currentUser && currentUser.role === 'admin';

    deviceGrid.innerHTML = deviceList.map(device => {
        const isSelected = device.serialNumber === selectedDevice;
        const lastSeenStr = device.lastSeen
            ? `${Math.floor((Date.now() - device.lastSeen) / 1000)}s ago`
            : 'Never';

        // Remove button only for admins
        const removeBtn = isAdmin ? `
            <button class="btn-remove-device" onclick="event.stopPropagation(); removeDevice('${device.serialNumber}')" title="Remove device from fleet">
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <line x1="18" y1="6" x2="6" y2="18"/>
                    <line x1="6" y1="6" x2="18" y2="18"/>
                </svg>
            </button>
        ` : '';

        return `
            <div class="device-card ${device.status} ${isSelected ? 'selected' : ''}" 
                 data-sn="${device.serialNumber}" 
                 onclick="selectDevice('${device.serialNumber}')">
                ${removeBtn}
                <div class="device-card-header">
                    <span class="device-card-sn">${device.serialNumber}</span>
                    <span class="device-card-status ${device.status}">${device.status.toUpperCase()}</span>
                </div>
                <div class="device-card-sensors">
                    <span class="sensor-badge ${device.gpsValid ? 'valid' : 'invalid'}">
                        ${device.gpsValid ? '✓' : '✗'} GPS
                    </span>
                    <span class="sensor-badge ${device.imuValid ? 'valid' : 'invalid'}">
                        ${device.imuValid ? '✓' : '✗'} IMU
                    </span>
                </div>
                ${device.gpsValid ? `
                    <div class="device-card-location">
                        📍 ${device.gpsLat.toFixed(4)}, ${device.gpsLng.toFixed(4)}
                    </div>
                ` : ''}
                <div class="device-card-lastseen">Last seen: ${lastSeenStr}</div>
            </div>
        `;
    }).join('');
}

// Remove device from fleet (admin only)
async function removeDevice(serialNumber) {
    if (!confirm(`Are you sure you want to remove ${serialNumber} from the fleet?`)) {
        return;
    }

    try {
        const response = await fetch(`${backendUrl}/api/devices/${serialNumber}`, {
            method: 'DELETE',
            credentials: 'include'
        });

        const data = await response.json();

        if (response.ok) {
            addLog('Fleet', `Removed device ${serialNumber}`, 'accent-secondary');
            devices.delete(serialNumber);

            // If the removed device was selected, deselect it
            if (selectedDevice === serialNumber) {
                selectedDevice = null;
                selectedDeviceBanner.style.display = 'none';
            }

            renderDeviceGrid(getDeviceListFromMap());
            updateDeviceCounts();
        } else {
            addLog('Fleet', `Failed to remove device: ${data.error}`, 'danger');
        }
    } catch (err) {
        addLog('Fleet', `Error removing device: ${err.message}`, 'danger');
    }
}

// Select a device to view details
function selectDevice(serialNumber) {
    selectedDevice = serialNumber;

    // Update UI
    selectedDeviceBanner.style.display = 'flex';
    selectedDeviceSN.textContent = serialNumber;

    // Update selected status badge
    const device = devices.get(serialNumber);
    if (device) {
        const status = getDeviceStatus(device);
        selectedDeviceStatus.textContent = status.toUpperCase();
        selectedDeviceStatus.className = `device-status-badge ${status}`;

        // Update dashboard with device data
        updateDashboard(device);

        // Center map on device
        if (device.gps.valid && map) {
            map.setView([device.gps.lat, device.gps.lng], 14);
        }
    }

    // Re-render grid to show selection
    renderDeviceGrid(getDeviceListFromMap());

    addLog('Device', `Selected ${serialNumber}`, 'accent-primary');
}

// Get device status
function getDeviceStatus(device) {
    if (!device.lastSeen) return 'unknown';
    const timeSinceLastSeen = Date.now() - device.lastSeen;
    if (timeSinceLastSeen > 30000) return 'offline';
    if (!device.gps.valid || !device.imu.valid) return 'degraded';
    return 'online';
}

// Convert devices Map to array for rendering
function getDeviceListFromMap() {
    const list = [];
    devices.forEach((device, serialNumber) => {
        list.push({
            serialNumber,
            status: getDeviceStatus(device),
            lastSeen: device.lastSeen,
            gpsValid: device.gps.valid,
            imuValid: device.imu.valid,
            gpsLat: device.gps.lat,
            gpsLng: device.gps.lng
        });
    });
    return list;
}

// Update dashboard for a specific device
function updateDashboard(state) {
    if (!state) return;

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
    gpsSatellites.textContent = state.gps.satellites || 0;

    // GPS Accuracy (from UBX NAV-PVT)
    if (gpsHAcc) gpsHAcc.textContent = `${(state.gps.hAcc || 0).toFixed(0)} mm`;
    if (gpsVAcc) gpsVAcc.textContent = `${(state.gps.vAcc || 0).toFixed(0)} mm`;
    if (gpsAltEllipsoid) {
        const altEllipsoidM = (state.gps.altEllipsoid || 0) / 1000;
        gpsAltEllipsoid.textContent = `${altEllipsoidM.toFixed(1)} m`;
    }

    if (state.gps.valid || DEBUG_TEL_AVIV) {
        gpsValid.textContent = DEBUG_TEL_AVIV ? 'DEBUG: TEL AVIV' : 'FIX ACQUIRED';
        gpsValid.classList.add('on');
    } else {
        gpsValid.textContent = 'NO FIX';
        gpsValid.classList.remove('on');
    }

    // GPS Connected
    if (gpsConnected) {
        if (state.gps.connected) {
            gpsConnected.textContent = 'CONNECTED';
            gpsConnected.classList.add('on');
        } else {
            gpsConnected.textContent = 'DISCONNECTED';
            gpsConnected.classList.remove('on');
        }
    }

    // GPS Velocity (N/E/D)
    if (gpsVelocity) {
        const vn = state.gps.velocityNorth || 0;
        const ve = state.gps.velocityEast || 0;
        const vd = state.gps.velocityDown || 0;
        gpsVelocity.textContent = `${vn.toFixed(1)} / ${ve.toFixed(1)} / ${vd.toFixed(1)} m/s`;
    }

    // GPS Time
    if (gpsTime) {
        gpsTime.textContent = state.gps.time || '--:--:--';
    }

    // Device Info - Serial Number
    if (deviceSerial) {
        deviceSerial.textContent = state.deviceInfo?.serialNumber || selectedDevice || '--';
    }

    // Device Info
    const workHours = state.deviceInfo?.motorWorkHours || 0;
    motorWorkHours.textContent = `${workHours.toFixed(2)} h`;
    firmwareVersion.textContent = state.deviceInfo?.firmwareVersion || '--';
    controllerIp.textContent = state.deviceInfo?.controllerIp || '--';
    routerIp.textContent = state.deviceInfo?.routerIp || '--';

    // Power Monitoring
    if (busVoltage) {
        if (state.power?.connected && state.power.busVoltage > 0) {
            busVoltage.textContent = `${state.power.busVoltage.toFixed(2)} V`;
        } else if (state.power?.connected === false) {
            busVoltage.textContent = 'N/C';
        } else {
            busVoltage.textContent = '--';
        }
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
        if (el) {
            if (val) el.classList.add('on');
            else el.classList.remove('on');
        }
    });

    // Button
    btnTech.textContent = state.sensors.button ? 'PRESSED' : 'RELEASED';
    if (state.sensors.button) btnTech.classList.add('on');
    else btnTech.classList.remove('on');

    // Relays
    state.relays.forEach((val, i) => {
        const el = document.getElementById(`relay-${i}`);
        if (el) {
            el.textContent = val ? 'ACTIVE' : 'OFF';
            if (val) el.classList.add('on');
            else el.classList.remove('on');
        }
    });

    // LEDs
    const ledInt = document.getElementById('led-int');
    if (ledInt) {
        ledInt.textContent = state.leds.internal ? 'ON' : 'OFF';
        if (state.leds.internal) ledInt.classList.add('on');
        else ledInt.classList.remove('on');
    }

    const ledIo = document.getElementById('led-io');
    if (ledIo) {
        ledIo.textContent = state.leds.io;
        if (state.leds.io !== 'OFF') ledIo.classList.add('on');
        else ledIo.classList.remove('on');
    }
}

// Update device counts in header
function updateDeviceCounts() {
    const deviceList = getDeviceListFromMap();
    const online = deviceList.filter(d => d.status === 'online' || d.status === 'degraded').length;
    deviceCountEl.textContent = online;
    deviceTotalEl.textContent = deviceList.length;
}

// Socket event handlers are now initialized in initSocket() after auth check


// Back to fleet button
btnBackToFleet.addEventListener('click', () => {
    selectedDevice = null;
    selectedDeviceBanner.style.display = 'none';
    renderDeviceGrid(getDeviceListFromMap());

    // Reset map view to show all devices
    if (map && deviceMarkers.size > 0) {
        const bounds = L.latLngBounds([]);
        deviceMarkers.forEach(marker => {
            bounds.extend(marker.getLatLng());
        });
        map.fitBounds(bounds, { padding: [50, 50] });
    }
});

// Refresh button
document.getElementById('btn-refresh-fleet').addEventListener('click', () => {
    addLog('System', 'Refreshing device list...', 'accent-secondary');
    // Fetch current state from API
    fetch(`${backendUrl}/api/devices`)
        .then(res => res.json())
        .then(deviceList => {
            renderDeviceGrid(deviceList);
            const online = deviceList.filter(d => d.status === 'online' || d.status === 'degraded').length;
            deviceCountEl.textContent = online;
            deviceTotalEl.textContent = deviceList.length;
            addLog('System', `Found ${deviceList.length} devices`, 'accent-secondary');
        })
        .catch(err => {
            addLog('System', 'Failed to refresh: ' + err.message, 'danger');
        });
});

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

// Check authentication
async function checkAuth() {
    try {
        const res = await fetch('/auth/me', { credentials: 'include' });
        const data = await res.json();

        if (!data.authenticated) {
            window.location.href = '/login.html';
            return false;
        }

        currentUser = data.user;
        allowedDevices = data.allowedDevices;

        // Show admin button in header if user is admin
        if (currentUser.role === 'admin') {
            const adminBtn = document.getElementById('btn-admin');
            if (adminBtn) {
                adminBtn.style.display = 'flex';
            }
        }

        addLog('Auth', `Logged in as ${currentUser.email}`, 'accent-secondary');
        return true;
    } catch (err) {
        console.error('Auth check failed:', err);
        window.location.href = '/login.html';
        return false;
    }
}

// Initialize Socket.io connection
function initSocket() {
    socket = io(backendUrl, { withCredentials: true });

    socket.on('connect', () => {
        mqttStatus.classList.add('active');
        addLog('System', 'Connected to Mission Control Server', 'accent-secondary');
    });

    socket.on('disconnect', () => {
        mqttStatus.classList.remove('active');
        addLog('System', 'Disconnected from Server', 'danger');
    });

    // Handle device list updates
    socket.on('devices_list', (deviceList) => {
        // Filter by allowed devices
        if (allowedDevices) {
            deviceList = deviceList.filter(d => allowedDevices.includes(d.serialNumber));
        }
        renderDeviceGrid(deviceList);

        const online = deviceList.filter(d => d.status === 'online' || d.status === 'degraded').length;
        deviceCountEl.textContent = online;
        deviceTotalEl.textContent = deviceList.length;
    });

    // Handle individual device updates
    socket.on('device_update', (data) => {
        const { serialNumber, device, deviceList } = data;

        // Skip if user doesn't have access to this device
        if (allowedDevices && !allowedDevices.includes(serialNumber)) {
            return;
        }

        devices.set(serialNumber, device);

        if (device.lastSeen) {
            const secondsAgo = Math.floor((Date.now() - device.lastSeen) / 1000);
            lastSeenText.textContent = `LAST UPDATE: ${secondsAgo}S AGO`;
        }

        if (deviceList) {
            let filteredList = deviceList;
            if (allowedDevices) {
                filteredList = deviceList.filter(d => allowedDevices.includes(d.serialNumber));
            }
            renderDeviceGrid(filteredList);
            updateDeviceCounts();
        }

        updateDeviceMarker(serialNumber, device.gps.lat, device.gps.lng, device.gps.valid);

        if (serialNumber === selectedDevice) {
            updateDashboard(device);
            const status = getDeviceStatus(device);
            selectedDeviceStatus.textContent = status.toUpperCase();
            selectedDeviceStatus.className = `device-status-badge ${status}`;
        }

        if (!selectedDevice && devices.size === 1) {
            selectDevice(serialNumber);
        }
    });

    socket.on('device_selected', (device) => {
        updateDashboard(device);
    });
}

// Initialize
window.addEventListener('load', async () => {
    const isAuthed = await checkAuth();
    if (!isAuthed) return;

    initSocket();
    initMap();
    addLog('System', 'Multi-Device Mission Control Loaded', 'accent-secondary');
});

// Make selectDevice and removeDevice available globally
window.selectDevice = selectDevice;
window.removeDevice = removeDevice;
