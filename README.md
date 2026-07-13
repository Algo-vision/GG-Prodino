# GG-GRK - V1.5

GRK Controller: an embedded IoT device with IMU, GPS, relay control, MQTT telemetry, and a real-time web dashboard.

## Repository Layout

- **[`firmware/`](firmware/)** - Board firmware (PlatformIO/Arduino), HTTP API reference, hardware/LED behavior. See [`firmware/README.md`](firmware/README.md).
- **[`server/web_ui/`](server/web_ui/)** - GRK Mission Control, a real-time Node.js/Socket.io web dashboard with live GPS tracking.
- **[`server/mqtt_server/`](server/mqtt_server/)** - Python MQTT debug subscriber with a REST/WebSocket API. See [`server/mqtt_server/README.md`](server/mqtt_server/README.md).
- **[`tools/`](tools/)** - Desktop GUI client, API test scripts, and OTA uploader. See [`tools/README.md`](tools/README.md).
- **[`docs/`](docs/)** - Deployment guides:
  - [Local Development Setup](docs/local_setup/LOCAL_DEVELOPMENT_SETUP.md)
  - [Cloud Setup Guide](docs/cloud_setup/) - AWS IoT Core, EC2, and RUTX12 bridge configuration

## MQTT Topics

The board publishes to `grk/{serial}/...` topics, e.g. `grk/gps/position`, `grk/imu/orientation`, `grk/relays/state`. See [`firmware/README.md`](firmware/README.md#mqtt-integration) for the full topic list.

## Getting Started

- To build/flash the board: see [`firmware/README.md`](firmware/README.md#installation-instructions).
- To run the web dashboard or MQTT debug server locally: see [`docs/local_setup/LOCAL_DEVELOPMENT_SETUP.md`](docs/local_setup/LOCAL_DEVELOPMENT_SETUP.md).
- To deploy to AWS IoT Core / EC2 / RUTX12: see [`docs/cloud_setup/`](docs/cloud_setup/).
