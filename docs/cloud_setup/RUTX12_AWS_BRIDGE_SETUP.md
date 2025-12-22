# RUTX12 MQTT Bridge and Prodino Setup Guide

This guide describes how to configure the Teltonika RUTX12 as a gateway between the Prodino device and AWS IoT Core.

## 1. Network Subnet Alignment
For the Prodino to talk to the RUTX12, they must be on the same subnet.
- **RUTX12 IP:** `192.168.100.131` (Default gateway: `192.168.100.1`)
- **Prodino IP:** `192.168.100.198`
- **Subnet Mask:** `255.255.255.0`

> [!IMPORTANT]
> If the Prodino is on `192.168.1.x`, it will fail to connect to the RUTX12. Ensure the Prodino code is updated and uploaded with the `100` subnet address.

## 2. RUTX12 Bridge Configuration (WebUI)
1.  Go to **Services -> MQTT -> Broker**.
2.  Set **Enable** to **On** and click **Save & Apply**.
3.  Go to **Services -> MQTT -> Bridge**.
4.  Add a new Bridge (e.g., `AWS_Bridge`).
5.  **General Settings:**
    *   **Remote Address:** Your AWS Endpoint (e.g., `aa0ttgw7natni-ats.iot.eu-north-1.amazonaws.com`)
    *   **Remote Port:** `8883`
    *   **Remote ID:** `GRK_RUTX12` (Must match your AWS Thing name).
    *   **Try Private:** `Off` (Crucial for AWS).
    *   **Clean Session:** `On`.
6.  **TLS Settings:**
    *   **Enable TLS:** `On`.
    *   **CA File:** Upload `AmazonRootCA1.pem`.
    *   **Cert File:** Upload `rutx12-cert.pem.crt`.
    *   **Key File:** Upload `rutx12-private.pem.key`.
7.  **Topic Settings:**
    *   **Topic:** `prodino/#`
    *   **Direction:** `Out`
    *   **QoS:** `1`
8.  Click **Save & Apply**.

## 3. Verification (RUTX12 CLI)
SSH into the RUTX12 and run:
```bash
logread -f | grep mosquitto
```
Look for these lines:
- `Bridge GRK_RUTX12 sending CONNECT`
- `Received CONNACK`

## 4. Verification (End-to-End)
1.  Open the **AWS IoT Core Console -> Test -> MQTT Test Client**.
2.  Subscribe to `prodino/#`.
3.  From a laptop on the same network as the RUTX12, send a test message:
    ```bash
    mosquitto_pub -h 192.168.100.131 -t "prodino/status" -m "RUTX12 Bridge is LIVE!"
    ```
4.  If the message appears in AWS, the bridge is working.

## 5. Troubleshooting
- **`rc = -2` on Prodino:** The Prodino cannot find the RUTX12. Check the IP subnet and Ethernet cable.
- **`Connection lost` in RUTX12 logs:** The AWS Policy is likely not attached to the certificate.
- **`SSL Handshake failed`:** Ensure you are using the correct `AmazonRootCA1.pem` file.
