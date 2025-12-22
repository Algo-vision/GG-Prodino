const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const mqtt = require('mqtt');
const path = require('path');
const cors = require('cors');
require('dotenv').config();

const app = express();
const server = http.createServer(app);
const io = new Server(server, {
    cors: {
        origin: "*",
        methods: ["GET", "POST"]
    }
});

app.use(cors());
// Static serving removed from backend

// Configuration
const MQTT_BROKER = process.env.MQTT_BROKER || 'mqtt://localhost:1883';
const PORT = process.env.BACKEND_PORT || 5555;

// State Management
let deviceState = {
    connected: false,
    lastSeen: null,
    gps: { lat: 0, lng: 0, alt: 0, speed: 0, heading: 0, valid: false, connected: false },
    imu: {
        accel: { x: 0, y: 0, z: 0 },
        gyro: { x: 0, y: 0, z: 0 },
        orientation: { pitch: 0, roll: 0, yaw: 0 },
        valid: false
    },
    relays: [false, false, false, false],
    leds: { internal: false, io: 'OFF' },
    sensors: { optos: [false, false, false, false], button: false }
};

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
    client.subscribe('prodino/#', (err) => {
        if (!err) console.log('Subscribed to prodino/#');
    });
});

client.on('error', (err) => {
    console.error('MQTT Error:', err);
});

client.on('message', (topic, message) => {
    const payload = message.toString();
    let data;
    try {
        data = JSON.parse(payload);
    } catch (e) {
        data = payload;
    }

    console.log(`← ${topic}: ${payload.substring(0, 80)}${payload.length > 80 ? '...' : ''}`);

    deviceState.lastSeen = Date.now();
    deviceState.connected = true;

    // Update state based on topic
    if (topic === 'prodino/gps/position') deviceState.gps = { ...deviceState.gps, ...data };
    else if (topic === 'prodino/gps/velocity') deviceState.gps.speed = data.ground;
    else if (topic === 'prodino/gps/heading') deviceState.gps.heading = parseFloat(data);
    else if (topic === 'prodino/validity/gps') {
        deviceState.gps.valid = data.valid;
        deviceState.gps.connected = data.connected;
    }
    else if (topic === 'prodino/imu/accel') deviceState.imu.accel = data;
    else if (topic === 'prodino/imu/gyro') deviceState.imu.gyro = { x: data.gx, y: data.gy, z: data.gz };
    else if (topic === 'prodino/imu/orientation') deviceState.imu.orientation = data;
    else if (topic === 'prodino/validity/imu') deviceState.imu.valid = (data === 'true' || data === true);
    else if (topic === 'prodino/relays/state') deviceState.relays = data;
    else if (topic === 'prodino/leds/internal') deviceState.leds.internal = (data === 'true' || data === true);
    else if (topic === 'prodino/leds/io') deviceState.leds.io = data;
    else if (topic === 'prodino/sensors/optos') deviceState.sensors.optos = data;
    else if (topic === 'prodino/sensors/button_tech') deviceState.sensors.button = (data === 'true' || data === true);

    // Broadcast to all connected web clients
    io.emit('device_update', deviceState);
});

// Socket.io Connection
io.on('connection', (socket) => {
    console.log('Web Client Connected');
    socket.emit('device_update', deviceState);
});

// API Endpoints
app.get('/api/state', (req, res) => {
    res.json(deviceState);
});

server.listen(PORT, () => {
    console.log(`Backend Server running on http://localhost:${PORT}`);
});
