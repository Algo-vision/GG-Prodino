# Local Development Setup Guide

This guide describes how to run the Prodino system locally on your laptop without using the RUTX12 cellular gateway.

## 1. Run a Local MQTT Broker
You need a local Mosquitto broker running on your laptop to receive data from the Prodino.

### Windows
1.  Open a command prompt as Administrator.
2.  Navigate to the Mosquitto folder: `cd "C:\Program Files\mosquitto"`
3.  Run Mosquitto in verbose mode:
    ```cmd
    mosquitto -v
    ```

### Linux
Run the following in your terminal:
```bash
sudo systemctl start mosquitto
# To watch logs:
tail -f /var/log/mosquitto/mosquitto.log
```

## 2. Configure the Prodino
The Prodino must be pointed to your **laptop's local IP address**.

1.  Find your laptop's IP address:
    *   **Windows:** `ipconfig` (Look for IPv4 Address under Ethernet/Wi-Fi).
    *   **Linux:** `hostname -I` or `ifconfig`.
2.  Open `include/mqtt_handler.hpp` in the Prodino project.
3.  Update the `MQTT_BROKER_IP` to your laptop's IP:
    ```cpp
    #define MQTT_BROKER_IP "192.168.x.x" // Your laptop IP
    ```
4.  Ensure the Prodino is on the same subnet as your laptop.
5.  Re-upload the code to the Prodino.

## 3. Run the Web UI and Backend Locally
1.  Open a terminal in the `prodino_web_ui` folder.
2.  Install dependencies:
    ```bash
    npm install
    ```
3.  **Configure `.env`:**
    Ensure the `.env` file points to your local broker (usually `localhost` or `127.0.0.1` since the backend is on the same machine):
    ```env
    MQTT_BROKER=mqtt://127.0.0.1:1883
    ```
4.  Start the Backend:
    ```bash
    node server.js
    ```
5.  Start the Web UI (in a separate terminal):
    ```bash
    node ui_server.js
    ```

## 4. Verification
1.  Open your browser to `http://localhost:5556`.
2.  Check the terminal running `server.js`. You should see:
    `MQTT: Connected to local broker`
3.  When the Prodino connects, you should see:
    `MQTT: Received message on topic prodino/status`
4.  The dashboard should now show live data from your Prodino.

## 5. Troubleshooting
- **`rc = -2` on Prodino:** The Prodino cannot find your laptop. Check that both are on the same Wi-Fi/Ethernet network and that your laptop's firewall is not blocking port 1883.
- **No data in UI:** Ensure the `MQTT_BROKER` in `.env` is correct and that the backend is successfully connecting to the broker.
