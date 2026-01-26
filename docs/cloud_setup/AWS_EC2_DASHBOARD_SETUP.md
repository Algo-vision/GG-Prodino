# AWS EC2 Dashboard Server Setup Guide

This guide describes how to set up an EC2 instance to host the Prodino Web UI and Backend with multi-device support and Google OAuth authentication.

## 1. Launch an EC2 Instance
1.  Navigate to **EC2 -> Instances -> Launch instances**.
2.  **Name:** `GRK_Dashboard_Server`.
3.  **OS Image:** Select **Ubuntu** (LTS version).
4.  **Instance type:** `t3.micro` (Free tier eligible).
5.  **Key pair:** Create a new key pair or select an existing one. **Download the `.pem` file** and keep it safe.
6.  **Network settings:**
    *   Allow SSH traffic from: `Anywhere` (or your IP for better security).
    *   Allow HTTP/HTTPS traffic from the internet.
7.  Click **Launch instance**.

## 2. Configure Security Group (Firewall)
1.  Go to your **Instance Summary -> Security tab**.
2.  Click on the **Security Group** ID.
3.  Click **Edit inbound rules**.
4.  Add the following **Custom TCP** rules:
    *   **Port 5556** (Web UI) - Source: `0.0.0.0/0`
    *   **Port 5555** (Backend API) - Source: `0.0.0.0/0`
5.  Click **Save rules**.

## 3. Connect and Install Dependencies
Open a terminal on your laptop and SSH into the instance:
```bash
ssh -i "path/to/your-key.pem" ubuntu@<YOUR_PUBLIC_IP>
```
*** the key should be instance_gg_key.pem ***
if there is an error like this:
```bash
ssh -i "path/to/your-key.pem" ubuntu@<YOUR_PUBLIC_IP>
The authenticity of host '<YOUR_PUBLIC_IP> (<YOUR_PUBLIC_IP>)' can't be established.
ED25519 key fingerprint is SHA256:zIw1zbA94CoWQ29q/c5w89GWhN2uv1hzpkXnQpXR9v8.
This key is not known by any other names.
Are you sure you want to continue connecting (yes/no/[fingerprint])? yes
Warning: Permanently added '<YOUR_PUBLIC_IP>' (ED25519) to the list of known hosts.
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@         WARNING: UNPROTECTED PRIVATE KEY FILE!          @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
Permissions 0664 for 'path/to/your-key.pem' are too open.
It is required that your private key files are NOT accessible by others.
This private key will be ignored.
Load key "path/to/your-key.pem": bad permissions
ubuntu@<YOUR_PUBLIC_IP>: Permission denied (publickey).
```
please do this:
```bash
chmod 600 path/to/your-key.pem
```

Once connected, run:
```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install Node.js and NPM
sudo apt install -y nodejs npm

# Install PM2 (Process Manager)
sudo npm install -g pm2
```

## 4. Transfer Code and Certificates
From your **local computer**, upload the `prodino_web_ui` folder (excluding `node_modules`):
```bash
rsync -avz -e "ssh -i keys/remote_aws_grk-key.pem" --exclude 'node_modules' --exclude 'data' --exclude '*.db' ./prodino_web_ui ubuntu@<YOUR_PUBLIC_IP>:/home/ubuntu/
```

### Upload Backend Certificates
Ensure the certificates for the backend are uploaded to the `certs/` folder on the EC2 instance:
```bash
rsync -avz -e "ssh -i keys/remote_aws_grk-key.pem" ./prodino_web_ui/certs ubuntu@<YOUR_PUBLIC_IP>:/home/ubuntu/prodino_web_ui/
```

## 5. Configure and Run
On the **VPS terminal**:
1.  Navigate to the folder: `cd prodino_web_ui`
2.  Install dependencies: `npm install`
3.  Update the `.env` file with your configuration.

### Detailed `.env` Configuration
Create or edit the `.env` file in the `prodino_web_ui` directory:
```bash
nano .env
```

Paste and customize the following configuration:
```env
# AWS IoT Core Endpoint (from IoT Core Settings)
MQTT_BROKER=mqtts://aa0ttgw7natni-ats.iot.eu-north-1.amazonaws.com

# Paths to the certificates uploaded in Step 4
MQTT_KEY_PATH=./certs/backend-private.pem.key
MQTT_CERT_PATH=./certs/backend-cert.pem.crt
MQTT_CA_PATH=./certs/AmazonRootCA1.pem

# Port configuration
BACKEND_PORT=5555
UI_PORT=5556

# Google OAuth (see GOOGLE_AUTH_SETUP.md for details)
GOOGLE_CLIENT_ID=your-client-id.apps.googleusercontent.com
GOOGLE_CLIENT_SECRET=your-client-secret
GOOGLE_CALLBACK_URL=http://<YOUR_PUBLIC_IP>:5555/auth/google/callback

# Session Secret (generate a strong random string)
SESSION_SECRET=your-random-secret-key-here

# Admin Emails (comma-separated, these users have full access)
ADMIN_EMAILS=ron@gg-el.com,haim.hadad@algowis.com
```

**Variable Descriptions:**
*   **MQTT_BROKER:** Your unique AWS IoT Data Endpoint. Must start with `mqtts://`.
*   **MQTT_KEY_PATH:** Path to your private key file (e.g., `...-private.pem.key`).
*   **MQTT_CERT_PATH:** Path to your device certificate (e.g., `...-certificate.pem.crt`).
*   **MQTT_CA_PATH:** Path to the `AmazonRootCA1.pem` certificate.
*   **BACKEND_PORT:** The port the Socket.io server will listen on (default: 5555).
*   **UI_PORT:** The port the Web UI server will listen on (default: 5556).
*   **GOOGLE_CLIENT_ID/SECRET:** OAuth credentials from Google Cloud Console.
*   **SESSION_SECRET:** Random string for session encryption (keep confidential).
*   **ADMIN_EMAILS:** Comma-separated list of admin user emails.

4.  Start the applications:
    ```bash
    pm2 start server.js --name "grk-backend"
    pm2 start ui_server.js --name "grk-ui"
    ```
5.  Verify they are running: `pm2 status`

## 6. Access the Dashboard
Open your browser and go to:
`http://<YOUR_PUBLIC_IP>:5556`

You will be redirected to the login page. Sign in with an authorized Google account.

---

## Multi-Device Support

The backend automatically supports multiple Prodino devices:

- **MQTT Subscription:** Listens to `prodino/#` (all devices)
- **Topic Format:** Each device publishes to `prodino/{serial_number}/...`
- **Web UI:** Shows a fleet overview with all connected devices

### Device Topics Example
```
prodino/SN0001/gps/position
prodino/SN0001/imu/orientation
prodino/SN0002/gps/position
prodino/SN0002/imu/orientation
```

### SQLite Database
User data is stored in `data/grk_users.db`. This file is created automatically and persists across restarts.

**Backup the database before updates:**
```bash
cp data/grk_users.db data/grk_users.db.backup
```

---

## Updating the Server

After making code changes locally, follow these steps to deploy and restart:

### 1. Upload Updated Code
From your **local computer**:
```bash
rsync -avz -e "ssh -i keys/instance_gg_key.pem" --exclude 'node_modules' --exclude 'data' --exclude '*.db' ./prodino_web_ui ubuntu@<YOUR_PUBLIC_IP>:/home/ubuntu/
```

### 2. Restart the Server
SSH into the EC2 instance and restart the services:
```bash
ssh -i "keys/instance_gg_key.pem" ubuntu@<YOUR_PUBLIC_IP>

# Restart the backend server
pm2 restart grk-backend

# Optional: Restart the UI server if changed
pm2 restart grk-ui

# Check status
pm2 status
```

### Useful PM2 Commands
```bash
pm2 logs grk-backend     # View server logs
pm2 logs grk-backend --lines 100  # View last 100 lines
pm2 restart all          # Restart all services
pm2 stop grk-backend     # Stop the backend
pm2 start grk-backend    # Start the backend
```
