# Spotify API Setup Guide

This guide explains how to get your Spotify API credentials, including the refresh token.

## Step 1: Create a Spotify App

1. Go to [Spotify Developer Dashboard](https://developer.spotify.com/dashboard)
2. Log in with your Spotify account
3. Click **"Create App"**
4. Fill in:
   - **App name**: ESP8266 Player (or anything)
   - **App description**: Anything
   - **Redirect URI**: `http://localhost:8888/callback`
5. Check "I understand" and click **Save**
6. Click **Settings** to see your **Client ID** and **Client Secret**

## Step 2: Get Your Refresh Token

### Option A: Using the Online Tool (Easiest)

1. Go to: https://getyourspotifyrefreshtoken.com/
2. Enter your Client ID and Client Secret
3. Click "Get Refresh Token"
4. Log in to Spotify and authorize
5. Copy the refresh token

### Option B: Using a Python Script

Create a file called `get_token.py`:

```python
import base64
import requests
from urllib.parse import urlencode
import webbrowser
from http.server import HTTPServer, BaseHTTPRequestHandler

CLIENT_ID = "YOUR_CLIENT_ID"
CLIENT_SECRET = "YOUR_CLIENT_SECRET"
REDIRECT_URI = "http://localhost:8888/callback"
SCOPES = "user-read-playback-state user-modify-playback-state user-read-currently-playing"

auth_code = None

class CallbackHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        global auth_code
        if "code=" in self.path:
            auth_code = self.path.split("code=")[1].split("&")[0]
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
            self.wfile.write(b"<h1>Success! You can close this window.</h1>")
        else:
            self.send_response(400)
            self.end_headers()

    def log_message(self, format, *args):
        pass

# Step 1: Open browser for authorization
auth_url = "https://accounts.spotify.com/authorize?" + urlencode({
    "client_id": CLIENT_ID,
    "response_type": "code",
    "redirect_uri": REDIRECT_URI,
    "scope": SCOPES,
})

print("Opening browser for Spotify authorization...")
webbrowser.open(auth_url)

# Step 2: Wait for callback
server = HTTPServer(("localhost", 8888), CallbackHandler)
print("Waiting for authorization...")
while auth_code is None:
    server.handle_request()

# Step 3: Exchange code for tokens
print("Exchanging code for tokens...")
credentials = base64.b64encode(f"{CLIENT_ID}:{CLIENT_SECRET}".encode()).decode()
response = requests.post(
    "https://accounts.spotify.com/api/token",
    headers={"Authorization": f"Basic {credentials}"},
    data={
        "grant_type": "authorization_code",
        "code": auth_code,
        "redirect_uri": REDIRECT_URI,
    },
)

tokens = response.json()
print("\n" + "=" * 50)
print("YOUR REFRESH TOKEN:")
print("=" * 50)
print(tokens.get("refresh_token", "Error: " + str(tokens)))
print("=" * 50)
```

Run it:
```bash
pip install requests
python get_token.py
```

### Option C: Using cURL (Manual)

1. **Get authorization code** - Open this URL in browser (replace YOUR_CLIENT_ID):
   ```
   https://accounts.spotify.com/authorize?client_id=YOUR_CLIENT_ID&response_type=code&redirect_uri=http://localhost:8888/callback&scope=user-read-playback-state%20user-modify-playback-state%20user-read-currently-playing
   ```

2. After authorizing, you'll be redirected to a URL like:
   ```
   http://localhost:8888/callback?code=AQBxxxxxxxx...
   ```
   Copy the `code` value.

3. **Exchange code for refresh token**:
   ```bash
   # First, encode your credentials (client_id:client_secret) in base64
   echo -n "YOUR_CLIENT_ID:YOUR_CLIENT_SECRET" | base64

   # Then make the request
   curl -X POST "https://accounts.spotify.com/api/token" \
     -H "Authorization: Basic YOUR_BASE64_CREDENTIALS" \
     -H "Content-Type: application/x-www-form-urlencoded" \
     -d "grant_type=authorization_code&code=YOUR_AUTH_CODE&redirect_uri=http://localhost:8888/callback"
   ```

4. The response will contain your `refresh_token`.

## Step 3: Find Your Device Name

1. Open Spotify on your phone or computer
2. Play any song
3. Tap the "Devices Available" icon (speaker/device icon)
4. Note the **exact name** of the device you want to control

Examples:
- `Living Room Speaker`
- `Echo Dot`
- `SAMSUNG TV`

## Step 4: Configure Your ESP8266

Edit `include/Config_local.h`:

```cpp
#define SPOTIFY_CLIENT_ID     "abc123..."
#define SPOTIFY_CLIENT_SECRET "xyz789..."
#define SPOTIFY_REFRESH_TOKEN "AQDxxxxxxxxxxxxxxx..."
#define SPOTIFY_DEVICE_NAME   "Living Room Speaker"
```

Then build and upload:
```bash
pio run -t upload
```

## Step 5: Using the Web Interface

After uploading, you can select your Spotify device via the web interface:

1. Connect to the same WiFi network as your ESP8266
2. Open a browser and go to `http://<ESP8266-IP-ADDRESS>/`
   - The IP address is shown in the serial monitor during boot
3. The web page shows:
   - Current connection status (WiFi, Spotify auth, selected device)
   - A dropdown to select from available Spotify devices
4. Select your device and click "Set Device"
5. The LED ring will flash cyan to confirm device selection

**Note**: Make sure Spotify is open/playing on at least one device for it to appear in the dropdown.

## LED Feedback Reference

| LED Pattern                | Meaning                           |
| -------------------------- | --------------------------------- |
| Yellow pulsing             | Connecting to WiFi                |
| Red fast blink             | WiFi error/disconnected           |
| Blue pulsing               | Connecting to Spotify             |
| Orange slow blink          | Spotify error (auth/device issue) |
| Blue spinning              | Reading NFC tag                   |
| Green flash (3x)           | Tag played successfully           |
| Red flash (5x)             | Tag failed to play                |
| Cyan flash (2x)            | Device selected via web UI        |
| Dim green (sound reactive) | Idle, ready for NFC               |

## Troubleshooting

### "Device not found"
- Make sure the Spotify device is powered on
- Play something on it first to "wake it up"
- Check the device name matches exactly (case-sensitive)

### "Token fetch failed" / 401 error
- Verify Client ID and Secret are correct
- Make sure you copied the entire refresh token
- Refresh tokens don't expire, but they can be invalidated if you regenerate them

### Scopes / Permissions
The refresh token needs these scopes:
- `user-read-playback-state` - Check what's playing
- `user-modify-playback-state` - Control playback
- `user-read-currently-playing` - Get current track info

If your token was generated with different scopes, generate a new one.
