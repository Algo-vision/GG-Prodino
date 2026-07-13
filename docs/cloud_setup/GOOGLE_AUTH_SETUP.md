# Google OAuth Authentication Setup Guide

This guide explains how to configure Google OAuth for the GRK Mission Control dashboard.

## Prerequisites
- A Google account
- Access to [Google Cloud Console](https://console.cloud.google.com/)

---

## 1. Create a Google Cloud Project

1. Go to [Google Cloud Console](https://console.cloud.google.com/)
2. Click **Select a project** → **New Project**
3. **Project name:** `GRK-Mission-Control`
4. Click **Create**

---

## 2. Configure OAuth Consent Screen

1. Go to **APIs & Services → OAuth consent screen**
2. Select **External** user type, click **Create**
3. Fill in:
   - **App name:** `GRK Mission Control`
   - **User support email:** Your email
   - **Developer contact:** Your email
4. Click **Save and Continue** through remaining steps
5. On **Test users** step, add the admin emails:
   - `ron@gg-el.com`
   - `haim.hadad@algowis.com`
6. Click **Save and Continue**, then **Back to Dashboard**

---

## 3. Create OAuth 2.0 Credentials

1. Go to **APIs & Services → Credentials**
2. Click **+ Create Credentials → OAuth client ID**
3. **Application type:** Web application
4. **Name:** `GRK Web UI`
5. **Authorized redirect URIs:** Add the appropriate URI(s):

### For Local Development:
```
http://localhost:5555/auth/google/callback
```

### For Production (EC2):
```
http://<YOUR_EC2_PUBLIC_IP>:5555/auth/google/callback
```

6. Click **Create**
7. Copy the **Client ID** and **Client Secret**

---

## 4. Configure Environment Variables

### Local Development (`.env` file):
```env
# Google OAuth
GOOGLE_CLIENT_ID=your-client-id.apps.googleusercontent.com
GOOGLE_CLIENT_SECRET=your-client-secret
GOOGLE_CALLBACK_URL=http://localhost:5555/auth/google/callback

# Session (change in production)
SESSION_SECRET=your-random-32-character-secret

# Admin emails (comma-separated)
ADMIN_EMAILS=ron@gg-el.com,haim.hadad@algowis.com
```

### Production (EC2):
```env
# Google OAuth
GOOGLE_CLIENT_ID=your-client-id.apps.googleusercontent.com
GOOGLE_CLIENT_SECRET=your-client-secret
GOOGLE_CALLBACK_URL=http://<YOUR_EC2_PUBLIC_IP>:5555/auth/google/callback

# Session (use a strong random secret)
SESSION_SECRET=<generate-a-strong-random-string>

# Admin emails
ADMIN_EMAILS=ron@gg-el.com,haim.hadad@algowis.com
```

> **Tip:** Generate a secure session secret:
> ```bash
> node -e "console.log(require('crypto').randomBytes(32).toString('hex'))"
> ```

---

## 5. Restart the Server

### Local:
```bash
cd web_ui
npm start
```

### Production (EC2):
```bash
pm2 restart grk-backend
```

---

## 6. Test the Login Flow

1. Open `http://localhost:5555` (or your EC2 IP)
2. You'll be redirected to the login page
3. Click **Sign in with Google**
4. Log in with an authorized admin email
5. You should be redirected to the dashboard

---

## User Management

### Adding Users (Admin Only)
1. Log in as an admin
2. Go to `/admin.html` (or click the admin link in the telemetry log)
3. Enter the new user's email and click **Add User**

### Access Levels
| Role | Access |
|------|--------|
| Admin | All devices + user management |
| User (all) | All devices, no user management |
| User (restricted) | Only assigned devices |

---

## Troubleshooting

### Error: "invalid_client"
- **Cause:** Wrong or placeholder credentials in `.env`
- **Fix:** Copy the correct Client ID and Secret from Google Cloud Console

### Error: "redirect_uri_mismatch"
- **Cause:** Callback URL doesn't match Google Console settings
- **Fix:** Ensure `GOOGLE_CALLBACK_URL` exactly matches the URI in Google Console

### Error: "Access denied"
- **Cause:** Email not in the authorized users table
- **Fix:** Ask an admin to add your email via `/admin.html`

---

## Security Notes

- In production, consider using HTTPS (requires SSL certificate)
- Keep `SESSION_SECRET` confidential and unique per environment
- Regularly review the user list in the admin panel
