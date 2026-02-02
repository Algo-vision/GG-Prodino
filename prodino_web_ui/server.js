const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const mqtt = require('mqtt');
const path = require('path');
const cors = require('cors');
const session = require('express-session');
require('dotenv').config();

// Auth modules
const db = require('./db');
const { passport, initializePassport, isAuthenticated, isAdmin, filterDevicesForUser } = require('./auth');

const app = express();
const server = http.createServer(app);
const io = new Server(server, {
    cors: {
        origin: "*",
        methods: ["GET", "POST"],
        credentials: true
    }
});

// Middleware
app.use(cors({ origin: true, credentials: true }));
app.use(express.json());

// Session middleware - shared with Socket.io
const sessionMiddleware = session({
    secret: process.env.SESSION_SECRET || 'grk-secret-change-in-production',
    resave: false,
    saveUninitialized: false,
    cookie: {
        secure: process.env.NODE_ENV === 'production',
        maxAge: 7 * 24 * 60 * 60 * 1000 // 7 days
    }
});
app.use(sessionMiddleware);

// Passport middleware
app.use(passport.initialize());
app.use(passport.session());

// Check if in development mode (defined early for Socket.io middleware)
const IS_DEV_MODE = process.env.DEV_MODE === 'true' || process.env.NODE_ENV === 'development';

// Socket.io authentication middleware - share session with Socket.io
io.use((socket, next) => {
    // In dev mode, allow unauthenticated connections with mock user
    if (IS_DEV_MODE) {
        socket.user = { id: 'dev', email: 'dev@admin.local', name: 'Dev Admin', role: 'admin', access_type: 'all' };
        console.log('🔧 Socket connected (dev mode)');
        return next();
    }

    sessionMiddleware(socket.request, {}, () => {
        passport.initialize()(socket.request, {}, () => {
            passport.session()(socket.request, {}, () => {
                // Check if user is authenticated
                if (socket.request.session && socket.request.session.passport && socket.request.session.passport.user) {
                    const userId = socket.request.session.passport.user;
                    const user = db.findUserById(userId);
                    if (user) {
                        socket.user = user;
                        console.log(`🔐 Socket authenticated: ${user.email}`);
                        return next();
                    }
                }
                console.log('❌ Socket connection rejected - not authenticated');
                return next(new Error('Authentication required'));
            });
        });
    });
});

// Initialize Passport with Google OAuth
if (!initializePassport()) {
    console.warn('⚠️ Google OAuth not configured - auth disabled');
}

// Seed admin users
const adminEmails = process.env.ADMIN_EMAILS || '';
if (adminEmails) {
    db.seedAdmins(adminEmails);
}

// Serve static files (public pages)
app.use(express.static(path.join(__dirname, 'public')));

// Configuration
const MQTT_BROKER = process.env.MQTT_BROKER || 'mqtt://localhost:1883';
const PORT = process.env.BACKEND_PORT || 5555;
const DEVICE_TIMEOUT_MS = 30000; // Device considered offline after 30 seconds

// Multi-Device State Management
const devices = new Map(); // Key: serialNumber, Value: deviceState

// Create default state for a new device
function createDefaultDeviceState(serialNumber) {
    return {
        serialNumber: serialNumber,
        connected: false,
        lastSeen: null,
        gps: {
            lat: 0, lng: 0, alt: 0, speed: 0, heading: 0,
            valid: false, connected: false, satellites: 0,
            velocityNorth: 0, velocityEast: 0, velocityDown: 0,
            time: '--:--:--'
        },
        imu: {
            accel: { x: 0, y: 0, z: 0 },
            gyro: { x: 0, y: 0, z: 0 },
            orientation: { pitch: 0, roll: 0, yaw: 0 },
            valid: false
        },
        relays: [false, false, false, false],
        leds: { internal: false, io: 'OFF' },
        sensors: { optos: [false, false, false, false], button: false },
        power: { connected: false, busVoltage: 0 },
        deviceInfo: { motorWorkHours: 0, firmwareVersion: '--', controllerIp: '--', routerIp: '--', serialNumber: serialNumber }
    };
}

// Get device status (online/offline/degraded)
function getDeviceStatus(device) {
    if (!device.lastSeen) return 'unknown';
    const timeSinceLastSeen = Date.now() - device.lastSeen;
    if (timeSinceLastSeen > DEVICE_TIMEOUT_MS) return 'offline';
    if (!device.gps.valid || !device.imu.valid) return 'degraded';
    return 'online';
}

// Get all devices as array with status
// Filters out DEFAULT and UNCONFIGURED devices
function getDeviceList() {
    const deviceList = [];
    const EXCLUDED_SERIALS = ['DEFAULT', 'UNCONFIGURED', 'NONE'];
    devices.forEach((device, serialNumber) => {
        // Skip excluded serial numbers
        if (EXCLUDED_SERIALS.includes(serialNumber.toUpperCase())) {
            return;
        }
        deviceList.push({
            serialNumber: serialNumber,
            status: getDeviceStatus(device),
            lastSeen: device.lastSeen,
            gpsValid: device.gps.valid,
            imuValid: device.imu.valid,
            gpsLat: device.gps.lat,
            gpsLng: device.gps.lng,
            ledIo: device.leds.io
        });
    });
    return deviceList;
}

const fs = require('fs');

// MQTT Client Setup
console.log(`Connecting to MQTT Broker: ${MQTT_BROKER}`);

let mqttOptions = {
    reconnectPeriod: 5000,
    connectTimeout: 30 * 1000,
};

// Check for AWS IoT Certificates
if (process.env.MQTT_KEY_PATH && process.env.MQTT_CERT_PATH && process.env.MQTT_CA_PATH) {
    console.log('🔒 Using AWS IoT Certificates for Mutual TLS');
    try {
        mqttOptions.key = fs.readFileSync(process.env.MQTT_KEY_PATH);
        mqttOptions.cert = fs.readFileSync(process.env.MQTT_CERT_PATH);
        mqttOptions.ca = fs.readFileSync(process.env.MQTT_CA_PATH);
        mqttOptions.protocol = 'mqtts';
        mqttOptions.rejectUnauthorized = true;
    } catch (err) {
        console.error('❌ Error loading certificates:', err.message);
    }
}

const client = mqtt.connect(MQTT_BROKER, mqttOptions);

client.on('connect', () => {
    console.log('Connected to MQTT Broker');
    // Subscribe to all devices with wildcard for serial number
    // Supports both old format (prodino/gps/...) and new format (prodino/{SN}/gps/...)
    client.subscribe('prodino/#', (err) => {
        if (!err) console.log('Subscribed to prodino/# (all devices)');
    });
});

client.on('error', (err) => {
    console.error('MQTT Error:', err);
});

// Parse topic to extract serial number and data path
// New format: prodino/{serialNumber}/{dataPath}
// Legacy format: prodino/{dataPath} (uses "DEFAULT" as serial number)
function parseTopicPath(topic) {
    const parts = topic.split('/');
    if (parts.length < 2 || parts[0] !== 'prodino') {
        return null;
    }

    // Check if second part looks like a serial number (starts with SN, GRK, or is alphanumeric ID)
    const potentialSN = parts[1];
    const isSerialNumber = /^(SN|GRK|PRODINO)[A-Z0-9-]+$/i.test(potentialSN) ||
        /^[A-Z]{2,4}[0-9]{3,6}$/i.test(potentialSN);

    if (isSerialNumber && parts.length >= 3) {
        // New format: prodino/SN0001/gps/position
        return {
            serialNumber: potentialSN.toUpperCase(),
            dataPath: parts.slice(2).join('/')
        };
    } else {
        // Legacy format: prodino/gps/position (single device, no serial number)
        return {
            serialNumber: 'DEFAULT',
            dataPath: parts.slice(1).join('/')
        };
    }
}

client.on('message', (topic, message) => {
    const payload = message.toString();
    let data;
    try {
        data = JSON.parse(payload);
    } catch (e) {
        data = payload;
    }

    // Parse topic to get serial number and data path
    const parsed = parseTopicPath(topic);
    if (!parsed) {
        console.log(`⚠️ Ignoring invalid topic: ${topic}`);
        return;
    }

    const { serialNumber, dataPath } = parsed;

    // Get or create device state
    if (!devices.has(serialNumber)) {
        console.log(`📱 New device discovered: ${serialNumber}`);
        devices.set(serialNumber, createDefaultDeviceState(serialNumber));
    }

    const device = devices.get(serialNumber);
    device.lastSeen = Date.now();
    device.connected = true;

    console.log(`← [${serialNumber}] ${dataPath}: ${payload.substring(0, 60)}${payload.length > 60 ? '...' : ''}`);

    // Update device state based on data path
    switch (dataPath) {
        case 'gps/position':
            device.gps = { ...device.gps, ...data };
            break;
        case 'gps/velocity':
            device.gps.speed = data.ground || 0;
            device.gps.velocityNorth = data.north || 0;
            device.gps.velocityEast = data.east || 0;
            device.gps.velocityDown = data.down || 0;
            break;
        case 'gps/time':
            device.gps.time = data || '--:--:--';
            break;
        case 'gps/heading':
            device.gps.heading = parseFloat(data);
            break;
        case 'validity/gps':
            device.gps.valid = data.valid;
            device.gps.connected = data.connected;
            break;
        case 'imu/accel':
            device.imu.accel = data;
            break;
        case 'imu/gyro':
            device.imu.gyro = { x: data.gx, y: data.gy, z: data.gz };
            break;
        case 'imu/orientation':
            device.imu.orientation = data;
            break;
        case 'validity/imu':
            device.imu.valid = (data === 'true' || data === true);
            break;
        case 'relays/state':
            device.relays = data;
            break;
        case 'leds/internal':
            device.leds.internal = (data === 'true' || data === true);
            break;
        case 'leds/io':
            device.leds.io = data;
            break;
        case 'sensors/optos':
            device.sensors.optos = data;
            break;
        case 'sensors/button_tech':
            device.sensors.button = (data === 'true' || data === true);
            break;
        case 'gps/satellites':
            device.gps.satellites = parseInt(data) || 0;
            break;
        case 'status':
            // Full status message from device - extract deviceInfo fields
            if (typeof data === 'object') {
                device.deviceInfo = {
                    motorWorkHours: data.motorWorkHours || 0,
                    firmwareVersion: data.firmwareVersion || '--',
                    controllerIp: data.controllerIp || '--',
                    routerIp: data.routerIp || '--'
                };
                // Also update satellites from status message if present
                if (data.gpsSatellites !== undefined) {
                    device.gps.satellites = data.gpsSatellites;
                }
                // Update power monitoring from status if present
                if (data.inaConnected !== undefined) {
                    device.power.connected = data.inaConnected;
                }
                if (data.busVoltage !== undefined) {
                    device.power.busVoltage = data.busVoltage;
                }
            }
            break;
        case 'power/connected':
            device.power.connected = (data === 'true' || data === true);
            break;
        case 'power/bus_voltage':
            device.power.busVoltage = parseFloat(data) || 0;
            break;
    }

    // Broadcast to authenticated web clients (filtered by user permissions)
    io.sockets.sockets.forEach((socket) => {
        if (socket.user) {
            const filteredList = filterDevicesForUser(getDeviceList(), socket.user);
            const allowedSerials = filteredList.map(d => d.serialNumber);

            // Only send if user has access to this device
            if (allowedSerials.length === 0 || allowedSerials.includes(serialNumber)) {
                socket.emit('device_update', {
                    serialNumber: serialNumber,
                    device: device,
                    deviceList: filteredList
                });
            }
        }
    });
});

// Socket.io Connection (authentication is checked by middleware above)
io.on('connection', (socket) => {
    const user = socket.user;
    console.log(`🌐 Web Client Connected: ${user.email} (${user.role})`);

    // Get device list filtered by user permissions
    const filteredDeviceList = filterDevicesForUser(getDeviceList(), user);
    const allowedSerials = user.role === 'admin' || user.access_type === 'all'
        ? null  // null means all devices
        : filteredDeviceList.map(d => d.serialNumber);

    // Send current device list (filtered)
    socket.emit('devices_list', filteredDeviceList);

    // Send device states (only for devices user can access)
    devices.forEach((device, serialNumber) => {
        if (allowedSerials === null || allowedSerials.includes(serialNumber)) {
            socket.emit('device_update', {
                serialNumber: serialNumber,
                device: device,
                deviceList: filteredDeviceList
            });
        }
    });

    // Handle device selection request (check permission)
    socket.on('select_device', (serialNumber) => {
        console.log(`📱 ${user.email} selected device: ${serialNumber}`);

        // Verify user has access to this device
        if (allowedSerials !== null && !allowedSerials.includes(serialNumber)) {
            console.log(`⚠️ Access denied to device ${serialNumber} for ${user.email}`);
            return;
        }

        if (devices.has(serialNumber)) {
            socket.emit('device_selected', devices.get(serialNumber));
        }
    });

    socket.on('disconnect', () => {
        console.log(`👋 Web Client Disconnected: ${user.email}`);
    });
});

// Periodic device status check (mark devices offline)
setInterval(() => {
    let statusChanged = false;
    devices.forEach((device, serialNumber) => {
        const previouslyConnected = device.connected;
        device.connected = (Date.now() - device.lastSeen) < DEVICE_TIMEOUT_MS;
        if (previouslyConnected !== device.connected) {
            console.log(`📱 Device ${serialNumber} is now ${device.connected ? 'ONLINE' : 'OFFLINE'}`);
            statusChanged = true;
        }
    });

    if (statusChanged) {
        // Send filtered device list to each authenticated user
        io.sockets.sockets.forEach((socket) => {
            if (socket.user) {
                const filteredList = filterDevicesForUser(getDeviceList(), socket.user);
                socket.emit('devices_list', filteredList);
            }
        });
    }
}, 5000);

// ============================================================
// AUTH ROUTES
// ============================================================

// Check if in development mode
// Use IS_DEV_MODE defined earlier

if (IS_DEV_MODE) {
    console.log('🔧 DEV MODE ENABLED - Using mock authentication');

    // Dev mode: Mock login page
    app.get('/auth/dev-login', (req, res) => {
        res.send(`
            <!DOCTYPE html>
            <html>
            <head>
                <title>Dev Login</title>
                <style>
                    body { font-family: Arial; background: #1a1a2e; color: white; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }
                    .container { background: #16213e; padding: 40px; border-radius: 10px; text-align: center; }
                    input { padding: 10px; margin: 10px; width: 200px; border-radius: 5px; border: none; }
                    button { padding: 10px 30px; background: #4f46e5; color: white; border: none; border-radius: 5px; cursor: pointer; }
                    button:hover { background: #6366f1; }
                    h2 { color: #f59e0b; }
                </style>
            </head>
            <body>
                <div class="container">
                    <h2>🔧 DEV MODE LOGIN</h2>
                    <p>This bypasses Google OAuth for local development</p>
                    <form action="/auth/dev-login" method="POST">
                        <input type="email" name="email" placeholder="Email" value="dev@admin.local" required /><br/>
                        <input type="text" name="name" placeholder="Name" value="Dev Admin" required /><br/>
                        <button type="submit">Login as Admin</button>
                    </form>
                </div>
            </body>
            </html>
        `);
    });

    // Dev mode: Handle mock login POST
    app.post('/auth/dev-login', express.urlencoded({ extended: true }), (req, res) => {
        const { email, name } = req.body;

        // Create or get mock user
        let user = db.findUserByEmail(email);
        if (!user) {
            // Auto-create user as admin for dev mode
            db.createUser(email, 'Dev Admin', 'admin', 'all', 'dev-mode');
            user = db.findUserByEmail(email);
        }

        if (user) {
            db.updateUserName(user.id, name);
            db.updateUserLastLogin(user.id);
            user = db.findUserById(user.id);
        }

        // Create session
        req.login(user, (err) => {
            if (err) {
                console.error('Dev login error:', err);
                return res.status(500).send('Login failed');
            }
            console.log(`🔧 Dev login successful: ${email} (${name})`);
            res.redirect('/');
        });
    });

    // Override Google OAuth to redirect to dev login
    app.get('/auth/google', (req, res) => {
        res.redirect('/auth/dev-login');
    });

    app.get('/auth/google/callback', (req, res) => {
        res.redirect('/auth/dev-login');
    });
} else {
    // Production: Google OAuth login
    app.get('/auth/google', passport.authenticate('google', {
        scope: ['profile', 'email']
    }));

    // Google OAuth callback
    app.get('/auth/google/callback',
        passport.authenticate('google', {
            failureRedirect: '/login.html?error=access_denied',
            successRedirect: '/'
        })
    );
}

// Logout
app.get('/auth/logout', (req, res) => {
    req.logout((err) => {
        if (err) console.error('Logout error:', err);
        res.redirect('/login.html');
    });
});

// Get current user info
app.get('/auth/me', (req, res) => {
    if (req.isAuthenticated()) {
        const { id, email, name, role, access_type } = req.user;
        const allowedDevices = db.getUserDevices(id);
        res.json({
            authenticated: true,
            user: { id, email, name, role, access_type },
            allowedDevices: access_type === 'restricted' ? allowedDevices : null
        });
    } else {
        res.json({ authenticated: false });
    }
});

// ============================================================
// ADMIN API ROUTES
// ============================================================

// Get all users (admin only)
app.get('/api/admin/users', isAuthenticated, isAdmin, (req, res) => {
    const users = db.getAllUsers();
    // Add device assignments for each user
    const usersWithDevices = users.map(user => ({
        ...user,
        devices: db.getUserDevices(user.id)
    }));
    res.json(usersWithDevices);
});

// Add new user (admin only)
app.post('/api/admin/users', isAuthenticated, isAdmin, (req, res) => {
    const { email, role = 'user', accessType = 'all', devices: deviceList = [] } = req.body;

    if (!email || !email.includes('@')) {
        return res.status(400).json({ error: 'Valid email required' });
    }

    const result = db.createUser(email.toLowerCase(), null, role, accessType, req.user.email);

    if (!result.success) {
        return res.status(400).json({ error: result.error });
    }

    // Assign devices if specified
    if (accessType === 'restricted' && deviceList.length > 0) {
        for (const sn of deviceList) {
            db.assignDevice(result.id, sn, req.user.email);
        }
    }

    res.json({ success: true, id: result.id });
});

// Update user devices (admin only)
app.put('/api/admin/users/:id/devices', isAuthenticated, isAdmin, (req, res) => {
    const userId = parseInt(req.params.id);
    const { devices: deviceList = [] } = req.body;

    // Remove all existing device assignments
    db.removeAllDevices(userId);

    // Add new assignments
    for (const sn of deviceList) {
        db.assignDevice(userId, sn, req.user.email);
    }

    res.json({ success: true });
});

// Delete user (admin only)
app.delete('/api/admin/users/:id', isAuthenticated, isAdmin, (req, res) => {
    const userId = parseInt(req.params.id);
    const result = db.deleteUser(userId);

    if (!result.success) {
        return res.status(400).json({ error: result.error });
    }

    res.json({ success: true });
});

// ============================================================
// DEVICE API ENDPOINTS (Protected)
// ============================================================

// Get all devices (filtered by user permissions)
app.get('/api/devices', isAuthenticated, (req, res) => {
    let deviceList = getDeviceList();
    deviceList = filterDevicesForUser(deviceList, req.user);
    res.json(deviceList);
});

// Get specific device state
app.get('/api/devices/:serialNumber', isAuthenticated, (req, res) => {
    const { serialNumber } = req.params;

    // Check permission
    const allowed = filterDevicesForUser([{ serialNumber }], req.user);
    if (allowed.length === 0) {
        return res.status(403).json({ error: 'Access denied to this device' });
    }

    if (devices.has(serialNumber)) {
        res.json(devices.get(serialNumber));
    } else {
        res.status(404).json({ error: 'Device not found', serialNumber });
    }
});

// Remove device from fleet (admin only)
app.delete('/api/devices/:serialNumber', isAuthenticated, isAdmin, (req, res) => {
    const { serialNumber } = req.params;

    if (devices.has(serialNumber)) {
        devices.delete(serialNumber);
        console.log(`🗑️ Device ${serialNumber} removed from fleet by ${req.user.email}`);

        // Notify all connected clients about the updated device list
        io.sockets.sockets.forEach((socket) => {
            if (socket.user) {
                const filteredList = filterDevicesForUser(getDeviceList(), socket.user);
                socket.emit('devices_list', filteredList);
            }
        });

        res.json({ success: true, message: `Device ${serialNumber} removed from fleet` });
    } else {
        res.status(404).json({ error: 'Device not found', serialNumber });
    }
});

// Legacy endpoint - returns first device or DEFAULT
app.get('/api/state', isAuthenticated, (req, res) => {
    if (devices.has('DEFAULT')) {
        res.json(devices.get('DEFAULT'));
    } else if (devices.size > 0) {
        res.json(devices.values().next().value);
    } else {
        res.json(createDefaultDeviceState('NONE'));
    }
});

// Device count summary
app.get('/api/summary', isAuthenticated, (req, res) => {
    let deviceList = getDeviceList();
    deviceList = filterDevicesForUser(deviceList, req.user);

    const online = deviceList.filter(d => d.status === 'online').length;
    const degraded = deviceList.filter(d => d.status === 'degraded').length;
    const offline = deviceList.filter(d => d.status === 'offline').length;

    res.json({
        total: deviceList.length,
        online,
        degraded,
        offline,
        devices: deviceList
    });
});

server.listen(PORT, () => {
    console.log(`🚀 Backend Server running on http://localhost:${PORT}`);
    console.log(`📡 Multi-device support enabled`);
});
