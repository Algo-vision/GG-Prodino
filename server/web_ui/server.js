const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
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
const PORT = process.env.BACKEND_PORT || 5555;
// Must comfortably exceed the board's telemetry interval (60 s) or the dashboard
// flips a perfectly healthy device offline between sends. 3 missed sends.
const DEVICE_TIMEOUT_MS = 180000;

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
            time: '--:--:--',
            hAcc: 0, vAcc: 0, altEllipsoid: 0
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
        jetson: { cpuTemp: null },  // gateway (Jetson/RPi) CPU temp, from grk/<SN>/jetson/cpu_temp
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
// Placeholder / test serials that must never surface in the UI.
// SNTEST is tools/test_ingest.py's throwaway serial - never a real board.
const EXCLUDED_SERIALS = ['DEFAULT', 'UNCONFIGURED', 'NONE', 'SNTEST'];
function isExcludedSerial(serialNumber) {
    return EXCLUDED_SERIALS.includes(String(serialNumber).toUpperCase());
}

function getDeviceList() {
    const deviceList = [];
    devices.forEach((device, serialNumber) => {
        // Skip excluded serial numbers
        if (isExcludedSerial(serialNumber)) {
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

// ===========================================================================
// ENCRYPTED TELEMETRY INGEST  (POST /api/ingest)
//
// The board cannot afford TLS: a handshake costs it ~9.8 s of blocked CPU and
// leaves under 1 KB of free RAM (see docs/TELEMETRY_BENCHMARK.md). Instead it
// encrypts the status JSON with ChaCha20-Poly1305 - 9.6 ms, no handshake - and
// posts the packet over plain HTTP. Confidentiality, authenticity and replay
// protection all come from the AEAD, not from the transport.
//
// Wire format (mirrors firmware/include/secure_telemetry.hpp and
// tools/ingest_test_server.py, which is the reference implementation):
//   off  size  field
//   0    1     version = 0x02
//   1    16    serial (ASCII, NUL-padded)
//   17   12    nonce = boot_id(8, random per boot) || msg_seq(4 BE)
//   29   N     ciphertext
//   29+N 16    Poly1305 tag
//   AAD = bytes 0..28 (the header is authenticated but not encrypted, because
//         the server must read the serial in clear to pick the key)
//
// v1 used a flash-stored boot counter, which every firmware upload erased - a
// reflashed board restarted at 1 and got rejected as a replay until the backend
// was restarted. v2's boot id is 64 random bits drawn at boot, so there is no
// stored state to lose and technicians can reflash freely.
// ===========================================================================
const crypto = require('crypto');

const TELEM_VERSION = 2;
const TELEM_HEADER_LEN = 29;
const TELEM_TAG_LEN = 16;

// How many distinct boots to remember per device. A boot id is only ever seen
// again if an attacker replays that session, so this bounds how far back replay
// protection reaches: 512 boots is far more than a board sees in service, and
// costs ~20 KB per device.
const REPLAY_BOOT_HISTORY = 512;

// Per-board keys: { "SN2003": "<64 hex>" }. Gitignored - it holds secrets.
// Deleting an entry revokes that board instantly; replacing it rotates the key.
const DEVICE_KEYS_FILE = path.join(__dirname, 'device_keys.json');
let deviceKeys = {};
try {
    const raw = JSON.parse(fs.readFileSync(DEVICE_KEYS_FILE, 'utf8'));
    for (const [sn, hex] of Object.entries(raw)) {
        deviceKeys[sn.toUpperCase()] = Buffer.from(hex, 'hex');
    }
    console.log(`🔑 Loaded telemetry keys for: ${Object.keys(deviceKeys).join(', ') || '(none)'}`);
} catch (err) {
    console.warn(`⚠️  No ${DEVICE_KEYS_FILE} - /api/ingest will reject every board. ` +
                 `Generate one with tools/gen_device_key.py <SERIAL>`);
}

// Replay state per board: serial -> Map(bootIdHex -> highest msg_seq accepted).
// A packet is accepted when its boot id has never been seen (a genuine new boot,
// since boot ids are random) or when its sequence advances that boot's counter.
// Ordering between boots is deliberately NOT required - that is what let a
// reflashed board be mistaken for an attacker under v1.
const replayState = new Map();

function checkAndRecordNonce(serial, bootIdHex, msgSeq) {
    let boots = replayState.get(serial);
    if (!boots) { boots = new Map(); replayState.set(serial, boots); }

    const highest = boots.get(bootIdHex);
    if (highest !== undefined && msgSeq <= highest) return false;   // replay

    boots.set(bootIdHex, msgSeq);
    // Map preserves insertion order, so the first key is the oldest boot.
    while (boots.size > REPLAY_BOOT_HISTORY) {
        boots.delete(boots.keys().next().value);
    }
    return true;
}

// Map the board's flat status JSON onto the device state, using the same field
// names the board's own HTTP API uses, so both surfaces agree.
function applyStatusToDevice(device, d) {
    // Firmware 1.5.1 nests the status document (config / overview / imu / gps).
    // Older firmware sent one flat object, so read through a helper that accepts
    // either shape - a board on the previous firmware keeps working.
    const imu = d.imu || d;
    const gps = d.gps || d;
    const ov  = d.overview || d;
    const cfg = d.config || d;

    if (imu.imuX !== undefined) device.imu.accel = { x: imu.imuX, y: imu.imuY, z: imu.imuZ };
    if (imu.imuGx !== undefined) device.imu.gyro = { x: imu.imuGx, y: imu.imuGy, z: imu.imuGz };
    if (imu.pitch !== undefined) device.imu.orientation = { pitch: imu.pitch, roll: imu.roll, yaw: imu.yaw };
    if (imu.imuValid !== undefined) device.imu.valid = !!imu.imuValid;

    if (gps.gpsLat !== undefined) { device.gps.lat = gps.gpsLat; device.gps.lng = gps.gpsLng; device.gps.alt = gps.gpsAlt; }
    if (gps.gpsGroundSpeed !== undefined) device.gps.speed = gps.gpsGroundSpeed;
    if (gps.gpsHeading !== undefined) device.gps.heading = gps.gpsHeading;
    if (gps.gpsSpeedNorth !== undefined) {
        device.gps.velocityNorth = gps.gpsSpeedNorth;
        device.gps.velocityEast = gps.gpsSpeedEast;
        device.gps.velocityDown = gps.gpsSpeedDown;
    }
    if (gps.gpsTime !== undefined) device.gps.time = gps.gpsTime;
    if (gps.gpsValid !== undefined) device.gps.valid = !!gps.gpsValid;
    if (gps.gpsConnected !== undefined) device.gps.connected = !!gps.gpsConnected;
    if (gps.GPSConnected !== undefined) device.gps.connected = !!gps.GPSConnected;
    if (gps.gpsSatellites !== undefined) device.gps.satellites = gps.gpsSatellites;
    if (gps.gpsHAcc !== undefined) device.gps.hAcc = gps.gpsHAcc;
    if (gps.gpsVAcc !== undefined) device.gps.vAcc = gps.gpsVAcc;
    if (gps.gpsAltEllipsoid !== undefined) device.gps.altEllipsoid = gps.gpsAltEllipsoid;

    if (Array.isArray(ov.relays_status)) device.relays = ov.relays_status;
    if (Array.isArray(ov.optoin_status)) device.sensors.optos = ov.optoin_status;
    if (d.button_tech !== undefined) device.sensors.button = !!d.button_tech;
    if (d.ledInternal !== undefined) device.leds.internal = !!d.ledInternal;
    if (d.ledIo !== undefined) device.leds.io = d.ledIo;

    if (ov.powerConnected !== undefined) device.power.connected = !!ov.powerConnected;
    // 1.5.1 renamed busVoltage/busCurrent_mA to systemVoltage/systemCurrent_A.
    if (ov.systemVoltage !== undefined) device.power.busVoltage = ov.systemVoltage;
    else if (ov.busVoltage !== undefined) device.power.busVoltage = ov.busVoltage;

    device.deviceInfo = {
        ...device.deviceInfo,
        motorWorkHours: d.motorWorkHours !== undefined ? d.motorWorkHours : device.deviceInfo.motorWorkHours,
        firmwareVersion: cfg.firmwareVersion || device.deviceInfo.firmwareVersion,
        controllerIp: cfg.controllerIp || device.deviceInfo.controllerIp,
        routerIp: cfg.routerIp || device.deviceInfo.routerIp
    };
}

// Verify, decrypt and apply one telemetry packet. Transport-agnostic: both the
// HTTP route and the UDP listener hand their bytes to this, so there is exactly
// one implementation of the crypto and the replay rule.
// Returns an HTTP-style status: 204 ok, 400 malformed, 401 auth, 409 replay.
function handleTelemetryPacket(pkt, via) {
    if (!Buffer.isBuffer(pkt) || pkt.length < TELEM_HEADER_LEN + TELEM_TAG_LEN) {
        return 400;
    }

    const header = pkt.subarray(0, TELEM_HEADER_LEN);              // AAD
    const version = header[0];
    const serial = header.subarray(1, 17).toString('ascii').replace(/\0.*$/, '');
    const nonce = header.subarray(17, 29);
    const bootIdHex = nonce.subarray(0, 8).toString('hex');
    const msgSeq = nonce.readUInt32BE(8);
    const ciphertext = pkt.subarray(TELEM_HEADER_LEN, pkt.length - TELEM_TAG_LEN);
    const tag = pkt.subarray(pkt.length - TELEM_TAG_LEN);

    const key = deviceKeys[serial.toUpperCase()];
    if (version !== TELEM_VERSION || !key) {
        console.warn(`🚫 ingest: unknown device serial="${serial}" version=${version}`);
        return 401;
    }

    // Authenticity + confidentiality. Throws if anything was tampered with.
    let plaintext;
    try {
        const decipher = crypto.createDecipheriv('chacha20-poly1305', key, nonce,
                                                 { authTagLength: TELEM_TAG_LEN });
        decipher.setAAD(header, { plaintextLength: ciphertext.length });
        decipher.setAuthTag(tag);
        plaintext = Buffer.concat([decipher.update(ciphertext), decipher.final()]);
    } catch (err) {
        console.warn(`🚫 ingest: BAD AUTH from ${serial} boot=${bootIdHex} seq=${msgSeq}`);
        return 401;
    }

    if (!checkAndRecordNonce(serial, bootIdHex, msgSeq)) {
        console.warn(`🚫 ingest: REPLAY from ${serial} boot=${bootIdHex} seq=${msgSeq}`);
        return 409;
    }

    let data;
    try {
        data = JSON.parse(plaintext.toString('utf8'));
    } catch (err) {
        console.warn(`🚫 ingest: ${serial} sent ${plaintext.length}B that is not JSON`);
        return 400;
    }

    if (!devices.has(serial)) {
        console.log(`📱 New device discovered via ingest: ${serial}`);
        devices.set(serial, createDefaultDeviceState(serial));
    }
    const device = devices.get(serial);
    device.lastSeen = Date.now();
    device.connected = true;
    applyStatusToDevice(device, data);

    console.log(`🔐 [${serial}] ${via} boot=${bootIdHex.slice(0, 8)} seq=${msgSeq} ${pkt.length}B ` +
                `pitch=${(data.imu||data).pitch} roll=${(data.imu||data).roll} ip=${(data.config||data).controllerIp}`);

    broadcastDeviceUpdate(serial, device);
    return 204;
}

// express.json() is global, so this route brings its own raw-body parser.
app.post('/api/ingest',
    express.raw({ type: () => true, limit: '8kb' }),
    (req, res) => res.sendStatus(handleTelemetryPacket(req.body, 'ingest/tcp')));

// ---------------------------------------------------------------------------
// The same packet over UDP.
//
// A TCP POST costs the board three ~80 ms round trips (connect, reply, close);
// the connect alone blocked its main loop for 78 ms of the 97 ms total. UDP has
// no handshake, so a send is just the 16 ms of crypto plus the write.
//
// Dropping TCP costs us nothing here: the packet is already self-contained and
// authenticated, so it needs neither ordering nor a stream. A lost datagram just
// means one missed status update, and the server sees the gap in msg_seq.
//
// The reply is a single ASCII byte ('2' ok, '4' rejected, '9' replay) so the
// board can still count successes - it reads it without blocking.
// ---------------------------------------------------------------------------
const dgram = require('dgram');
const udpIngest = dgram.createSocket('udp4');

udpIngest.on('message', (pkt, rinfo) => {
    let code;
    try {
        code = handleTelemetryPacket(pkt, 'ingest/udp');
    } catch (err) {
        console.error('udp ingest error:', err.message);
        code = 400;
    }
    const reply = Buffer.from([code === 204 ? 0x32 : code === 409 ? 0x39 : 0x34]);
    udpIngest.send(reply, rinfo.port, rinfo.address, () => {});
});

udpIngest.on('error', (err) => console.error('UDP ingest socket error:', err.message));
udpIngest.bind(PORT, () => console.log(`📨 UDP telemetry ingest listening on :${PORT}`));

// Broadcast one device's state to authenticated web clients (filtered by user
// permissions).
function broadcastDeviceUpdate(serialNumber, device) {
    if (isExcludedSerial(serialNumber)) return;
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
}

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

    // Send device states (only for devices user can access, never placeholders -
    // otherwise the UI can end up "viewing" a device that is not in the fleet list)
    devices.forEach((device, serialNumber) => {
        if (isExcludedSerial(serialNumber)) return;
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

// The board holds ONE TCP connection open and reuses it for every telemetry
// send, so it never pays the ~78 ms TCP handshake (which blocked its main loop
// and delayed the 20 Hz HTTP API its local consumers poll). Node's default
// keepAliveTimeout is 5 s, which would hang up between sends - raise it well
// past the send interval. headersTimeout must exceed keepAliveTimeout.
server.keepAliveTimeout = 15 * 60 * 1000;   // 15 minutes
server.headersTimeout   = 16 * 60 * 1000;

server.listen(PORT, () => {
    console.log(`🚀 Backend Server running on http://localhost:${PORT}`);
    console.log(`📡 Multi-device support enabled`);
});
