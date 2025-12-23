# AWS IoT Core Console Setup Guide

This guide describes how to set up AWS IoT Core to receive data from the Prodino system.

## 1. Create a "Thing"
1.  Log in to the **AWS Management Console** and navigate to **AWS IoT Core**.
2.  In the left menu, go to **All devices -> Things**.
3.  Click **Create things**.
4.  Select **Create single thing** and click **Next**.
5.  **Thing name:** Enter `GRK_RUTX12` (or your preferred name).
6.  Leave other settings as default and click **Next**.

## 2. Configure Certificates
1.  On the **Configure device certificate** page, select **Auto-generate a new certificate (recommended)**.
2.  Click **Next**.

## 3. Attach a Policy
1.  If you have already created a policy (e.g., `GRK_FullAccess`), select it from the list.
2.  If not, click **Create policy** (this will open a new tab):
    *   **Policy name:** `GRK_FullAccess`
    *   **Policy document (JSON):** Click the **JSON** tab and paste the following:
        ```json
        {
          "Version": "2012-10-17",
          "Statement": [
            {
              "Effect": "Allow",
              "Action": "iot:*",
              "Resource": "*"
            }
          ]
        }
        ```
    *   (Note: `*` is for development; restrict this in production).
    *   Click **Create**.
3.  Go back to the "Create thing" tab, refresh the policy list, and select `GRK_FullAccess`.
4.  Click **Create thing**.

## 4. Download and Activate Files
1.  A popup will appear with download links. **Download all of them**:
    *   **Device certificate** (e.g., `...-certificate.pem.crt`)
    *   **Public key file**
    *   **Private key file** (e.g., `...-private.pem.key`)
    *   **Root CA certificates:** Download **Amazon Root CA 1**.
2.  **IMPORTANT:** You must click the **Activate** button for the certificate if it isn't already active.

## 5. Verify Policy Attachment
1.  Go to **All devices -> Certificates**.
2.  Click on the certificate you just created.
3.  Go to the **Policies** tab.
4.  Ensure `GRK_FullAccess` is listed. If not, click **Actions -> Attach policy**.

## 6. Get Your Endpoint
1.  In the AWS IoT Core menu, go to **Settings** (at the bottom of the left sidebar).
2.  Copy the **Device data endpoint** (e.g., `aa0ttgw7natni-ats.iot.eu-north-1.amazonaws.com`).
3.  This endpoint is used by both the RUTX12 and the Node.js backend.

## 7. Backend (Dashboard) Certificates
The Node.js backend needs its own set of certificates to connect to AWS IoT Core. You have two options:

### Option A: Create a new "Thing" (Recommended for organization)
1.  Repeat **Steps 1-4** to create another "Thing" named `GRK_Dashboard_Backend`.
2.  Download the certificates and keys.

### Option B: Create only a Certificate (Simpler)
1.  In the AWS IoT Core menu, go to **Security -> Certificates**.
2.  Click **Create certificate**.
3.  Select **Auto-generate a new certificate** and click **Create**.
4.  Download the **Device certificate**, **Private key**, and **Amazon Root CA 1**.
5.  Click **Activate**.
6.  Go to the **Policies** tab of the new certificate and click **Attach policy**.
7.  Select `GRK_FullAccess` and click **Attach**.

### Final Step: Rename and Upload
1.  Rename the downloaded files for clarity (e.g., `backend-private.pem.key`, `backend-cert.pem.crt`, `AmazonRootCA1.pem`).
2.  These files will be placed in the `prodino_web_ui/certs/` directory on your server.
3. please update the .env file with the new paths.
