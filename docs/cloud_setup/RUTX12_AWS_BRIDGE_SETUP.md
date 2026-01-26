## 1. RUTX12 Broker Configuration (Local)
1.  Go to **Services -> MQTT -> Broker**.
2.  **General Settings:**
    *   **Enable:** On
    *   **Local Port:** `1883`
    *   **Enable remote access:** `On` (Allows LAN connections from Prodino)
3.  **Security Settings (Broker):**
    *   **Use TLS/SSL:** `Off`
    *   **Require certificate:** `Off`
    *   **Miscellaneous:** `Allow anonymous` :
         *   **Allow anonymous:** `On`
4.  Click **Save & Apply**.

## 2. RUTX12 Bridge Configuration (Cloud)
1.  Go to **Services -> MQTT -> Bridge**.
2.  Add a new Bridge instance (e.g., `AWS_Bridge`).
3.  **General Settings:**
    *   **Enable:** On
    *   **Remote Address:** Your AWS Endpoint (e.g., `aa0ttgw7natni-ats.iot.eu-north-1.amazonaws.com`)
    *   **Remote Port:** `8883`
    *   **Protocol:** `MQTT` (Secure)
    *   **Connection name:** `GRK-SNxxxx` (If missing, use Bridge Connection Name).
    *   **protocol version:** `3.1.1`
    *   **Clean Session:** `On`
4.  **TLS/SSL Settings:**
    *   **Use TLS/SSL:** `On`
    *   **Certificate files from device:** `Off` (Important: Do not use simplified mode)
    *   **Bridge CA File:** Upload `AmazonRootCA1.pem`
    *   **Bridge Certificate File:** Upload `device_certificate.pem.crt`
    *   **Bridge Key File:** Upload `private_key.pem.key`
    *   **Insecure:** `Off`
    *   **Clean Session:** `On`
5.  **Topics (Subscription/Forwarding):**
    *   **Topic:** `prodino/SNxxxx/#`
    *   **Direction:** `Out`
    *   **QoS:** `1`
6.  Click **Save & Apply**.

## 3. Verification (RUTX12 CLI)
SSH into the RUTX12 and run:
```bash
logread -f | grep mosquitto
```
Look for these lines:
- `Bridge GRK-SNxxxx sending CONNECT`
- `Received CONNACK`

## 4. Verification (End-to-End)
1.  Open the **AWS IoT Core Console -> Test -> MQTT Test Client**.
2.  Subscribe to `prodino/#`.
3.  From a laptop on the same network as the RUTX12 (using standard `mosquitto_pub`):
    ```bash
    mosquitto_pub -h 192.168.100.131 -t "prodino/status" -m "RUTX12 Bridge is LIVE!"
    ```
    *Note: If local broker TLS is OFF (default), do not use --cafile arguments.*
4.  If the message appears in AWS, the bridge is working.

## 5. Troubleshooting
- **`Connection lost` error locally:** Check that "Require certificate" is OFF in Local Broker settings.
- **`Connection lost` error on Bridge:** Check that the Bridge Client ID matches the AWS Thing Name, or Policy allows all Client IDs.
