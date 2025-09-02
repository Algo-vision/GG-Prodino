# GG-Prodino Arduino HTTP API Guide

This document explains how to control the GG-Prodino Arduino device using HTTP requests, including the message structures and expected responses.

## Table of Contents
- [Overview](#overview)
- [Getting Started](#getting-started)
- [API Endpoints](#api-endpoints)
- [Request & Response Examples](#request--response-examples)
- [Error Handling](#error-handling)
- [Notes](#notes)

## Overview
The GG-Prodino device exposes a RESTful HTTP API for remote control and monitoring. You can interact with the device using standard HTTP clients (e.g., curl, Postman, Python requests).

## Getting Started
1. Connect the GG-Prodino device to your network.
2. Determine its IP address (see serial monitor or device display).
3. Use the IP address in your HTTP requests (e.g., `http://<device-ip>/api/...`).

## API Endpoints

### 1. Get Device Status
- **Endpoint:** `GET /api/status`
- **Description:** Returns the current status of the device.
- **Response Example:**
  ```json
  {
    "status": "ok",
    "uptime": 12345,
    "relays": [0, 1, 0, 1],
    "sensors": {
      "temperature": 23.5,
      "humidity": 45.2
    }
  }
  ```

### 2. Control Relays
- **Endpoint:** `POST /api/relay`
- **Description:** Set the state of one or more relays.
- **Request Body:**
  ```json
  {
    "relay": 1,        // Relay number (0-based)
    "state": 1         // 1 = ON, 0 = OFF
  }
  ```
- **Response Example:**
  ```json
  {
    "status": "ok",
    "relay": 1,
    "state": 1
  }
  ```

### 3. Get Sensor Data
- **Endpoint:** `GET /api/sensors`
- **Description:** Returns current sensor readings.
- **Response Example:**
  ```json
  {
    "temperature": 23.5,
    "humidity": 45.2,
    "pressure": 1012.3
  }
  ```

### 4. Custom Commands
- **Endpoint:** `POST /api/command`
- **Description:** Send a custom command to the device.
- **Request Body:**
  ```json
  {
    "cmd": "reboot"
  }
  ```
- **Response Example:**
  ```json
  {
    "status": "rebooting"
  }
  ```

## Request & Response Examples

### Turn Relay 2 ON
```bash
curl -X POST http://<device-ip>/api/relay -H "Content-Type: application/json" -d '{"relay":2,"state":1}'
```

### Get Device Status
```bash
curl http://<device-ip>/api/status
```

## Error Handling
- All error responses will include a `status` field with value `error` and a `message` field describing the issue.
- **Example:**
  ```json
  {
    "status": "error",
    "message": "Invalid relay number"
  }
  ```

## Notes
- Replace `<device-ip>` with the actual IP address of your GG-Prodino device.
- All requests and responses use JSON format.
- Ensure the device is powered and connected to the network.

For more details, refer to the source code or contact the project maintainer.
