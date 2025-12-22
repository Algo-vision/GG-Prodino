# AWS EC2 Dashboard Server Setup Guide

This guide describes how to set up an EC2 instance to host the Prodino Web UI and Backend.

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
rsync -avz -e "ssh -i keys/remote_aws_grk-key.pem" --exclude 'node_modules' ./prodino_web_ui ubuntu@<YOUR_PUBLIC_IP>:/home/ubuntu/
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
3.  Update the `.env` file with your AWS IoT Endpoint and certificate paths.
4.  Start the applications:
    ```bash
    pm2 start server.js --name "grk-backend"
    pm2 start ui_server.js --name "grk-ui"
    ```
5.  Verify they are running: `pm2 status`

## 6. Access the Dashboard
Open your browser and go to:
`http://<YOUR_PUBLIC_IP>:5556`
